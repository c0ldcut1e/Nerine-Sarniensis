#include "boost_pad_list.hpp"

#include "world_scanner.hpp"

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
