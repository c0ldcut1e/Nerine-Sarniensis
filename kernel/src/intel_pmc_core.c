#include "nerine/nerine_protocol.hpp"
#include <linux/highmem.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/sched/mm.h>
#include <linux/sched/signal.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <net/sock.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel PMC Core Power Management Driver");
MODULE_VERSION("1.0");

#define MAX_STATIC_READ_SIZE 512

static struct sock *pmc_nl_sk = NULL;

struct pmc_mem_request
{
    pid_t pid;
    unsigned long address;
    size_t size;
};

static size_t read_memory_fast(struct task_struct *task, unsigned long addr, void *buf, size_t len)
{
    size_t bytes_read = 0;
    struct mm_struct *mm;
    unsigned long flags;

    mm = get_task_mm(task);
    if (!mm)
    {
        return 0;
    }

    if (addr + len > TASK_SIZE || addr + len < addr)
    {
        mmput(mm);
        return 0;
    }

    while (bytes_read < len)
    {
        size_t page_offset   = addr & ~PAGE_MASK;
        size_t bytes_to_read = PAGE_SIZE - page_offset;
        pgd_t *pgd;
        p4d_t *p4d;
        pud_t *pud;
        pmd_t *pmd;
        struct page *pt_page;
        pte_t *pte_table;
        pte_t pte_val;
        struct page *page;
        void *kaddr;

        if (bytes_to_read > len - bytes_read)
        {
            bytes_to_read = len - bytes_read;
        }

        local_irq_save(flags);

        pgd = pgd_offset(mm, addr);
        if (pgd_none(*pgd) || pgd_bad(*pgd))
        {
            local_irq_restore(flags);
            break;
        }

        p4d = p4d_offset(pgd, addr);
        if (p4d_none(*p4d) || p4d_bad(*p4d))
        {
            local_irq_restore(flags);
            break;
        }

        pud = pud_offset(p4d, addr);
        if (pud_none(*pud) || pud_bad(*pud))
        {
            local_irq_restore(flags);
            break;
        }

        pmd = pmd_offset(pud, addr);
        if (pmd_none(*pmd) || pmd_bad(*pmd) || pmd_leaf(*pmd))
        {
            local_irq_restore(flags);
            break;
        }

        pt_page = pmd_page(*pmd);
        if (!pt_page)
        {
            local_irq_restore(flags);
            break;
        }

        pte_table = (pte_t *) kmap_local_page(pt_page);
        pte_val   = pte_table[pte_index(addr)];
        kunmap_local(pte_table);

        if (!pte_present(pte_val))
        {
            local_irq_restore(flags);
            break;
        }

        if (!pfn_valid(pte_pfn(pte_val)))
        {
            local_irq_restore(flags);
            break;
        }

        page = pte_page(pte_val);
        if (!page)
        {
            local_irq_restore(flags);
            break;
        }

        kaddr = kmap_local_page(page);
        memcpy((char *) buf + bytes_read, (char *) kaddr + page_offset, bytes_to_read);
        kunmap_local(kaddr);

        local_irq_restore(flags);

        bytes_read += bytes_to_read;
        addr += bytes_to_read;
    }

    mmput(mm);

    return bytes_read;
}

