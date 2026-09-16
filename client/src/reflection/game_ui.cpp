#include "game_ui.hpp"

#include <chrono>

#include "core/nerine_memory.hpp"
#include "object_array.hpp"
#include "offset_resolver.hpp"
#include "remote_object.hpp"
#include "world_scanner.hpp"

static uint64_t g_gfxShellAddress;
static std::chrono::steady_clock::time_point g_lastShellSearch;

bool GameUi::IsMenuOpen()
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t localPlayerController = WorldScanner::GetLocalPlayerAddress();
    if (!localPlayerController)
    {
        return false;
    }

    uint64_t playerControllerClass = offsetResolver->GetClassAddress("PlayerController_TA");
    uint64_t localPlayer = ReadMemory<uint64_t>(localPlayerController + offsetResolver->GetPropertyOffsetCached(playerControllerClass, "Player"));
    if (!localPlayer)
    {
        return false;
    }

    uint64_t gfxShellClass = offsetResolver->GetClassAddress("GFxShell_TA");
    uint64_t playerOffset  = offsetResolver->GetPropertyOffsetCached(gfxShellClass, "Player");
    RemoteObject gfxShell(g_gfxShellAddress);
    if (!gfxShell.IsValidInstanceOf(gfxShellClass) || ReadMemory<uint64_t>(g_gfxShellAddress + playerOffset) != localPlayer)
    {
        if (std::chrono::steady_clock::now() - g_lastShellSearch < std::chrono::seconds(1))
        {
            return false;
        }

        g_lastShellSearch             = std::chrono::steady_clock::now();
        std::vector<uint64_t> objects = ObjectArray::GetAllObjectAddresses();
        g_gfxShellAddress             = 0;
        for (size_t i = 0; i < objects.size(); i++)
        {
            RemoteObject object(objects[i]);
            if (object.IsValidInstanceOf(gfxShellClass) && ReadMemory<uint64_t>(object.GetAddress() + playerOffset) == localPlayer)
            {
                g_gfxShellAddress = object.GetAddress();
                break;
            }
        }

        if (g_gfxShellAddress == 0)
        {
            return false;
        }
    }

    return (ReadMemory<uint32_t>(g_gfxShellAddress + offsetResolver->GetPropertyOffsetCached(gfxShellClass, "bGamePaused")) &
            offsetResolver->GetBoolMaskCached(gfxShellClass, "bGamePaused")) != 0;
}
