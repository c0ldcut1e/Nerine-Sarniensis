#ifndef BOOST_PAD_HPP
#define BOOST_PAD_HPP

#include "SdkHeaders.hpp"
#include "remote_object.hpp"

class BoostPad : public RemoteObject
{
public:
    BoostPad();
    explicit BoostPad(uint64_t address);

    FVector GetLocation() const;
    uint8_t GetPickupState() const;
    float GetRespawnDelay() const;
};

#endif
