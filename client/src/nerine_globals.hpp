#ifndef NERINE_GLOBALS_HPP
#define NERINE_GLOBALS_HPP

#include <cstdint>
#include <sys/types.h>

#include "nerine_device.hpp"

class NerineGlobals
{
public:
    static NerineGlobals &GetInstance();

    NerineGlobals(const NerineGlobals &other)            = delete;
    NerineGlobals &operator=(const NerineGlobals &other) = delete;

    NerineDevice &GetDevice();

    pid_t GetPid() const;
    void SetPid(pid_t pid);

    uint64_t GetModuleBase() const;
    void SetModuleBase(uint64_t moduleBase);

private:
    NerineGlobals();

    NerineDevice m_device;
    pid_t m_pid;
    uint64_t m_moduleBase;
};

#endif
