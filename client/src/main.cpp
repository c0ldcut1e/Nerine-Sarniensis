#include <unistd.h>

#include "core/nerine_globals.hpp"
#include "core/nerine_utils.hpp"
#include "overlay/nerine_overlay.hpp"
#include "reflection/offset_resolver.hpp"

#include <fmt/base.h>
#include <fmt/core.h>

int main()
{
    NerineOverlay overlay;
    pid_t pid = GetPidByProcessName("RocketLeague.exe");
    if (pid >= 0)
    {
        fmt::println(stderr, "Rocket League is already running");
        return 1;
    }

    if (!overlay.StartVirtualGamepad())
    {
        fmt::println(stderr, "Failed to initialize the virtual gamepad");
        return 1;
    }

    fmt::println("Waiting for Rocket League");
    while (pid < 0)
    {
        usleep(250000);
        pid = GetPidByProcessName("RocketLeague.exe");
    }

    fmt::println("Found Rocket League pid {}", pid);

    NerineGlobals::GetInstance()->SetPid(pid);

    uint64_t moduleBase = GetModuleBase(pid, "RocketLeague.exe");
    if (moduleBase == 0)
    {
        fmt::println(stderr, "Failed to find the module base\n");
        return 1;
    }

    fmt::println("Found module base at 0x{:016X}", moduleBase);

    NerineGlobals::GetInstance()->SetModuleBase(moduleBase);

    fmt::println("Resolving game offsets");
    while (!OffsetResolver::GetInstance()->Resolve())
    {
        usleep(250000);
    }

    return overlay.Run();
}
