#include <unistd.h>

#include "nerine_globals.hpp"
#include "nerine_overlay.hpp"
#include "nerine_utils.hpp"

#include <fmt/base.h>
#include <fmt/core.h>

int main()
{
    NerineOverlay overlay;
    pid_t pid = GetPidByProcessName("RocketLeague.exe");
    if (pid >= 0)
    {
        fmt::println(stderr, "Rocket League is already running (pid {}); close it and restart Sarniensis before launching Rocket League", pid);
        return 1;
    }

    fmt::println("Waiting for Rocket League");
    while (pid < 0)
    {
        usleep(250000);
        pid = GetPidByProcessName("RocketLeague.exe");
    }

    fmt::println("Found Rocket League pid {}", pid);

    NerineGlobals::GetInstance().SetPid(pid);

    uint64_t moduleBase = GetModuleBase(pid, "RocketLeague.exe");
    if (moduleBase == 0)
    {
        fmt::println(stderr, "Failed to find the module base\n");
        return 1;
    }

    fmt::println("Found module base at 0x{:016X}", moduleBase);

    NerineGlobals::GetInstance().SetModuleBase(moduleBase);

    return overlay.Run();
}
