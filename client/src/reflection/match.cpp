#include "match.hpp"

#include "nerine_memory.hpp"
#include "object_array.hpp"
#include "offsets.hpp"
#include "remote_object.hpp"
#include "world_scanner.hpp"

#include <fmt/base.h>

uint64_t Match::GetGameEvent()
{
    return WorldScanner::GetGameEventAddress();
}

bool Match::IsActive()
{
    return WorldScanner::GetGameEventAddress() != 0;
}

bool Match::IsRoundActive()
{
    uint64_t gameEvent = WorldScanner::GetGameEventAddress();
    return gameEvent && (ReadMemory<uint32_t>(gameEvent + kGameEventFlagsOffset) & kGameEventRoundActiveFlag) != 0;
}

bool Match::HasBallBeenHit()
{
    uint64_t gameEvent = WorldScanner::GetGameEventAddress();
    return gameEvent && (ReadMemory<uint32_t>(gameEvent + kGameEventFlagsOffset) & kGameEventBallHasBeenHitFlag) != 0;
}

void Match::Debug()
{
    uint64_t gameEventClassAddress = ObjectArray::FindClassByName("GameEvent_Soccar_TA");

    fmt::println("--- Match::Debug ---");
    fmt::println("gameEventClassAddress = {:#x}", gameEventClassAddress);

    if (gameEventClassAddress)
    {
        RemoteObject classObject(gameEventClassAddress);
        fmt::println("class object name = '{}'", classObject.GetName());
    }

    fmt::println("game event = {:#x}, round active = {}, ball hit = {}", WorldScanner::GetGameEventAddress(), Match::IsRoundActive(),
                 Match::HasBallBeenHit());

    if (!gameEventClassAddress)
    {
        return;
    }

    std::vector<uint64_t> addresses = ObjectArray::GetAllObjectAddresses();
    int validCount                  = 0;

    for (size_t i = 0; i < addresses.size(); i++)
    {
        RemoteObject object(addresses[i]);
        if (!object.IsValidInstanceOf(gameEventClassAddress))
        {
            continue;
        }

        validCount++;
        fmt::println("valid GameEvent_Soccar_TA #{} at {:#x}", validCount, object.GetAddress());
    }

    fmt::println("total valid GameEvent_Soccar_TA instances = {}", validCount);
}
