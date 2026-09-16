#include "boost_pad.hpp"

#include <mutex>
#include <unordered_map>

#include "core/nerine_memory.hpp"
#include "offset_resolver.hpp"

struct BoostPadStaticData
{
    FVector location;
    float respawnDelay;
    float boostAmount;
};

static std::mutex g_staticCacheMutex;
static std::unordered_map<uint64_t, BoostPadStaticData> g_staticCache;

static BoostPadStaticData GetStaticData(uint64_t address)
{
    std::lock_guard<std::mutex> lock(g_staticCacheMutex);
    std::unordered_map<uint64_t, BoostPadStaticData>::iterator entry = g_staticCache.find(address);
    if (entry != g_staticCache.end())
    {
        return entry->second;
    }

    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t actorClass            = offsetResolver->GetClassAddress("Actor");
    uint64_t pickupClass           = offsetResolver->GetClassAddress("VehiclePickup_TA");
    uint64_t boostPickupClass      = offsetResolver->GetClassAddress("VehiclePickup_Boost_TA");

    BoostPadStaticData data;
    data.location     = ReadMemory<FVector>(address + offsetResolver->GetPropertyOffsetCached(actorClass, "Location"));
    data.respawnDelay = ReadMemory<float>(address + offsetResolver->GetPropertyOffsetCached(pickupClass, "RespawnDelay"));
    data.boostAmount  = ReadMemory<float>(address + offsetResolver->GetPropertyOffsetCached(boostPickupClass, "BoostAmount"));

    g_staticCache.emplace(address, data);
    return data;
}

BoostPad::BoostPad()
{}

BoostPad::BoostPad(uint64_t address)
{
    m_address = address;
}

FVector BoostPad::GetLocation() const
{
    return GetStaticData(m_address).location;
}

uint8_t BoostPad::GetPickupState() const
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t pickupClass           = offsetResolver->GetClassAddress("VehiclePickup_TA");
    return ReadMemory<uint8_t>(m_address + offsetResolver->GetPropertyOffsetCached(pickupClass, "NewReplicatedPickupData") +
                               offsetResolver->GetStructPropertyOffsetCached(pickupClass, "NewReplicatedPickupData", "PickedUp"));
}

float BoostPad::GetRespawnDelay() const
{
    return GetStaticData(m_address).respawnDelay;
}

float BoostPad::GetBoostAmount() const
{
    return GetStaticData(m_address).boostAmount;
}

void BoostPad::ClearStaticCache()
{
    std::lock_guard<std::mutex> lock(g_staticCacheMutex);
    g_staticCache.clear();
}
