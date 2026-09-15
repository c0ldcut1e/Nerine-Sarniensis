#include "boost_pad.hpp"

#include "nerine_memory.hpp"
#include "offset_resolver.hpp"

BoostPad::BoostPad()
{}

BoostPad::BoostPad(uint64_t address)
{
    m_address = address;
}

FVector BoostPad::GetLocation() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t actorClass            = offsetResolver.GetClassAddress("Actor");
    return ReadMemory<FVector>(m_address + offsetResolver.GetPropertyOffsetCached(actorClass, "Location"));
}

uint8_t BoostPad::GetPickupState() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t pickupClass           = offsetResolver.GetClassAddress("VehiclePickup_TA");
    return ReadMemory<uint8_t>(m_address + offsetResolver.GetPropertyOffsetCached(pickupClass, "NewReplicatedPickupData") +
                               offsetResolver.GetStructPropertyOffsetCached(pickupClass, "NewReplicatedPickupData", "PickedUp"));
}

float BoostPad::GetRespawnDelay() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t pickupClass           = offsetResolver.GetClassAddress("VehiclePickup_TA");
    return ReadMemory<float>(m_address + offsetResolver.GetPropertyOffsetCached(pickupClass, "RespawnDelay"));
}
