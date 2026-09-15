#include "match.hpp"

#include "nerine_memory.hpp"
#include "offsets.hpp"
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
    return gameEvent && (ReadMemory<uint32_t>(gameEvent + g_gameEventFlagsOffset) & g_gameEventRoundActiveFlag) != 0;
}

bool Match::HasBallBeenHit()
{
    uint64_t gameEvent = WorldScanner::GetGameEventAddress();
    return gameEvent && (ReadMemory<uint32_t>(gameEvent + g_gameEventFlagsOffset) & g_gameEventBallHasBeenHitFlag) != 0;
}
