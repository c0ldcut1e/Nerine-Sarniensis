#include "ball.hpp"

#include <fmt/base.h>

#include "nerine_memory.hpp"
#include "object_array.hpp"
#include "offsets.hpp"
#include "world_scanner.hpp"

constexpr uint64_t kPredictedPositionsOffset = 0x970;

Ball::Ball()
{}

Ball::Ball(uint64_t address)
{
    m_address = address;
}

FVector Ball::GetLocation() const
{
    return ReadMemory<FVector>(m_address + kRigidBodyStateOffset + kRigidBodyLocationOffset);
}

FVector Ball::GetLinearVelocity() const
{
    return ReadMemory<FVector>(m_address + kRigidBodyStateOffset + kRigidBodyLinearVelocityOffset);
}

FVector Ball::GetAngularVelocity() const
{
    return ReadMemory<FVector>(m_address + kRigidBodyStateOffset + kRigidBodyAngularVelocityOffset);
}

BallPhysics Ball::GetPhysics() const
{
    BallPhysics physics;

    physics.MagnusCoefficient      = ReadMemory<FVector>(m_address + kBallMagnusCoefficientOffset);
    physics.AirResistance          = ReadMemory<FVector>(m_address + kBallAirResistanceOffset);
    physics.MinResistanceVelocity  = ReadMemory<FVector>(m_address + kBallMinResistanceVelocityOffset);
    physics.MagnusMinSpeed         = ReadMemory<float>(m_address + kBallMagnusMinSpeedOffset);
    physics.AngularVelocityDamping = ReadMemory<float>(m_address + kBallAngularVelocityDampingOffset);
    physics.Radius                 = ReadMemory<float>(m_address + kBallRadiusOffset);
    physics.WorldBounceScale       = ReadMemory<float>(m_address + kBallWorldBounceScaleOffset);
    physics.GravityScale           = ReadMemory<float>(m_address + kBallGravityScaleOffset);
    physics.MaxLinearSpeed =
            ReadMemory<float>(m_address + kRigidBodyMaxLinearSpeedOffset) * ReadMemory<float>(m_address + kBallMaxLinearSpeedScaleOffset);
    physics.MaxAngularSpeed = ReadMemory<float>(m_address + kRigidBodyMaxAngularSpeedOffset);
    physics.RollingFriction = ReadMemory<float>(m_address + kRigidBodyRollingFrictionOffset);

    return physics;
}

std::vector<FPredictedPosition> Ball::GetPredictedPositions() const
{
    std::vector<FPredictedPosition> predictions;
    if (!IsValidAddress())
    {
        return predictions;
    }

    uint64_t arrayAddress = m_address + kPredictedPositionsOffset;
    uint64_t data         = ReadMemory<uint64_t>(arrayAddress);
    int32_t count         = ReadMemory<int32_t>(arrayAddress + 0x8);
    int32_t capacity      = ReadMemory<int32_t>(arrayAddress + 0xC);
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

void Ball::Debug()
{
    uint64_t ballClassAddress = ObjectArray::FindClassByName("Ball_TA");

    fmt::println("--- Ball::Debug ---");
    fmt::println("WorldScanner::GetBallAddress() = {:#x}", WorldScanner::GetBallAddress());
    fmt::println("WorldScanner::GetGameEventAddress() = {:#x}", WorldScanner::GetGameEventAddress());
    fmt::println("ballClassAddress = {:#x}", ballClassAddress);

    if (!ballClassAddress)
    {
        return;
    }

    RemoteObject classObject(ballClassAddress);
    fmt::println("class object name = '{}'", classObject.GetName());

    std::vector<uint64_t> addresses = ObjectArray::GetAllObjectAddresses();
    int validCount                  = 0;

    for (size_t i = 0; i < addresses.size(); i++)
    {
        RemoteObject object(addresses[i]);
        if (!object.IsValidInstanceOf(ballClassAddress))
        {
            continue;
        }

        validCount++;
        Ball candidate(object.GetAddress());
        FVector location = candidate.GetLocation();
        fmt::println("valid Ball_TA #{} at {:#x}, location ({:.1f}, {:.1f}, {:.1f})", validCount, object.GetAddress(), location.X, location.Y,
                     location.Z);
    }

    fmt::println("total valid Ball_TA instances = {}", validCount);
}
