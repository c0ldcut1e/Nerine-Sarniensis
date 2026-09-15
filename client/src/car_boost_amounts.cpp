#include "car_boost_amounts.hpp"

#include <algorithm>
#include <vector>

#include "reflection/car_list.hpp"
#include "reflection/local_player.hpp"
#include "reflection/projection.hpp"
#include "SDK_HEADERS/Core_structs.hpp"

#include <imgui.h>

constexpr int kCircleSegments = 64;

CarBoostAmounts::CarBoostAmounts()
{
    m_enabled      = true;
    m_circleRadius = 40.0f;
    m_heightOffset = 150.0f;
}

void CarBoostAmounts::DrawMenu()
{
    if (!ImGui::CollapsingHeader("Car Boost Amounts", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("Show Car Boost Amounts", &m_enabled);

    if (!m_enabled)
    {
        return;
    }

    ImGui::SliderFloat("Circle Size", &m_circleRadius, 10.0f, 100.0f);
    ImGui::SliderFloat("Circle Height", &m_heightOffset, 50.0f, 300.0f);

    if (ImGui::Button("Log Car Boost Values"))
    {
        CarList::Debug();
    }
}

void CarBoostAmounts::Render(float screenWidth, float screenHeight)
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

    FTPOV pov             = player.GetCameraPOV();
    Car localCar          = player.GetCar();
    std::vector<Car> cars = CarList::GetAll();
    ImDrawList *drawList  = ImGui::GetForegroundDrawList();

    for (const Car &car : cars)
    {
        if (car.GetAddress() == localCar.GetAddress())
        {
            continue;
        }

        FVector carLocation = car.GetLocation();
        if (carLocation.X == 0.0f && carLocation.Y == 0.0f && carLocation.Z == 0.0f)
        {
            continue;
        }

        carLocation.Z += m_heightOffset;

        float screenX;
        float screenY;
        float scale;
        if (!Projection::WorldToScreen(carLocation, pov, screenWidth, screenHeight, screenX, screenY, scale))
        {
            continue;
        }

        float progress = std::clamp(car.GetBoostAmount() / 100.0f, 0.0f, 1.0f);
        float colorR;
        float colorG;
        float colorB;
        ImGui::ColorConvertHSVtoRGB(progress / 3.0f, 0.85f, 0.9f, colorR, colorG, colorB);

        ImVec2 center(screenX, screenY);
        float radius    = m_circleRadius * scale;
        ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(colorR, colorG, colorB, 1.0f));

        if (progress >= 1.0f)
        {
            drawList->AddCircleFilled(center, radius, fillColor, kCircleSegments);
        }
        else if (progress > 0.0f)
        {
            constexpr float kStartAngle = -(float) M_PI / 2.0f;
            float endAngle              = kStartAngle + progress * 2.0f * (float) M_PI;

            drawList->PathLineTo(center);
            drawList->PathArcTo(center, radius, kStartAngle, endAngle, kCircleSegments);
            drawList->PathFillConvex(fillColor);
        }

        drawList->AddCircle(center, radius, IM_COL32_WHITE, kCircleSegments, 2.0f);
    }
}

void CarBoostAmounts::SaveConfig(toml::table &table) const
{
    table.insert_or_assign("enabled", m_enabled);
    table.insert_or_assign("circle_radius", m_circleRadius);
    table.insert_or_assign("height_offset", m_heightOffset);
}

void CarBoostAmounts::LoadConfig(const toml::table &table)
{
    m_enabled      = table["enabled"].value_or(m_enabled);
    m_circleRadius = table["circle_radius"].value_or(m_circleRadius);
    m_heightOffset = table["height_offset"].value_or(m_heightOffset);
}
