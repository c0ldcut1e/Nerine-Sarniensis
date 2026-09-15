#ifndef NERINE_PROTOCOL_HPP
#define NERINE_PROTOCOL_HPP

#include <linux/types.h>

#define NETLINK_CUSTOM_PROTO               31

#define NERINE_READ_MEMORY_MAX_SIZE        (1024 * 1024)
#define NERINE_NETLINK_MESSAGE_TYPE        0x10
#define NERINE_NETLINK_COMMAND_READ_MEMORY 1

struct NerineNetlinkRequest
{
    __u32 command;
    __s32 pid;
    __u32 reserved;
    __aligned_u64 address;
    __aligned_u64 size;
};

struct NerineNetlinkResponse
{
    __u32 command;
    __s32 status;
    __aligned_s64 bytesRead;
};

#endif
