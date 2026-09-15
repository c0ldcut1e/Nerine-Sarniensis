#include "nerine_device.hpp"

// #define NO_KERNEL_MODULE

#include <cerrno>
#include <cstring>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef NO_KERNEL_MODULE
#include <sys/uio.h>
#endif

#include <fmt/base.h>

NerineDevice::NerineDevice()
{
    m_socket         = -1;
    m_portId         = 0;
    m_sequence       = 0;
    m_responseBuffer = new char[NLMSG_SPACE(sizeof(struct NerineNetlinkResponse)) + NERINE_READ_MEMORY_MAX_SIZE];

#ifndef NO_KERNEL_MODULE
    struct sockaddr_nl address = {};
    socklen_t addressSize      = sizeof(address);
    int receiveBufferSize      = NERINE_READ_MEMORY_MAX_SIZE + NLMSG_SPACE(sizeof(struct NerineNetlinkResponse));

    m_socket = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_CUSTOM_PROTO);
    if (m_socket < 0)
    {
        fmt::println(stderr, "Failed to create netlink socket. Error: {} ({})", strerror(errno), errno);
        return;
    }

    if (setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, &receiveBufferSize, sizeof(receiveBufferSize)) != 0)
    {
        int error = errno;
        fmt::println(stderr, "Failed to set SO_RCVBUF. Error: {} ({})", strerror(error), error);
        close(m_socket);
        m_socket = -1;
        errno    = error;
        return;
    }

    address.nl_family = AF_NETLINK;
    if (bind(m_socket, (struct sockaddr *) &address, sizeof(address)) != 0)
    {
        int error = errno;
        fmt::println(stderr, "Failed to bind netlink socket. Error: {} ({})", strerror(error), error);
        close(m_socket);
        m_socket = -1;
        errno    = error;
        return;
    }

    if (getsockname(m_socket, (struct sockaddr *) &address, &addressSize) != 0)
    {
        int error = errno;
        fmt::println(stderr, "Failed to get netlink socket name. Error: {} ({})", strerror(error), error);
        close(m_socket);
        m_socket = -1;
        errno    = error;
        return;
    }

    m_portId = address.nl_pid;
    fmt::println("Initialized Netlink socket");
#else
    fmt::println("Initialized using NO_KERNEL_MODULE fallback");
#endif
}

NerineDevice::~NerineDevice()
{
    if (m_socket >= 0)
    {
        close(m_socket);
    }

    delete[] m_responseBuffer;
}

bool NerineDevice::IsOpen() const
{
#ifdef NO_KERNEL_MODULE
    return true;
#else
    return m_socket >= 0;
#endif
}

ssize_t NerineDevice::ReadMemory(pid_t pid, uint64_t address, void *data, size_t size)
{
    if (size == 0 || size > NERINE_READ_MEMORY_MAX_SIZE)
    {
        fmt::println(stderr, "Invalid size parameters (size: {}, max: {})", size, NERINE_READ_MEMORY_MAX_SIZE);
        return -EINVAL;
    }

#ifdef NO_KERNEL_MODULE
    struct iovec localIov;
    struct iovec remoteIov;
    ssize_t bytesRead;

    localIov.iov_base  = data;
    localIov.iov_len   = size;
    remoteIov.iov_base = (void *) address;
    remoteIov.iov_len  = size;

    bytesRead = process_vm_readv(pid, &localIov, 1, &remoteIov, 1, 0);
    if (bytesRead < 0)
    {
        int error = errno;
        fmt::println(stderr, "process_vm_readv failed for PID {} at {:#x}. Error: {} ({})", pid, address, strerror(error), error);
        return -error;
    }

    return bytesRead;
#else
    struct NerineNetlinkRequest request = {};
    int64_t bytesRead;
    int result;

    request.command = NERINE_NETLINK_COMMAND_READ_MEMORY;
    request.pid     = (int32_t) pid;
    request.address = address;
    request.size    = size;

    result = SendRequest(request, data, size, &bytesRead);
    if (result != 0)
    {
        fmt::println(stderr, "SendRequest failed for PID {} at {:#x} (size: {}). Error code: {}", pid, address, size, result);
        return result;
    }

    return bytesRead;
#endif
}

