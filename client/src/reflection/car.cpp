#include "car.hpp"

#include <mutex>
#include <unordered_map>
#include <vector>

#include "core/nerine_memory.hpp"
#include "offset_resolver.hpp"

constexpr int kMaxPlausibleWheelCount = 8;

struct CarStaticData
{
    bool valid;
    int team;
    std::string playerName;
    FVector collisionExtent;
    FVector collisionOffset;
    int wheelCount;
    std::vector<FVector> wheelRestPositions;
    std::vector<float> wheelRadii;
};

static std::mutex g_staticCacheMutex;
static std::unordered_map<uint64_t, CarStaticData> g_staticCache;

static CarStaticData ReadStaticData(uint64_t address)
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver->GetClassAddress("Vehicle_TA");
    uint64_t vehicleSimClass       = offsetResolver->GetClassAddress("VehicleSim_TA");
    uint64_t wheelClass            = offsetResolver->GetClassAddress("Wheel_TA");
    uint64_t priClass              = offsetResolver->GetClassAddress("PRI_TA");
    uint64_t teamClass             = offsetResolver->GetClassAddress("Team_TA");
    uint64_t pri                   = ReadMemory<uint64_t>(address + offsetResolver->GetPropertyOffsetCached(vehicleClass, "PRI"));
    uint64_t team                  = ReadMemory<uint64_t>(pri + offsetResolver->GetPropertyOffsetCached(priClass, "Team"));
    uint64_t vehicleSim            = ReadMemory<uint64_t>(address + offsetResolver->GetPropertyOffsetCached(vehicleClass, "VehicleSim"));
    uint64_t wheelsOffset          = offsetResolver->GetPropertyOffsetCached(vehicleSimClass, "Wheels");
    uint64_t wheels                = ReadMemory<uint64_t>(vehicleSim + wheelsOffset);

    CarStaticData data;
    data.valid           = false;
    data.team            = ReadMemory<int>(team + offsetResolver->GetPropertyOffsetCached(teamClass, "TeamIndex"));
    data.playerName      = ReadFString(pri + offsetResolver->GetPropertyOffsetCached(priClass, "PlayerName"));
    data.collisionExtent = ReadMemory<FVector>(address + offsetResolver->GetPropertyOffsetCached(vehicleClass, "LocalCollisionExtent"));
    data.collisionOffset = ReadMemory<FVector>(address + offsetResolver->GetPropertyOffsetCached(vehicleClass, "LocalCollisionOffset"));
    data.wheelCount      = ReadMemory<int>(vehicleSim + wheelsOffset + sizeof(uint64_t));

    if (data.wheelCount < 0 || data.wheelCount > kMaxPlausibleWheelCount)
    {
        data.wheelCount = 0;
        return data;
    }

    data.valid = pri != 0 && team != 0 && !data.playerName.empty();

    for (int i = 0; i < data.wheelCount; i++)
    {
        uint64_t wheel = ReadMemory<uint64_t>(wheels + i * sizeof(uint64_t));
        data.wheelRestPositions.push_back(ReadMemory<FVector>(wheel + offsetResolver->GetPropertyOffsetCached(wheelClass, "LocalRestPosition")));
        data.wheelRadii.push_back(ReadMemory<float>(wheel + offsetResolver->GetPropertyOffsetCached(wheelClass, "WheelRadius")));
    }

    return data;
}

static CarStaticData GetStaticData(uint64_t address)
{
    std::lock_guard<std::mutex> lock(g_staticCacheMutex);
    std::unordered_map<uint64_t, CarStaticData>::iterator entry = g_staticCache.find(address);
    if (entry != g_staticCache.end())
    {
        return entry->second;
    }

    CarStaticData data = ReadStaticData(address);
    if (data.valid)
    {
        g_staticCache.emplace(address, data);
    }

    return data;
}

Car::Car()
{}

Car::Car(uint64_t address)
{
    m_address = address;
}

FVector Car::GetLocation() const
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t rigidBodyActorClass   = offsetResolver->GetClassAddress("RBActor_TA");
    return ReadMemory<FVector>(m_address + offsetResolver->GetPropertyOffsetCached(rigidBodyActorClass, "RBState") +
                               offsetResolver->GetStructPropertyOffsetCached(rigidBodyActorClass, "RBState", "Location"));
}

FVector Car::GetLinearVelocity() const
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t rigidBodyActorClass   = offsetResolver->GetClassAddress("RBActor_TA");
    return ReadMemory<FVector>(m_address + offsetResolver->GetPropertyOffsetCached(rigidBodyActorClass, "RBState") +
                               offsetResolver->GetStructPropertyOffsetCached(rigidBodyActorClass, "RBState", "LinearVelocity"));
}

