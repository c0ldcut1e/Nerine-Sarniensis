#include "boost_pad.hpp"

#include "nerine_memory.hpp"
#include "offsets.hpp"

BoostPad::BoostPad()
{}

BoostPad::BoostPad(uint64_t address)
{
    m_address = address;
}

FVector BoostPad::GetLocation() const
{
    return ReadMemory<FVector>(m_address + g_actorLocationOffset);
}

uint8_t BoostPad::GetPickupState() const
{
    return ReadMemory<uint8_t>(m_address + g_pickupPickedUpOffset);
}

float BoostPad::GetRespawnDelay() const
{
    return ReadMemory<float>(m_address + g_pickupRespawnDelayOffset);
}
