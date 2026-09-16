#ifndef HITBOX_RENDERER_HPP
#define HITBOX_RENDERER_HPP

#include "SdkHeaders.hpp"

#include <imgui.h>

class HitboxRenderer
{
public:
    static void DrawBox(const FMatrix &transform, float halfLength, float halfWidth, float halfHeight, const FVector &offset, const FTPOV &pov,
                        float screenWidth, float screenHeight, ImU32 color, float lineWidth);
    static void DrawWheel(const FMatrix &transform, const FVector &offset, float radius, const FTPOV &pov, float screenWidth, float screenHeight,
                          ImU32 color, float lineWidth);
};

#endif
