#ifndef LOCAL_PLAYER_HPP
#define LOCAL_PLAYER_HPP

#include "SdkHeaders.hpp"
#include "car.hpp"
#include "remote_object.hpp"

class LocalPlayer : public RemoteObject
{
public:
    LocalPlayer();
    explicit LocalPlayer(uint64_t address);

    FTPOV GetCameraPOV() const;
    Car GetCar() const;

    static LocalPlayer Find();
};

#endif
