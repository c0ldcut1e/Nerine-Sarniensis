#include "match.hpp"

#include "nerine_memory.hpp"
#include "offset_resolver.hpp"
#include "world_scanner.hpp"

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
    if (!gameEvent)
    {
        return false;
    }

    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t soccarGameEventClass  = offsetResolver.GetClassAddress("GameEvent_Soccar_TA");
    return (ReadMemory<uint32_t>(gameEvent + offsetResolver.GetPropertyOffsetCached(soccarGameEventClass, "bRoundActive")) &
            offsetResolver.GetBoolMaskCached(soccarGameEventClass, "bRoundActive")) != 0;
}

bool Match::HasBallBeenHit()
{
    uint64_t gameEvent = WorldScanner::GetGameEventAddress();
    if (!gameEvent)
    {
        return false;
    }

    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t soccarGameEventClass  = offsetResolver.GetClassAddress("GameEvent_Soccar_TA");
    return (ReadMemory<uint32_t>(gameEvent + offsetResolver.GetPropertyOffsetCached(soccarGameEventClass, "bRoundActive")) &
            offsetResolver.GetBoolMaskCached(soccarGameEventClass, "bBallHasBeenHit")) != 0;
}
