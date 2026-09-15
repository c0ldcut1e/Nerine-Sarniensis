#include "car.hpp"

#include "nerine_memory.hpp"
#include "offset_resolver.hpp"

Car::Car()
{}

Car::Car(uint64_t address)
{
    m_address = address;
}

FVector Car::GetLocation() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t rigidBodyActorClass   = offsetResolver.GetClassAddress("RBActor_TA");
    return ReadMemory<FVector>(m_address + offsetResolver.GetPropertyOffsetCached(rigidBodyActorClass, "RBState") +
                               offsetResolver.GetStructPropertyOffsetCached(rigidBodyActorClass, "RBState", "Location"));
}

FVector Car::GetLinearVelocity() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t rigidBodyActorClass   = offsetResolver.GetClassAddress("RBActor_TA");
    return ReadMemory<FVector>(m_address + offsetResolver.GetPropertyOffsetCached(rigidBodyActorClass, "RBState") +
                               offsetResolver.GetStructPropertyOffsetCached(rigidBodyActorClass, "RBState", "LinearVelocity"));
}

FVector Car::GetAngularVelocity() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t rigidBodyActorClass   = offsetResolver.GetClassAddress("RBActor_TA");
    return ReadMemory<FVector>(m_address + offsetResolver.GetPropertyOffsetCached(rigidBodyActorClass, "RBState") +
                               offsetResolver.GetStructPropertyOffsetCached(rigidBodyActorClass, "RBState", "AngularVelocity"));
}

FQuat Car::GetRotation() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t rigidBodyActorClass   = offsetResolver.GetClassAddress("RBActor_TA");
    return ReadMemory<FQuat>(m_address + offsetResolver.GetPropertyOffsetCached(rigidBodyActorClass, "RBState") +
                             offsetResolver.GetStructPropertyOffsetCached(rigidBodyActorClass, "RBState", "Quaternion"));
}

FMatrix Car::GetRenderTransform() const
{
    OffsetResolver &offsetResolver   = OffsetResolver::GetInstance();
    uint64_t vehicleClass            = offsetResolver.GetClassAddress("Vehicle_TA");
    uint64_t primitiveComponentClass = offsetResolver.GetClassAddress("PrimitiveComponent");
    uint64_t carMesh                 = ReadMemory<uint64_t>(m_address + offsetResolver.GetPropertyOffsetCached(vehicleClass, "CarMesh"));
    return ReadMemory<FMatrix>(carMesh + offsetResolver.GetPropertyOffsetCached(primitiveComponentClass, "LocalToWorld"));
}

FVector Car::GetLocalCollisionOffset() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver.GetClassAddress("Vehicle_TA");
    return ReadMemory<FVector>(m_address + offsetResolver.GetPropertyOffsetCached(vehicleClass, "LocalCollisionOffset"));
}

FVector Car::GetLocalCollisionExtent() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver.GetClassAddress("Vehicle_TA");
    return ReadMemory<FVector>(m_address + offsetResolver.GetPropertyOffsetCached(vehicleClass, "LocalCollisionExtent"));
}

int Car::GetWheelCount() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver.GetClassAddress("Vehicle_TA");
    uint64_t vehicleSimClass       = offsetResolver.GetClassAddress("VehicleSim_TA");
    uint64_t vehicleSim            = ReadMemory<uint64_t>(m_address + offsetResolver.GetPropertyOffsetCached(vehicleClass, "VehicleSim"));
    return ReadMemory<int>(vehicleSim + offsetResolver.GetPropertyOffsetCached(vehicleSimClass, "Wheels") + sizeof(uint64_t));
}

FVector Car::GetWheelLocalRestPosition(int wheelIndex) const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t wheelClass            = offsetResolver.GetClassAddress("Wheel_TA");
    return ReadMemory<FVector>(GetWheelAddress(wheelIndex) + offsetResolver.GetPropertyOffsetCached(wheelClass, "LocalRestPosition"));
}

float Car::GetWheelRadius(int wheelIndex) const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t wheelClass            = offsetResolver.GetClassAddress("Wheel_TA");
    return ReadMemory<float>(GetWheelAddress(wheelIndex) + offsetResolver.GetPropertyOffsetCached(wheelClass, "WheelRadius"));
}

float Car::GetBoostAmount() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver.GetClassAddress("Vehicle_TA");
    uint64_t boostComponentClass   = offsetResolver.GetClassAddress("CarComponent_Boost_TA");
    uint64_t boostComponent        = ReadMemory<uint64_t>(m_address + offsetResolver.GetPropertyOffsetCached(vehicleClass, "BoostComponent"));
    return ReadMemory<float>(boostComponent + offsetResolver.GetPropertyOffsetCached(boostComponentClass, "CurrentBoostAmount")) * 100.0f /
           ReadMemory<float>(boostComponent + offsetResolver.GetPropertyOffsetCached(boostComponentClass, "MaxBoostAmount"));
}

int Car::GetTeam() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver.GetClassAddress("Vehicle_TA");
    uint64_t priClass              = offsetResolver.GetClassAddress("PRI_TA");
    uint64_t teamClass             = offsetResolver.GetClassAddress("Team_TA");
    uint64_t pri                   = ReadMemory<uint64_t>(m_address + offsetResolver.GetPropertyOffsetCached(vehicleClass, "PRI"));
    uint64_t team                  = ReadMemory<uint64_t>(pri + offsetResolver.GetPropertyOffsetCached(priClass, "Team"));
    return ReadMemory<int>(team + offsetResolver.GetPropertyOffsetCached(teamClass, "TeamIndex"));
}

bool Car::IsOnGround() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver.GetClassAddress("Vehicle_TA");
    return (ReadMemory<uint32_t>(m_address + offsetResolver.GetPropertyOffsetCached(vehicleClass, "bOnGround")) &
            offsetResolver.GetBoolMaskCached(vehicleClass, "bOnGround")) != 0;
}

bool Car::HasFlip() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver.GetClassAddress("Vehicle_TA");
    return (ReadMemory<uint32_t>(m_address + offsetResolver.GetPropertyOffsetCached(vehicleClass, "bOnGround")) &
            offsetResolver.GetBoolMaskCached(vehicleClass, "bDoubleJumped")) == 0;
}

bool Car::IsDemolished() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t actorClass            = offsetResolver.GetClassAddress("Actor");
    return (ReadMemory<uint32_t>(m_address + offsetResolver.GetPropertyOffsetCached(actorClass, "bHidden")) &
            offsetResolver.GetBoolMaskCached(actorClass, "bHidden")) != 0;
}

uint64_t Car::GetWheelAddress(int wheelIndex) const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t vehicleClass          = offsetResolver.GetClassAddress("Vehicle_TA");
    uint64_t vehicleSimClass       = offsetResolver.GetClassAddress("VehicleSim_TA");
    uint64_t vehicleSim            = ReadMemory<uint64_t>(m_address + offsetResolver.GetPropertyOffsetCached(vehicleClass, "VehicleSim"));
    uint64_t wheels                = ReadMemory<uint64_t>(vehicleSim + offsetResolver.GetPropertyOffsetCached(vehicleSimClass, "Wheels"));
    return ReadMemory<uint64_t>(wheels + wheelIndex * sizeof(uint64_t));
}
