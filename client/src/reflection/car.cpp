#include "car.hpp"

#include "nerine_memory.hpp"
#include "offsets.hpp"

Car::Car()
{}

Car::Car(uint64_t address)
{
    m_address = address;
}

FVector Car::GetLocation() const
{
    return ReadMemory<FVector>(m_address + kRigidBodyStateOffset + kRigidBodyLocationOffset);
}

FVector Car::GetLinearVelocity() const
{
    return ReadMemory<FVector>(m_address + kRigidBodyStateOffset + kRigidBodyLinearVelocityOffset);
}

FVector Car::GetAngularVelocity() const
{
    return ReadMemory<FVector>(m_address + kRigidBodyStateOffset + kRigidBodyAngularVelocityOffset);
}

FQuat Car::GetRotation() const
{
    return ReadMemory<FQuat>(m_address + kRigidBodyStateOffset + kRigidBodyRotationOffset);
}

FMatrix Car::GetRenderTransform() const
{
    uint64_t carMesh = ReadMemory<uint64_t>(m_address + kCarMeshOffset);

    return ReadMemory<FMatrix>(carMesh + kPrimitiveComponentLocalToWorldOffset);
}

FVector Car::GetLocalCollisionOffset() const
{
    return ReadMemory<FVector>(m_address + kCarLocalCollisionOffset);
}

FVector Car::GetLocalCollisionExtent() const
{
    return ReadMemory<FVector>(m_address + kCarLocalCollisionExtentOffset);
}

int Car::GetWheelCount() const
{
    uint64_t vehicleSim = ReadMemory<uint64_t>(m_address + kCarVehicleSimOffset);

    return ReadMemory<int>(vehicleSim + kVehicleSimWheelsOffset + sizeof(uint64_t));
}

FVector Car::GetWheelLocalRestPosition(int wheelIndex) const
{
    return ReadMemory<FVector>(GetWheelAddress(wheelIndex) + kWheelLocalRestPositionOffset);
}

float Car::GetWheelRadius(int wheelIndex) const
{
    return ReadMemory<float>(GetWheelAddress(wheelIndex) + kWheelRadiusOffset);
}

float Car::GetBoostAmount() const
{
    uint64_t boostComponent = ReadMemory<uint64_t>(m_address + kCarBoostComponentOffset);

    return ReadMemory<float>(boostComponent + kBoostCurrentAmountOffset) * 100.0f / ReadMemory<float>(boostComponent + kBoostMaxAmountOffset);
}

int Car::GetTeam() const
{
    uint64_t pri  = ReadMemory<uint64_t>(m_address + kCarPriOffset);
    uint64_t team = ReadMemory<uint64_t>(pri + kPriTeamOffset);

    return ReadMemory<int>(team + kTeamIndexOffset);
}

bool Car::IsOnGround() const
{
    return (ReadMemory<uint32_t>(m_address + kCarStateFlagsOffset) & kCarOnGroundFlag) != 0;
}

bool Car::HasFlip() const
{
    return (ReadMemory<uint32_t>(m_address + kCarStateFlagsOffset) & kCarDoubleJumpedFlag) == 0;
}

bool Car::IsDemolished() const
{
    return (ReadMemory<uint32_t>(m_address + kActorHiddenOffset) & kActorHiddenFlag) != 0;
}

uint64_t Car::GetWheelAddress(int wheelIndex) const
{
    uint64_t vehicleSim = ReadMemory<uint64_t>(m_address + kCarVehicleSimOffset);
    uint64_t wheels     = ReadMemory<uint64_t>(vehicleSim + kVehicleSimWheelsOffset);

    return ReadMemory<uint64_t>(wheels + wheelIndex * sizeof(uint64_t));
}
