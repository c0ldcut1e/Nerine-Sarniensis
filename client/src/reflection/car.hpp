#ifndef CAR_HPP
#define CAR_HPP

#include <cstdint>
#include <string>

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
    bool IsSupersonic() const;
    std::string GetPlayerName() const;
    bool HasStaticData() const;

    static void ClearStaticCache();
};

#endif
