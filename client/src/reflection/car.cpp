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
    return ReadMemory<FVector>(m_address + g_rigidBodyStateOffset + g_rigidBodyLocationOffset);
}

FVector Car::GetLinearVelocity() const
{
    return ReadMemory<FVector>(m_address + g_rigidBodyStateOffset + g_rigidBodyLinearVelocityOffset);
}

FVector Car::GetAngularVelocity() const
{
    return ReadMemory<FVector>(m_address + g_rigidBodyStateOffset + g_rigidBodyAngularVelocityOffset);
}

FQuat Car::GetRotation() const
{
    return ReadMemory<FQuat>(m_address + g_rigidBodyStateOffset + g_rigidBodyRotationOffset);
}

FMatrix Car::GetRenderTransform() const
{
    uint64_t carMesh = ReadMemory<uint64_t>(m_address + g_carMeshOffset);

    return ReadMemory<FMatrix>(carMesh + g_primitiveComponentLocalToWorldOffset);
}

FVector Car::GetLocalCollisionOffset() const
{
    return ReadMemory<FVector>(m_address + g_carLocalCollisionOffset);
}

FVector Car::GetLocalCollisionExtent() const
{
    return ReadMemory<FVector>(m_address + g_carLocalCollisionExtentOffset);
}

int Car::GetWheelCount() const
{
    uint64_t vehicleSim = ReadMemory<uint64_t>(m_address + g_carVehicleSimOffset);

    return ReadMemory<int>(vehicleSim + g_vehicleSimWheelsOffset + sizeof(uint64_t));
}

FVector Car::GetWheelLocalRestPosition(int wheelIndex) const
{
    return ReadMemory<FVector>(GetWheelAddress(wheelIndex) + g_wheelLocalRestPositionOffset);
}

float Car::GetWheelRadius(int wheelIndex) const
{
    return ReadMemory<float>(GetWheelAddress(wheelIndex) + g_wheelRadiusOffset);
}

float Car::GetBoostAmount() const
{
    uint64_t boostComponent = ReadMemory<uint64_t>(m_address + g_carBoostComponentOffset);

    return ReadMemory<float>(boostComponent + g_boostCurrentAmountOffset) * 100.0f / ReadMemory<float>(boostComponent + g_boostMaxAmountOffset);
}

int Car::GetTeam() const
{
    uint64_t pri  = ReadMemory<uint64_t>(m_address + g_carPriOffset);
    uint64_t team = ReadMemory<uint64_t>(pri + g_priTeamOffset);

    return ReadMemory<int>(team + g_teamIndexOffset);
}

bool Car::IsOnGround() const
{
    return (ReadMemory<uint32_t>(m_address + g_carStateFlagsOffset) & g_carOnGroundFlag) != 0;
}

bool Car::HasFlip() const
{
    return (ReadMemory<uint32_t>(m_address + g_carStateFlagsOffset) & g_carDoubleJumpedFlag) == 0;
}

bool Car::IsDemolished() const
{
    return (ReadMemory<uint32_t>(m_address + g_actorHiddenOffset) & g_actorHiddenFlag) != 0;
}

uint64_t Car::GetWheelAddress(int wheelIndex) const
{
    uint64_t vehicleSim = ReadMemory<uint64_t>(m_address + g_carVehicleSimOffset);
    uint64_t wheels     = ReadMemory<uint64_t>(vehicleSim + g_vehicleSimWheelsOffset);

    return ReadMemory<uint64_t>(wheels + wheelIndex * sizeof(uint64_t));
}
