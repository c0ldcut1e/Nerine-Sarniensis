#include "trajectory.hpp"

#include <algorithm>
#include <cmath>

#include "RocketSim.h"

static std::string g_collisionMeshesFolder = "/home/yuki/Games/Heroic/rocketleague/Binaries/Win64/collision_meshes";

constexpr float kTickRate         = 120.0f;
constexpr float kMaxSimulatedTime = 6.0f;
constexpr int kSampleInterval     = 2;

constexpr float kTouchVelocityDeviation = 50.0f;
constexpr float kMinTouchSpacing        = 75.0f;

constexpr float kZeroVelocityNudge = 0.001f;

constexpr float kMaxPlausiblePosition        = 10000.0f;
constexpr float kMaxPlausibleVelocity        = 6500.0f;
constexpr float kMaxPlausibleAngularVelocity = 100.0f;
constexpr float kMinGravityScale             = 0.05f;
constexpr float kMaxGravityScale             = 10.0f;
constexpr float kMaxWorldBounceScale         = 5.0f;
constexpr float kMaxWorldRestitution         = 0.95f;
constexpr float kMinBallRadius               = 50.0f;
constexpr float kMaxBallRadius               = 100.0f;
constexpr float kMaxBallSpeed                = 6500.0f;

static RocketSim::Vec ToRocketSimVec(const FVector &value)
{
    return RocketSim::Vec(value.X, value.Y, value.Z);
}

static FVector FromRocketSimVec(const RocketSim::Vec &value)
{
    return FVector{value.x, value.y, value.z};
}

static float Distance(const FVector &a, const FVector &b)
{
    float deltaX = a.X - b.X;
    float deltaY = a.Y - b.Y;
    float deltaZ = a.Z - b.Z;

    return sqrtf(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
}

static bool IsPlausibleVector(const FVector &value, float maxMagnitude)
{
    return std::isfinite(value.X) && std::isfinite(value.Y) && std::isfinite(value.Z) && fabsf(value.X) <= maxMagnitude &&
           fabsf(value.Y) <= maxMagnitude && fabsf(value.Z) <= maxMagnitude;
}

static bool IsPlausibleRocketSimVec(const RocketSim::Vec &value, float maxMagnitude)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) && fabsf(value.x) <= maxMagnitude &&
           fabsf(value.y) <= maxMagnitude && fabsf(value.z) <= maxMagnitude;
}

static RocketSim::Arena *CreateArena()
{
    RocketSim::Init(g_collisionMeshesFolder);

    return RocketSim::Arena::Create(RocketSim::GameMode::SOCCAR, {}, kTickRate);
}

const std::string &Trajectory::GetCollisionMeshesFolder()
{
    return g_collisionMeshesFolder;
}

void Trajectory::SetCollisionMeshesFolder(const std::string &folder)
{
    g_collisionMeshesFolder = folder;
}

static RocketSim::Arena *&GetArenaSlot()
{
    static RocketSim::Arena *arena = CreateArena();

    return arena;
}

static void RecreateArena()
{
    RocketSim::Arena *&slot = GetArenaSlot();

    delete slot;
    slot = CreateArena();
}

std::vector<FVector> Trajectory::Simulate(const Ball &ball, std::vector<FVector> &mapTouches, int maxTouches)
{
    RocketSim::Arena *arena = GetArenaSlot();
    BallPhysics physics     = ball.GetPhysics();
    FVector location        = ball.GetLocation();
    FVector linearVelocity  = ball.GetLinearVelocity();
    FVector angularVelocity = ball.GetAngularVelocity();

    mapTouches.clear();

    std::vector<FVector> path;
    path.push_back(location);

    if (!IsPlausibleVector(location, kMaxPlausiblePosition) || !IsPlausibleVector(linearVelocity, kMaxPlausibleVelocity) ||
        !IsPlausibleVector(angularVelocity, kMaxPlausibleAngularVelocity))
    {
        return path;
    }

    RocketSim::MutatorConfig mutatorConfig = RocketSim::MutatorConfig(RocketSim::GameMode::SOCCAR);
    if (std::isfinite(physics.GravityScale) && physics.GravityScale >= kMinGravityScale && physics.GravityScale <= kMaxGravityScale)
    {
        mutatorConfig.gravity.z = RocketSim::RLConst::GRAVITY_Z * physics.GravityScale;
    }

    if (std::isfinite(physics.WorldBounceScale) && physics.WorldBounceScale > 0.0f && physics.WorldBounceScale <= kMaxWorldBounceScale)
    {
        mutatorConfig.ballWorldRestitution = std::min(RocketSim::RLConst::BALL_RESTITUTION * physics.WorldBounceScale, kMaxWorldRestitution);
    }

    if (std::isfinite(physics.Radius) && physics.Radius >= kMinBallRadius && physics.Radius <= kMaxBallRadius)
    {
        mutatorConfig.ballRadius = physics.Radius;
    }

    if (std::isfinite(physics.MaxLinearSpeed) && physics.MaxLinearSpeed > 0.0f && physics.MaxLinearSpeed <= kMaxBallSpeed)
    {
        mutatorConfig.ballMaxSpeed = physics.MaxLinearSpeed;
    }

    arena->SetMutatorConfig(mutatorConfig);

    RocketSim::BallState state;
    state.pos    = ToRocketSimVec(location);
    state.vel    = ToRocketSimVec(linearVelocity);
    state.angVel = ToRocketSimVec(angularVelocity);
    if (state.vel.IsZero() && state.angVel.IsZero())
    {
        state.vel.z = -kZeroVelocityNudge;
    }

    arena->ball->SetState(state);

    RocketSim::Vec velocity = state.vel;
    int totalTicks          = (int) (kMaxSimulatedTime * kTickRate);

    for (int tick = 0; tick < totalTicks; tick++)
    {
        RocketSim::Vec expectedVelocity = velocity + RocketSim::Vec(0.0f, 0.0f, mutatorConfig.gravity.z * arena->tickTime);

        arena->Step(1);

        RocketSim::BallState afterState = arena->ball->GetState();
        velocity                        = afterState.vel;

        if (!IsPlausibleRocketSimVec(afterState.pos, kMaxPlausiblePosition) || !IsPlausibleRocketSimVec(velocity, kMaxPlausibleVelocity))
        {
            RecreateArena();
            return path;
        }

        if ((velocity - expectedVelocity).Length() > kTouchVelocityDeviation)
        {
            FVector touch = FromRocketSimVec(afterState.pos);

            if (mapTouches.empty() || Distance(touch, mapTouches.back()) > kMinTouchSpacing)
            {
                mapTouches.push_back(touch);
            }

            if ((int) mapTouches.size() >= maxTouches)
            {
                path.push_back(touch);
                return path;
            }
        }

        if ((tick + 1) % kSampleInterval == 0)
        {
            path.push_back(FromRocketSimVec(afterState.pos));
        }
    }

    FVector finalPosition = FromRocketSimVec(arena->ball->GetState().pos);
    if (path.back().X != finalPosition.X || path.back().Y != finalPosition.Y || path.back().Z != finalPosition.Z)
    {
        path.push_back(finalPosition);
    }

    return path;
}