int NerineDevice::SendRequest(const struct NerineNetlinkRequest &request, void *data, size_t size, int64_t *bytesRead)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    char requestBuffer[NLMSG_SPACE(sizeof(request))] = {};

    struct NerineNetlinkResponse response;
    struct msghdr messageHeader = {};
    struct nlmsghdr *message;
    struct sockaddr_nl kernelAddress = {};
    struct iovec requestIov;
    struct iovec responseIov;
    ssize_t receivedSize;

    message              = (struct nlmsghdr *) requestBuffer;
    message->nlmsg_len   = NLMSG_LENGTH(sizeof(request));
    message->nlmsg_type  = NERINE_NETLINK_MESSAGE_TYPE;
    message->nlmsg_flags = NLM_F_REQUEST;
    message->nlmsg_seq   = m_sequence++;
    message->nlmsg_pid   = m_portId;
    memcpy(NLMSG_DATA(message), &request, sizeof(request));

    kernelAddress.nl_family   = AF_NETLINK;
    requestIov.iov_base       = message;
    requestIov.iov_len        = message->nlmsg_len;
    messageHeader.msg_name    = &kernelAddress;
    messageHeader.msg_namelen = sizeof(kernelAddress);
    messageHeader.msg_iov     = &requestIov;
    messageHeader.msg_iovlen  = 1;

    if (sendmsg(m_socket, &messageHeader, 0) < 0)
    {
        int error = errno;
        fmt::println(stderr, "sendmsg failed. Error: {} ({})", strerror(error), error);
        return -error;
    }

    responseIov.iov_base = m_responseBuffer;
    responseIov.iov_len  = NLMSG_SPACE(sizeof(struct NerineNetlinkResponse) + size);

    messageHeader.msg_name    = NULL;
    messageHeader.msg_namelen = 0;
    messageHeader.msg_iov     = &responseIov;
    messageHeader.msg_iovlen  = 1;

    receivedSize = recvmsg(m_socket, &messageHeader, 0);
    if (receivedSize < 0)
    {
        int error = errno;
        fmt::println(stderr, "recvmsg failed. Error: {} ({})", strerror(error), error);
        return -error;
    }

    message = (struct nlmsghdr *) m_responseBuffer;
    if ((messageHeader.msg_flags & MSG_TRUNC) != 0)
    {
        fmt::println(stderr, "Received truncated netlink message (MSG_TRUNC set)");
        return -EPROTO;
    }
    if (!NLMSG_OK(message, receivedSize))
    {
        fmt::println(stderr, "Received invalid netlink message structure (NLMSG_OK failed)");
        return -EPROTO;
    }
    if (message->nlmsg_type != NERINE_NETLINK_MESSAGE_TYPE)
    {
        fmt::println(stderr, "Unexpected netlink message type: {}", message->nlmsg_type);
        return -EPROTO;
    }
    if (message->nlmsg_seq != m_sequence - 1)
    {
        fmt::println(stderr, "Sequence mismatch (expected {}, got {})", m_sequence - 1, message->nlmsg_seq);
        return -EPROTO;
    }
    if (message->nlmsg_len < NLMSG_LENGTH(sizeof(response)))
    {
        fmt::println(stderr, "Response message length too short: {}", message->nlmsg_len);
        return -EPROTO;
    }

    memcpy(&response, NLMSG_DATA(message), sizeof(response));
    if (response.command != request.command)
    {
        fmt::println(stderr, "Command mismatch in response (sent {}, got {})", request.command, response.command);
        return -EPROTO;
    }
    if (response.status > 0 || response.bytesRead < 0 || (uint64_t) response.bytesRead > size)
    {
        fmt::println(stderr, "Bad response metadata (status: {}, bytesRead: {})", response.status, response.bytesRead);
        return -EPROTO;
    }

    if (response.status != 0)
    {
        fmt::println(stderr, "Kernel returned error status code: {}", response.status);
        return response.status;
    }

    if (response.bytesRead > 0 && data != nullptr)
    {
        memcpy(data, (char *) NLMSG_DATA(message) + sizeof(response), response.bytesRead);
    }

    *bytesRead = response.bytesRead;
    return 0;
}
