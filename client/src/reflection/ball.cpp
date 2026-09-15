#include "ball.hpp"

#include "nerine_memory.hpp"
#include "offsets.hpp"
#include "world_scanner.hpp"

Ball::Ball()
{}

Ball::Ball(uint64_t address)
{
    m_address = address;
}

FVector Ball::GetLocation() const
{
    return ReadMemory<FVector>(m_address + g_rigidBodyStateOffset + g_rigidBodyLocationOffset);
}

FVector Ball::GetLinearVelocity() const
{
    return ReadMemory<FVector>(m_address + g_rigidBodyStateOffset + g_rigidBodyLinearVelocityOffset);
}

FVector Ball::GetAngularVelocity() const
{
    return ReadMemory<FVector>(m_address + g_rigidBodyStateOffset + g_rigidBodyAngularVelocityOffset);
}

BallPhysics Ball::GetPhysics() const
{
    BallPhysics physics;

    physics.MagnusCoefficient      = ReadMemory<FVector>(m_address + g_ballMagnusCoefficientOffset);
    physics.AirResistance          = ReadMemory<FVector>(m_address + g_ballAirResistanceOffset);
    physics.MinResistanceVelocity  = ReadMemory<FVector>(m_address + g_ballMinResistanceVelocityOffset);
    physics.MagnusMinSpeed         = ReadMemory<float>(m_address + g_ballMagnusMinSpeedOffset);
    physics.AngularVelocityDamping = ReadMemory<float>(m_address + g_ballAngularVelocityDampingOffset);
    physics.Radius                 = ReadMemory<float>(m_address + g_ballRadiusOffset);
    physics.WorldBounceScale       = ReadMemory<float>(m_address + g_ballWorldBounceScaleOffset);
    physics.GravityScale           = ReadMemory<float>(m_address + g_ballGravityScaleOffset);
    physics.MaxLinearSpeed =
            ReadMemory<float>(m_address + g_rigidBodyMaxLinearSpeedOffset) * ReadMemory<float>(m_address + g_ballMaxLinearSpeedScaleOffset);
    physics.MaxAngularSpeed = ReadMemory<float>(m_address + g_rigidBodyMaxAngularSpeedOffset);
    physics.RollingFriction = ReadMemory<float>(m_address + g_rigidBodyRollingFrictionOffset);

    return physics;
}

std::vector<FPredictedPosition> Ball::GetPredictedPositions() const
{
    std::vector<FPredictedPosition> predictions;
    if (!IsValidAddress())
    {
        return predictions;
    }

    uint64_t arrayAddress = m_address + g_ballPredictedPositionsOffset;
    uint64_t data         = ReadMemory<uint64_t>(arrayAddress);
    int count             = ReadMemory<int>(arrayAddress + 0x8);
    int capacity          = ReadMemory<int>(arrayAddress + 0xC);
    if (!data || count <= 0 || count > 4096 || capacity < count || capacity > 4096)
    {
        return predictions;
    }

    return ReadMemoryArray<FPredictedPosition>(data, (size_t) count);
}

Ball Ball::Find()
{
    return Ball(WorldScanner::GetBallAddress());
}
