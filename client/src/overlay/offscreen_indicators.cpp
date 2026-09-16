#include "offscreen_indicators.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "SdkHeaders.hpp"
#include "reflection/car.hpp"
#include "reflection/car_list.hpp"
#include "reflection/local_player.hpp"
#include "reflection/match.hpp"
#include "reflection/projection.hpp"

#include <imgui.h>

constexpr float kArrowSpread            = 2.5f;
constexpr float kMinimumDirectionLength = 0.0001f;
constexpr ImU32 kBlueTeamColor          = IM_COL32(60, 140, 255, 255);
constexpr ImU32 kOrangeTeamColor        = IM_COL32(255, 140, 30, 255);

OffscreenIndicators::OffscreenIndicators()
{
    m_enabled       = true;
    m_showTeammates = true;
    m_size          = 14.0f;
    m_margin        = 40.0f;
}

void OffscreenIndicators::DrawMenu()
{
    if (!ImGui::CollapsingHeader("Offscreen Indicators", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("Enabled", &m_enabled);

    if (!m_enabled)
    {
        return;
    }

    ImGui::Checkbox("Show Teammates", &m_showTeammates);
    ImGui::SliderFloat("Arrow Size", &m_size, 5.0f, 40.0f);
    ImGui::SliderFloat("Edge Margin", &m_margin, 10.0f, 150.0f);
}

void OffscreenIndicators::Render(float screenWidth, float screenHeight)
{
    if (!m_enabled || !Match::IsActive())
    {
        return;
    }

    LocalPlayer player = LocalPlayer::Find();
    if (!player)
    {
        return;
    }

    Car localCar = player.GetCar();
    if (!localCar)
    {
        return;
    }

    FTPOV pov     = player.GetCameraPOV();
    int localTeam = localCar.GetTeam();

    float centerX    = screenWidth / 2.0f;
    float centerY    = screenHeight / 2.0f;
    float halfWidth  = centerX - m_margin;
    float halfHeight = centerY - m_margin;

    ImDrawList *drawList = ImGui::GetBackgroundDrawList();

    for (const Car &car : CarList::GetAll())
    {
        if (car.GetAddress() == localCar.GetAddress())
        {
            continue;
        }

        if (!m_showTeammates && car.GetTeam() == localTeam)
        {
            continue;
        }

        if (car.IsDemolished())
        {
            continue;
        }

        FVector location = car.GetLocation();
        float screenX;
        float screenY;
        float scale;

        if (Projection::WorldToScreen(location, pov, screenWidth, screenHeight, screenX, screenY, scale) && fabsf(screenX - centerX) <= halfWidth &&
            fabsf(screenY - centerY) <= halfHeight)
        {
            continue;
        }

        FVector camera;
        Projection::WorldToCamera(location, pov, camera);

        float directionX = camera.Y;
        float directionY = -camera.Z;
        float length     = sqrtf(directionX * directionX + directionY * directionY);

        if (length < kMinimumDirectionLength)
        {
            directionX = 0.0f;
            directionY = 1.0f;
            length     = 1.0f;
        }

        directionX /= length;
        directionY /= length;

        float edgeScaleX = fabsf(directionX) > kMinimumDirectionLength ? halfWidth / fabsf(directionX) : std::numeric_limits<float>::max();
        float edgeScaleY = fabsf(directionY) > kMinimumDirectionLength ? halfHeight / fabsf(directionY) : std::numeric_limits<float>::max();
        float edgeScale  = std::min(edgeScaleX, edgeScaleY);

        ImVec2 nose(centerX + directionX * edgeScale, centerY + directionY * edgeScale);
        float angle    = atan2f(directionY, directionX);
        ImU32 carColor = car.GetTeam() == 0 ? kBlueTeamColor : kOrangeTeamColor;

        ImVec2 left(nose.x + cosf(angle + kArrowSpread) * m_size, nose.y + sinf(angle + kArrowSpread) * m_size);
        ImVec2 right(nose.x + cosf(angle - kArrowSpread) * m_size, nose.y + sinf(angle - kArrowSpread) * m_size);

        drawList->AddTriangleFilled(nose, left, right, carColor);
    }
}

void OffscreenIndicators::SaveConfig(toml::table &table) const
{
    table.insert_or_assign("enabled", m_enabled);
    table.insert_or_assign("show_teammates", m_showTeammates);
    table.insert_or_assign("size", m_size);
    table.insert_or_assign("margin", m_margin);
}

void OffscreenIndicators::LoadConfig(const toml::table &table)
{
    m_enabled       = table["enabled"].value_or(m_enabled);
    m_showTeammates = table["show_teammates"].value_or(m_showTeammates);
    m_size          = table["size"].value_or(m_size);
    m_margin        = table["margin"].value_or(m_margin);
}
