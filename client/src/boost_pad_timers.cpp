#include "boost_pad_timers.hpp"

#include <algorithm>

#include "reflection/boost_pad_list.hpp"
#include "reflection/local_player.hpp"
#include "reflection/projection.hpp"
#include "SDK_HEADERS/Core_structs.hpp"

#include <imgui.h>

constexpr int kCircleSegments       = 64;
constexpr float kCircleHeightOffset = 90.0f;

BoostPadTimers::BoostPadTimers()
{
    m_enabled      = true;
    m_circleRadius = 40.0f;
}

void BoostPadTimers::DrawMenu()
{
    if (!ImGui::CollapsingHeader("Boost Pad Timers", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("Enabled", &m_enabled);

    if (!m_enabled)
    {
        return;
    }

    ImGui::SliderFloat("Circle Size", &m_circleRadius, 10.0f, 100.0f);
}

void BoostPadTimers::Render(float screenWidth, float screenHeight)
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

    for (const BoostPad &pad : BoostPadList::GetAll())
    {
        uint64_t address    = pad.GetAddress();
        uint8_t pickupState = pad.GetPickupState();

        auto stateIt      = m_lastPickupStates.find(address);
        bool stateChanged = stateIt == m_lastPickupStates.end() || stateIt->second != pickupState;

        m_lastPickupStates[address] = pickupState;

        if ((pickupState & 1) == 0)
        {
            m_pickupTimestamps.erase(address);
            continue;
        }

        if (stateChanged || m_pickupTimestamps.find(address) == m_pickupTimestamps.end())
        {
            m_pickupTimestamps[address] = std::chrono::steady_clock::now();
        }

        float respawnDelay = pad.GetRespawnDelay();
        if (respawnDelay <= 0.0f)
        {
            continue;
        }

        float elapsed  = std::chrono::duration<float>(std::chrono::steady_clock::now() - m_pickupTimestamps[address]).count();
        float progress = std::min(elapsed / respawnDelay, 1.0f);

        FVector padLocation = pad.GetLocation();
        padLocation.Z += kCircleHeightOffset;

        float screenX;
        float screenY;
        float scale;
        if (!Projection::WorldToScreen(padLocation, pov, screenWidth, screenHeight, screenX, screenY, scale))
        {
            continue;
        }

        ImVec2 center(screenX, screenY);
        float radius = m_circleRadius * scale;

        float colorR;
        float colorG;
        float colorB;
        ImGui::ColorConvertHSVtoRGB(progress / 3.0f, 0.85f, 0.9f, colorR, colorG, colorB);
        ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(colorR, colorG, colorB, 1.0f));

        ImDrawList *drawList = ImGui::GetForegroundDrawList();

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

void BoostPadTimers::SaveConfig(toml::table &table) const
{
    table.insert_or_assign("enabled", m_enabled);
    table.insert_or_assign("circle_radius", m_circleRadius);
}

void BoostPadTimers::LoadConfig(const toml::table &table)
{
    m_enabled      = table["enabled"].value_or(m_enabled);
    m_circleRadius = table["circle_radius"].value_or(m_circleRadius);
}
