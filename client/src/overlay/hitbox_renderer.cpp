#include "hitbox_renderer.hpp"

#include <cmath>

#include "reflection/projection.hpp"

#include <imgui.h>

void HitboxRenderer::DrawBox(const FMatrix &transform, float halfLength, float halfWidth, float halfHeight, const FVector &offset, const FTPOV &pov,
                             float screenWidth, float screenHeight, ImU32 color, float lineWidth)
{
    constexpr int kBoxEdges[12][2] = {
            {0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };

    FVector corners[8];
    int cornerIndex = 0;

    for (float signX : {-1.0f, 1.0f})
    {
        for (float signY : {-1.0f, 1.0f})
        {
            for (float signZ : {-1.0f, 1.0f})
            {
                FVector localCorner{offset.X + signX * halfLength, offset.Y + signY * halfWidth, offset.Z + signZ * halfHeight};
                corners[cornerIndex] = {
                        localCorner.X * transform.XPlane.X + localCorner.Y * transform.YPlane.X + localCorner.Z * transform.ZPlane.X +
                                transform.WPlane.X,
                        localCorner.X * transform.XPlane.Y + localCorner.Y * transform.YPlane.Y + localCorner.Z * transform.ZPlane.Y +
                                transform.WPlane.Y,
                        localCorner.X * transform.XPlane.Z + localCorner.Y * transform.YPlane.Z + localCorner.Z * transform.ZPlane.Z +
                                transform.WPlane.Z,
                };
                cornerIndex++;
            }
        }
    }

    ImVec2 screenCorners[8];
    bool valid[8];

    for (int i = 0; i < 8; i++)
    {
        float screenX, screenY, scale;
        valid[i]         = Projection::WorldToScreen(corners[i], pov, screenWidth, screenHeight, screenX, screenY, scale);
        screenCorners[i] = ImVec2(screenX, screenY);
    }

    for (int i = 0; i < 12; i++)
    {
        if (valid[kBoxEdges[i][0]] && valid[kBoxEdges[i][1]])
        {
            ImGui::GetBackgroundDrawList()->AddLine(screenCorners[kBoxEdges[i][0]], screenCorners[kBoxEdges[i][1]], color, lineWidth);
        }
    }
}

void HitboxRenderer::DrawWheel(const FMatrix &transform, const FVector &offset, float radius, const FTPOV &pov, float screenWidth, float screenHeight,
                               ImU32 color, float lineWidth)
{
    constexpr int kWheelSegments = 24;

    ImVec2 previousScreenPosition;
    bool previousValid = false;

    for (int i = 0; i <= kWheelSegments; i++)
    {
        float angle = (2.0f * M_PI * i) / kWheelSegments;
        FVector localPoint{offset.X + cosf(angle) * radius, offset.Y, offset.Z + sinf(angle) * radius};
        FVector wheelPosition{
                localPoint.X * transform.XPlane.X + localPoint.Y * transform.YPlane.X + localPoint.Z * transform.ZPlane.X + transform.WPlane.X,
                localPoint.X * transform.XPlane.Y + localPoint.Y * transform.YPlane.Y + localPoint.Z * transform.ZPlane.Y + transform.WPlane.Y,
                localPoint.X * transform.XPlane.Z + localPoint.Y * transform.YPlane.Z + localPoint.Z * transform.ZPlane.Z + transform.WPlane.Z,
        };

        float screenX, screenY, scale;
        bool valid = Projection::WorldToScreen(wheelPosition, pov, screenWidth, screenHeight, screenX, screenY, scale);
        if (previousValid && valid)
        {
            ImGui::GetBackgroundDrawList()->AddLine(previousScreenPosition, ImVec2(screenX, screenY), color, lineWidth);
        }

        previousScreenPosition = ImVec2(screenX, screenY);
        previousValid          = valid;
    }
}
