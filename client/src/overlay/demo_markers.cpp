#include "demo_markers.hpp"

#include <chrono>
#include <unordered_map>
#include <vector>

#include "SdkHeaders.hpp"
#include "overlay/hitbox_renderer.hpp"
#include "reflection/car.hpp"
#include "reflection/car_list.hpp"
#include "reflection/local_player.hpp"
#include "reflection/match.hpp"

#include <imgui.h>

struct CarSnapshot
{
    FMatrix transform;
    FVector collisionExtent;
    FVector collisionOffset;
    std::vector<FVector> wheelRestPositions;
    std::vector<float> wheelRadii;
};

struct DemoMarker
{
    CarSnapshot snapshot;
    std::chrono::steady_clock::time_point demolishedAt;
};

static std::unordered_map<uint64_t, CarSnapshot> g_lastAliveSnapshots;
static std::vector<DemoMarker> g_markers;

DemoMarkers::DemoMarkers()
{
    m_enabled    = true;
    m_showWheels = true;
    m_duration   = 3.0f;
    m_lineWidth  = 2.0f;
    m_color[0]   = 1.0f;
    m_color[1]   = 0.0f;
    m_color[2]   = 0.0f;
    m_color[3]   = 0.9f;
}

void DemoMarkers::DrawMenu()
{
    if (!ImGui::CollapsingHeader("Demo Markers", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("Enabled", &m_enabled);

    if (!m_enabled)
    {
        return;
    }

    ImGui::Checkbox("Show Wheels", &m_showWheels);
    ImGui::SliderFloat("Duration", &m_duration, 0.5f, 15.0f);
    ImGui::SliderFloat("Line Width", &m_lineWidth, 1.0f, 10.0f);
    ImGui::ColorEdit4("Color", m_color);
}

void DemoMarkers::Render(float screenWidth, float screenHeight)
{
    if (!m_enabled || !Match::IsActive())
    {
        g_lastAliveSnapshots.clear();
        g_markers.clear();
        return;
    }

    LocalPlayer player = LocalPlayer::Find();
    if (!player)
    {
        return;
    }

    FTPOV pov                                 = player.GetCameraPOV();
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::unordered_map<uint64_t, CarSnapshot> stillAlive;

    for (const Car &car : CarList::GetAll())
    {
        std::unordered_map<uint64_t, CarSnapshot>::iterator previous = g_lastAliveSnapshots.find(car.GetAddress());

        if (car.IsDemolished())
        {
            if (previous != g_lastAliveSnapshots.end())
            {
                DemoMarker marker;
                marker.snapshot     = previous->second;
                marker.demolishedAt = now;
                g_markers.push_back(marker);
            }

            continue;
        }

        CarSnapshot snapshot;
        snapshot.transform       = car.GetRenderTransform();
        snapshot.collisionExtent = car.GetLocalCollisionExtent();
        snapshot.collisionOffset = car.GetLocalCollisionOffset();

        for (int i = 0; i < car.GetWheelCount(); i++)
        {
            snapshot.wheelRestPositions.push_back(car.GetWheelLocalRestPosition(i));
            snapshot.wheelRadii.push_back(car.GetWheelRadius(i));
        }

        stillAlive.emplace(car.GetAddress(), snapshot);
    }

    g_lastAliveSnapshots = stillAlive;

    ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(m_color[0], m_color[1], m_color[2], m_color[3]));
    std::vector<DemoMarker> activeMarkers;

    for (const DemoMarker &marker : g_markers)
    {
        if (std::chrono::duration<float>(now - marker.demolishedAt).count() >= m_duration)
        {
            continue;
        }

        HitboxRenderer::DrawBox(marker.snapshot.transform, marker.snapshot.collisionExtent.X, marker.snapshot.collisionExtent.Y,
                                marker.snapshot.collisionExtent.Z, marker.snapshot.collisionOffset, pov, screenWidth, screenHeight, color,
                                m_lineWidth);

        if (m_showWheels)
        {
            for (size_t i = 0; i < marker.snapshot.wheelRestPositions.size(); i++)
            {
                HitboxRenderer::DrawWheel(marker.snapshot.transform, marker.snapshot.wheelRestPositions[i], marker.snapshot.wheelRadii[i], pov,
                                          screenWidth, screenHeight, color, m_lineWidth);
            }
        }

        activeMarkers.push_back(marker);
    }

    g_markers = activeMarkers;
}

void DemoMarkers::SaveConfig(toml::table &table) const
{
    table.insert_or_assign("enabled", m_enabled);
    table.insert_or_assign("show_wheels", m_showWheels);
    table.insert_or_assign("duration", m_duration);
    table.insert_or_assign("line_width", m_lineWidth);
    table.insert_or_assign("color_r", m_color[0]);
    table.insert_or_assign("color_g", m_color[1]);
    table.insert_or_assign("color_b", m_color[2]);
    table.insert_or_assign("color_a", m_color[3]);
}

void DemoMarkers::LoadConfig(const toml::table &table)
{
    m_enabled    = table["enabled"].value_or(m_enabled);
    m_showWheels = table["show_wheels"].value_or(m_showWheels);
    m_duration   = table["duration"].value_or(m_duration);
    m_lineWidth  = table["line_width"].value_or(m_lineWidth);
    m_color[0]   = table["color_r"].value_or(m_color[0]);
    m_color[1]   = table["color_g"].value_or(m_color[1]);
    m_color[2]   = table["color_b"].value_or(m_color[2]);
    m_color[3]   = table["color_a"].value_or(m_color[3]);
}
