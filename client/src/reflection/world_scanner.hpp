#ifndef WORLD_SCANNER_HPP
#define WORLD_SCANNER_HPP

#include <cstdint>
#include <vector>

class WorldScanner
{
public:
    static uint64_t GetBallAddress();
    static uint64_t GetGameEventAddress();
    static uint64_t GetLocalPlayerAddress();
    static std::vector<uint64_t> GetBoostPadAddresses();
};

#endif