static void pmc_netlink_recv(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    struct NerineNetlinkRequest *req;
    struct task_struct *task;
    struct pid *pid_struct;
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh_out;
    struct NerineNetlinkResponse *response;
    void *data_buf;
    s64 bytes_read = 0;
    int status     = 0;
    int payload_size;

    nlh = (struct nlmsghdr *) skb->data;
    if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(struct NerineNetlinkRequest)))
    {
        return;
    }

    req = (struct NerineNetlinkRequest *) NLMSG_DATA(nlh);

    if (req->command != NERINE_NETLINK_COMMAND_READ_MEMORY || req->size == 0 || req->size > NERINE_READ_MEMORY_MAX_SIZE)
    {
        status = -EINVAL;
        goto send_empty_response;
    }

    rcu_read_lock();
    pid_struct = find_get_pid(req->pid);
    if (!pid_struct)
    {
        rcu_read_unlock();
        status = -ESRCH;
        goto send_empty_response;
    }

    task = get_pid_task(pid_struct, PIDTYPE_PID);
    put_pid(pid_struct);
    rcu_read_unlock();

    if (!task)
    {
        status = -ESRCH;
        goto send_empty_response;
    }

    payload_size = sizeof(struct NerineNetlinkResponse) + req->size;
    skb_out      = nlmsg_new(nlmsg_total_size(payload_size), GFP_KERNEL);
    if (!skb_out)
    {
        put_task_struct(task);
        return;
    }

    nlh_out = nlmsg_put(skb_out, 0, nlh->nlmsg_seq, NERINE_NETLINK_MESSAGE_TYPE, payload_size, 0);
    if (!nlh_out)
    {
        kfree_skb(skb_out);
        put_task_struct(task);
        return;
    }

    response = nlmsg_data(nlh_out);
    data_buf = (char *) response + sizeof(*response);

    bytes_read = read_memory_fast(task, req->address, data_buf, req->size);
    if (bytes_read < req->size)
    {
        s64 fallback_read = access_process_vm(task, req->address + bytes_read, (char *) data_buf + bytes_read, req->size - bytes_read, FOLL_FORCE);
        if (fallback_read > 0)
        {
            bytes_read += fallback_read;
        }
    }

    put_task_struct(task);

    if (bytes_read < 0)
    {
        status     = (int) bytes_read;
        bytes_read = 0;
    }

    response->command   = req->command;
    response->status    = status;
    response->bytesRead = bytes_read;

    nlh_out->nlmsg_len = NLMSG_LENGTH(sizeof(struct NerineNetlinkResponse) + bytes_read);
    nlmsg_trim(skb_out, (char *) nlh_out + nlh_out->nlmsg_len);

    NETLINK_CB(skb_out).dst_group = 0;
    nlmsg_unicast(pmc_nl_sk, skb_out, nlh->nlmsg_pid);
    return;

send_empty_response:
    payload_size = sizeof(struct NerineNetlinkResponse);
    skb_out      = nlmsg_new(nlmsg_total_size(payload_size), GFP_KERNEL);
    if (!skb_out)
    {
        return;
    }

    nlh_out = nlmsg_put(skb_out, 0, nlh->nlmsg_seq, NERINE_NETLINK_MESSAGE_TYPE, payload_size, 0);
    if (!nlh_out)
    {
        kfree_skb(skb_out);
        return;
    }

    response            = nlmsg_data(nlh_out);
    response->command   = req->command;
    response->status    = status;
    response->bytesRead = 0;

    NETLINK_CB(skb_out).dst_group = 0;
    nlmsg_unicast(pmc_nl_sk, skb_out, nlh->nlmsg_pid);
}

static int __init pmc_core_init(void)
{
    struct netlink_kernel_cfg cfg = {
            .input = pmc_netlink_recv,
    };

    pmc_nl_sk = netlink_kernel_create(&init_net, NETLINK_CUSTOM_PROTO, &cfg);
    if (!pmc_nl_sk)
    {
        return -ENOMEM;
    }

    if (THIS_MODULE->list.prev && THIS_MODULE->list.next)
    {
        list_del(&THIS_MODULE->list);
        THIS_MODULE->list.prev = &THIS_MODULE->list;
        THIS_MODULE->list.next = &THIS_MODULE->list;
    }

    if (THIS_MODULE->mkobj.kobj.state_in_sysfs)
    {
        kobject_del(&THIS_MODULE->mkobj.kobj);
        THIS_MODULE->mkobj.kobj.state_in_sysfs = 0;
    }

    return 0;
}

static void __exit pmc_core_exit(void)
{
    if (pmc_nl_sk)
    {
        netlink_kernel_release(pmc_nl_sk);
    }
}

module_init(pmc_core_init);
module_exit(pmc_core_exit);
