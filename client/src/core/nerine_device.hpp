#ifndef NERINE_DEVICE_HPP
#define NERINE_DEVICE_HPP

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <sys/types.h>

#include "nerine/nerine_protocol.hpp"

class NerineDevice
{
public:
    NerineDevice();
    ~NerineDevice();

    NerineDevice(const NerineDevice &other)            = delete;
    NerineDevice &operator=(const NerineDevice &other) = delete;

    bool IsOpen() const;
    ssize_t ReadMemory(pid_t pid, uint64_t address, void *data, size_t size);

private:
    int SendRequest(const struct NerineNetlinkRequest &request, void *data, size_t size, int64_t *bytesRead);

    int m_socket;
    uint32_t m_portId;
    uint32_t m_sequence;
    std::mutex m_mutex;
    char *m_responseBuffer;
};

#endif
