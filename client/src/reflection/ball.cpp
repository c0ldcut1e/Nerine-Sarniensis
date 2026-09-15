#include "ball.hpp"

#include "nerine_memory.hpp"
#include "offset_resolver.hpp"
#include "world_scanner.hpp"

Ball::Ball()
{}

Ball::Ball(uint64_t address)
{
    m_address = address;
}

FVector Ball::GetLocation() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t rigidBodyActorClass   = offsetResolver.GetClassAddress("RBActor_TA");
    return ReadMemory<FVector>(m_address + offsetResolver.GetPropertyOffsetCached(rigidBodyActorClass, "RBState") +
                               offsetResolver.GetStructPropertyOffsetCached(rigidBodyActorClass, "RBState", "Location"));
}

FVector Ball::GetLinearVelocity() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t rigidBodyActorClass   = offsetResolver.GetClassAddress("RBActor_TA");
    return ReadMemory<FVector>(m_address + offsetResolver.GetPropertyOffsetCached(rigidBodyActorClass, "RBState") +
                               offsetResolver.GetStructPropertyOffsetCached(rigidBodyActorClass, "RBState", "LinearVelocity"));
}

FVector Ball::GetAngularVelocity() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t rigidBodyActorClass   = offsetResolver.GetClassAddress("RBActor_TA");
    return ReadMemory<FVector>(m_address + offsetResolver.GetPropertyOffsetCached(rigidBodyActorClass, "RBState") +
                               offsetResolver.GetStructPropertyOffsetCached(rigidBodyActorClass, "RBState", "AngularVelocity"));
}

BallPhysics Ball::GetPhysics() const
{
    BallPhysics physics;
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t ballClass             = offsetResolver.GetClassAddress("Ball_TA");
    uint64_t rigidBodyActorClass   = offsetResolver.GetClassAddress("RBActor_TA");
    physics.MagnusCoefficient      = ReadMemory<FVector>(m_address + offsetResolver.GetPropertyOffsetCached(ballClass, "MagnusCoefficient"));
    physics.AirResistance          = ReadMemory<FVector>(m_address + offsetResolver.GetPropertyOffsetCached(ballClass, "AirResistance"));
    physics.MinResistanceVelocity  = ReadMemory<FVector>(m_address + offsetResolver.GetPropertyOffsetCached(ballClass, "MinResistanceVelocity"));
    physics.MagnusMinSpeed         = ReadMemory<float>(m_address + offsetResolver.GetPropertyOffsetCached(ballClass, "MagnusMinSpeed"));
    physics.AngularVelocityDamping = ReadMemory<float>(m_address + offsetResolver.GetPropertyOffsetCached(ballClass, "AngularVelocityDamping"));
    physics.Radius                 = ReadMemory<float>(m_address + offsetResolver.GetPropertyOffsetCached(ballClass, "Radius"));
    physics.WorldBounceScale       = ReadMemory<float>(m_address + offsetResolver.GetPropertyOffsetCached(ballClass, "ReplicatedWorldBounceScale"));
    physics.GravityScale           = ReadMemory<float>(m_address + offsetResolver.GetPropertyOffsetCached(ballClass, "ReplicatedBallGravityScale"));
    physics.MaxLinearSpeed  = ReadMemory<float>(m_address + offsetResolver.GetPropertyOffsetCached(rigidBodyActorClass, "MaxLinearSpeed")) *
                              ReadMemory<float>(m_address + offsetResolver.GetPropertyOffsetCached(ballClass, "ReplicatedBallMaxLinearSpeedScale"));
    physics.MaxAngularSpeed = ReadMemory<float>(m_address + offsetResolver.GetPropertyOffsetCached(rigidBodyActorClass, "MaxAngularSpeed"));
    physics.RollingFriction = ReadMemory<float>(m_address + offsetResolver.GetPropertyOffsetCached(rigidBodyActorClass, "RollingFriction"));

    return physics;
}

std::vector<FPredictedPosition> Ball::GetPredictedPositions() const
{
    std::vector<FPredictedPosition> predictions;
    if (!IsValidAddress())
    {
        return predictions;
    }

    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t ballClass             = offsetResolver.GetClassAddress("Ball_TA");
    uint64_t arrayAddress          = m_address + offsetResolver.GetPropertyOffsetCached(ballClass, "PredictedPositions");
    uint64_t data                  = ReadMemory<uint64_t>(arrayAddress);
    int count                      = ReadMemory<int>(arrayAddress + 0x8);
    int capacity                   = ReadMemory<int>(arrayAddress + 0xC);
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
