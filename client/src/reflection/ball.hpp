#ifndef BALL_HPP
#define BALL_HPP

#include <vector>

#include "SdkHeaders.hpp"
#include "remote_object.hpp"

struct BallPhysics
{
    FVector MagnusCoefficient;
    FVector AirResistance;
    FVector MinResistanceVelocity;
    float MagnusMinSpeed;
    float AngularVelocityDamping;
    float Radius;
    float WorldBounceScale;
    float GravityScale;
    float MaxLinearSpeed;
    float MaxAngularSpeed;
    float RollingFriction;
};

class Ball : public RemoteObject
{
public:
    Ball();
    explicit Ball(uint64_t address);

    FVector GetLocation() const;
    FVector GetLinearVelocity() const;
    FVector GetAngularVelocity() const;
    BallPhysics GetPhysics() const;

    std::vector<FPredictedPosition> GetPredictedPositions() const;

    static Ball Find();
};

#endif
