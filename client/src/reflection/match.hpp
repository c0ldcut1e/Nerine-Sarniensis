#ifndef MATCH_HPP
#define MATCH_HPP

#include <cstdint>

class Match
{
public:
    static uint64_t GetGameEvent();
    static bool IsActive();
    static bool IsRoundActive();
    static bool HasBallBeenHit();
};

#endif
