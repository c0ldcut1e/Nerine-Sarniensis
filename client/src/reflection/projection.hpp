#ifndef PROJECTION_HPP
#define PROJECTION_HPP

#include "SdkHeaders.hpp"

class Projection
{
public:
    static bool WorldToScreen(FVector worldLocation, FTPOV pov, float screenWidth, float screenHeight, float &outX, float &outY, float &outScale);
};

#endif