FVector Car::GetAngularVelocity() const
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t rigidBodyActorClass   = offsetResolver->GetClassAddress("RBActor_TA");
    return ReadMemory<FVector>(m_address + offsetResolver->GetPropertyOffsetCached(rigidBodyActorClass, "RBState") +
                               offsetResolver->GetStructPropertyOffsetCached(rigidBodyActorClass, "RBState", "AngularVelocity"));
}

FQuat Car::GetRotation() const
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t rigidBodyActorClass   = offsetResolver->GetClassAddress("RBActor_TA");
    return ReadMemory<FQuat>(m_address + offsetResolver->GetPropertyOffsetCached(rigidBodyActorClass, "RBState") +
                             offsetResolver->GetStructPropertyOffsetCached(rigidBodyActorClass, "RBState", "Quaternion"));
}

FMatrix Car::GetRenderTransform() const
{
    OffsetResolver *offsetResolver   = OffsetResolver::GetInstance();
    uint64_t vehicleClass            = offsetResolver->GetClassAddress("Vehicle_TA");
    uint64_t primitiveComponentClass = offsetResolver->GetClassAddress("PrimitiveComponent");
    uint64_t carMesh                 = ReadMemory<uint64_t>(m_address + offsetResolver->GetPropertyOffsetCached(vehicleClass, "CarMesh"));
    return ReadMemory<FMatrix>(carMesh + offsetResolver->GetPropertyOffsetCached(primitiveComponentClass, "LocalToWorld"));
}

FVector Car::GetLocalCollisionOffset() const
{
    return GetStaticData(m_address).collisionOffset;
}

FVector Car::GetLocalCollisionExtent() const
{
    return GetStaticData(m_address).collisionExtent;
}

int Car::GetWheelCount() const
{
    return GetStaticData(m_address).wheelCount;
}

FVector Car::GetWheelLocalRestPosition(int wheelIndex) const
{
    return GetStaticData(m_address).wheelRestPositions[wheelIndex];
}

float Car::GetWheelRadius(int wheelIndex) const
{
    return GetStaticData(m_address).wheelRadii[wheelIndex];
}

float Car::GetBoostAmount() const
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver->GetClassAddress("Vehicle_TA");
    uint64_t boostComponentClass   = offsetResolver->GetClassAddress("CarComponent_Boost_TA");
    uint64_t boostComponent        = ReadMemory<uint64_t>(m_address + offsetResolver->GetPropertyOffsetCached(vehicleClass, "BoostComponent"));
    return ReadMemory<float>(boostComponent + offsetResolver->GetPropertyOffsetCached(boostComponentClass, "CurrentBoostAmount")) * 100.0f /
           ReadMemory<float>(boostComponent + offsetResolver->GetPropertyOffsetCached(boostComponentClass, "MaxBoostAmount"));
}

int Car::GetTeam() const
{
    return GetStaticData(m_address).team;
}

bool Car::IsOnGround() const
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver->GetClassAddress("Vehicle_TA");
    return (ReadMemory<uint32_t>(m_address + offsetResolver->GetPropertyOffsetCached(vehicleClass, "bOnGround")) &
            offsetResolver->GetBoolMaskCached(vehicleClass, "bOnGround")) != 0;
}

bool Car::HasFlip() const
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver->GetClassAddress("Vehicle_TA");
    return (ReadMemory<uint32_t>(m_address + offsetResolver->GetPropertyOffsetCached(vehicleClass, "bOnGround")) &
            offsetResolver->GetBoolMaskCached(vehicleClass, "bDoubleJumped")) == 0;
}

bool Car::IsDemolished() const
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t actorClass            = offsetResolver->GetClassAddress("Actor");
    return (ReadMemory<uint32_t>(m_address + offsetResolver->GetPropertyOffsetCached(actorClass, "bHidden")) &
            offsetResolver->GetBoolMaskCached(actorClass, "bHidden")) != 0;
}

bool Car::IsSupersonic() const
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver->GetClassAddress("Vehicle_TA");
    return (ReadMemory<uint32_t>(m_address + offsetResolver->GetPropertyOffsetCached(vehicleClass, "bSupersonic")) &
            offsetResolver->GetBoolMaskCached(vehicleClass, "bSupersonic")) != 0;
}

std::string Car::GetPlayerName() const
{
    return GetStaticData(m_address).playerName;
}

bool Car::HasStaticData() const
{
    return GetStaticData(m_address).valid;
}

void Car::ClearStaticCache()
{
    std::lock_guard<std::mutex> lock(g_staticCacheMutex);
    g_staticCache.clear();
}
