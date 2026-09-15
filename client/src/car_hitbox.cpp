#include "car_hitbox.hpp"

#include <cmath>
#include <vector>

#include "SdkHeaders.hpp"
#include "reflection/car.hpp"
#include "reflection/car_list.hpp"
#include "reflection/local_player.hpp"
#include "reflection/projection.hpp"

#include <imgui.h>

static void DrawOrientedBox(const FMatrix &transform, float halfLength, float halfWidth, float halfHeight, const FVector &offset, const FTPOV &pov,
                            float screenWidth, float screenHeight, ImU32 color)
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
            ImGui::GetForegroundDrawList()->AddLine(screenCorners[kBoxEdges[i][0]], screenCorners[kBoxEdges[i][1]], color, 2.0f);
        }
    }
}

static void DrawWheel(const FMatrix &transform, const FVector &offset, float radius, const FTPOV &pov, float screenWidth, float screenHeight,
                      ImU32 color)
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
            ImGui::GetForegroundDrawList()->AddLine(previousScreenPosition, ImVec2(screenX, screenY), color, 2.0f);
        }

        previousScreenPosition = ImVec2(screenX, screenY);
        previousValid          = valid;
    }
}

CarHitbox::CarHitbox()
{
    m_enabled      = true;
    m_showEveryCar = false;
    m_color[0]     = 1.0f;
    m_color[1]     = 1.0f;
    m_color[2]     = 0.0f;
    m_color[3]     = 0.9f;
}

void CarHitbox::DrawMenu()
{
    if (!ImGui::CollapsingHeader("Car Hitboxes", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("Enabled", &m_enabled);

    if (!m_enabled)
    {
        return;
    }

    ImGui::Checkbox("Show Every Car", &m_showEveryCar);
    ImGui::ColorEdit4("Color", m_color);
}

void CarHitbox::Render(float screenWidth, float screenHeight)
{
    if (!m_enabled)
    {
        return;
    }

    LocalPlayer player = LocalPlayer::Find();
    if (!player)
    {
        return;
    }

    FTPOV pov = player.GetCameraPOV();

    Car localCar = player.GetCar();
    if (!localCar)
    {
        return;
    }

    ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(m_color[0], m_color[1], m_color[2], m_color[3]));

    if (!m_showEveryCar)
    {
        FMatrix transform = localCar.GetRenderTransform();

        FVector collisionExtent = localCar.GetLocalCollisionExtent();
        DrawOrientedBox(transform, collisionExtent.X, collisionExtent.Y, collisionExtent.Z, localCar.GetLocalCollisionOffset(), pov, screenWidth,
                        screenHeight, color);
        for (int i = 0; i < localCar.GetWheelCount(); i++)
        {
            DrawWheel(transform, localCar.GetWheelLocalRestPosition(i), localCar.GetWheelRadius(i), pov, screenWidth, screenHeight, color);
        }
        return;
    }

    std::vector<Car> cars = CarList::GetAll();

    for (const Car &car : cars)
    {
        FMatrix transform       = car.GetRenderTransform();
        FVector collisionExtent = car.GetLocalCollisionExtent();

        DrawOrientedBox(transform, collisionExtent.X, collisionExtent.Y, collisionExtent.Z, car.GetLocalCollisionOffset(), pov, screenWidth,
                        screenHeight, color);
        for (int i = 0; i < car.GetWheelCount(); i++)
        {
            DrawWheel(transform, car.GetWheelLocalRestPosition(i), car.GetWheelRadius(i), pov, screenWidth, screenHeight, color);
        }
    }
}

void CarHitbox::SaveConfig(toml::table &table) const
{
    table.insert_or_assign("enabled", m_enabled);
    table.insert_or_assign("show_every_car", m_showEveryCar);
    table.insert_or_assign("color_r", m_color[0]);
    table.insert_or_assign("color_g", m_color[1]);
    table.insert_or_assign("color_b", m_color[2]);
    table.insert_or_assign("color_a", m_color[3]);
}

void CarHitbox::LoadConfig(const toml::table &table)
{
    m_enabled      = table["enabled"].value_or(m_enabled);
    m_showEveryCar = table["show_every_car"].value_or(m_showEveryCar);
    m_color[0]     = table["color_r"].value_or(m_color[0]);
    m_color[1]     = table["color_g"].value_or(m_color[1]);
    m_color[2]     = table["color_b"].value_or(m_color[2]);
    m_color[3]     = table["color_a"].value_or(m_color[3]);
}
