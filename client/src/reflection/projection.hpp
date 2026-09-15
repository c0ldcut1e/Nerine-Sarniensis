#ifndef PROJECTION_HPP
#define PROJECTION_HPP

#include "SdkHeaders.hpp"
#include "SDK_HEADERS/Core_structs.hpp"

class Projection
{
public:
    static bool WorldToScreen(FVector worldLocation, FTPOV pov, float screenWidth, float screenHeight, float &outX, float &outY, float &outScale);
};

#endif
