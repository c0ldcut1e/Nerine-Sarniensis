#ifndef CAR_HPP
#define CAR_HPP

#include "SdkHeaders.hpp"
#include "remote_object.hpp"

class Car : public RemoteObject
{
public:
    Car();
    explicit Car(uint64_t address);

    FVector GetLocation() const;
    FVector GetLinearVelocity() const;
    FVector GetAngularVelocity() const;
    FQuat GetRotation() const;
    FMatrix GetRenderTransform() const;
    FVector GetLocalCollisionOffset() const;
    FVector GetLocalCollisionExtent() const;
    int GetWheelCount() const;
    FVector GetWheelLocalRestPosition(int wheelIndex) const;
    float GetWheelRadius(int wheelIndex) const;
    float GetBoostAmount() const;
    int GetTeam() const;
    bool IsOnGround() const;
    bool HasFlip() const;
    bool IsDemolished() const;

private:
    uint64_t GetWheelAddress(int wheelIndex) const;
};

#endif
