#include "projection.hpp"

#include <cmath>

bool Projection::WorldToScreen(FVector worldLocation, FTPOV pov, float screenWidth, float screenHeight, float &outX, float &outY, float &outScale)
{
    float pitch = pov.Rotation.Pitch * (M_PI / 32768.0f);
    float yaw   = pov.Rotation.Yaw * (M_PI / 32768.0f);
    float roll  = pov.Rotation.Roll * (M_PI / 32768.0f);

    float sp = sinf(pitch);
    float cp = cosf(pitch);
    float sy = sinf(yaw);
    float cy = cosf(yaw);
    float sr = sinf(roll);
    float cr = cosf(roll);

    FVector forward;
    forward.X = cp * cy;
    forward.Y = cp * sy;
    forward.Z = sp;

    FVector right;
    right.X = sr * sp * cy - cr * sy;
    right.Y = sr * sp * sy + cr * cy;
    right.Z = -sr * cp;

    FVector up;
    up.X = -(cr * sp * cy + sr * sy);
    up.Y = -(cr * sp * sy - sr * cy);
    up.Z = cr * cp;

    float deltaX = worldLocation.X - pov.Location.X;
    float deltaY = worldLocation.Y - pov.Location.Y;
    float deltaZ = worldLocation.Z - pov.Location.Z;

    float transformedForward = deltaX * forward.X + deltaY * forward.Y + deltaZ * forward.Z;
    if (transformedForward < 1.0f)
    {
        return false;
    }

    float transformedRight = deltaX * right.X + deltaY * right.Y + deltaZ * right.Z;
    float transformedUp    = deltaX * up.X + deltaY * up.Y + deltaZ * up.Z;

    float screenCenterX = screenWidth / 2.0f;
    float screenCenterY = screenHeight / 2.0f;

    float fovRadians     = pov.FOV * (M_PI / 180.0f);
    float screenDistance = screenCenterX / tanf(fovRadians / 2.0f);

    outScale = screenDistance / transformedForward;
    outX     = screenCenterX + transformedRight * outScale;
    outY     = screenCenterY - transformedUp * outScale;

    return true;
}
