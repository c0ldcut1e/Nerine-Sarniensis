#include "boost_pad_list.hpp"

#include <cstdio>
#include <string>

#include "nerine_memory.hpp"
#include "object_array.hpp"
#include "world_scanner.hpp"

#include <fmt/base.h>

std::vector<BoostPad> BoostPadList::GetAll()
{
    std::vector<uint64_t> addresses = WorldScanner::GetBoostPadAddresses();
    std::vector<BoostPad> pads;
    pads.reserve(addresses.size());

    for (size_t i = 0; i < addresses.size(); i++)
    {
        pads.push_back(BoostPad(addresses[i]));
    }

    return pads;
}

void BoostPadList::Debug()
{
    constexpr uint64_t kDumpStart   = 0x260;
    constexpr size_t kDumpByteCount = 0x90;

    uint64_t boostPadClassAddress = ObjectArray::FindClassByName("VehiclePickup_Boost_TA");
    std::vector<BoostPad> pads    = GetAll();

    fmt::println("--- BoostPadList::Debug ---");
    fmt::println("boostPadClassAddress = {:#x}", boostPadClassAddress);
    fmt::println("pad count = {}", pads.size());

    for (size_t i = 0; i < pads.size(); i++)
    {
        FVector location = pads[i].GetLocation();
        fmt::println("pad #{} at {:#x}, location ({:.0f}, {:.0f}, {:.0f}), PickupState={}, RespawnDelay={:.2f}", i, pads[i].GetAddress(), location.X,
                     location.Y, location.Z, pads[i].GetPickupState(), pads[i].GetRespawnDelay());

        std::vector<uint8_t> bytes = ReadMemoryArray<uint8_t>(pads[i].GetAddress() + kDumpStart, kDumpByteCount);
        for (size_t row = 0; row < bytes.size(); row += 16)
        {
            std::string line;
            char byteText[4];

            for (size_t col = 0; col < 16 && row + col < bytes.size(); col++)
            {
                snprintf(byteText, sizeof(byteText), "%02x ", bytes[row + col]);
                line += byteText;
            }

            fmt::println("  +{:#05x}: {}", kDumpStart + row, line);
        }
    }
}
