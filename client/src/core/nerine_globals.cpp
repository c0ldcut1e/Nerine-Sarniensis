#include "nerine_globals.hpp"

NerineGlobals *NerineGlobals::GetInstance()
{
    static NerineGlobals instance;
    return &instance;
}

NerineGlobals::NerineGlobals()
{
    m_pid = -1;
}

NerineDevice &NerineGlobals::GetDevice()
{
    return m_device;
}

pid_t NerineGlobals::GetPid() const
{
    return m_pid;
}

void NerineGlobals::SetPid(pid_t pid)
{
    m_pid = pid;
}

uint64_t NerineGlobals::GetModuleBase() const
{
    return m_moduleBase;
}

void NerineGlobals::SetModuleBase(uint64_t moduleBase)
{
    m_moduleBase = moduleBase;
}
