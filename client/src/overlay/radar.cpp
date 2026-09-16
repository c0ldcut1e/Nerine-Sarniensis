#include "radar.hpp"

#include <algorithm>
#include <cmath>

#include "SdkHeaders.hpp"
#include "overlay/screen_position.hpp"
#include "reflection/ball.hpp"
#include "reflection/boost_pad.hpp"
#include "reflection/boost_pad_list.hpp"
#include "reflection/car.hpp"
#include "reflection/car_list.hpp"
#include "reflection/match.hpp"

#include <imgui.h>

constexpr float kFieldExtentX         = 4096.0f;
constexpr float kFieldExtentY         = 5120.0f;
constexpr float kCornerCut            = 1152.0f;
constexpr float kGoalHalfWidth        = 892.755f;
constexpr float kGoalLineThickness    = 4.0f;
constexpr float kMargin               = 20.0f;
constexpr float kCarDotRadius         = 5.0f;
constexpr float kBallDotRadius        = 4.0f;
constexpr float kBoostPadDotRadius    = 3.0f;
constexpr float kBigBoostPadDotRadius = 5.0f;
constexpr float kBigBoostAmount       = 50.0f;
constexpr float kCarArrowSpread       = 2.5f;
constexpr ImU32 kBlueTeamColor        = IM_COL32(60, 140, 255, 255);
constexpr ImU32 kOrangeTeamColor      = IM_COL32(255, 140, 30, 255);

static ImVec2 WorldToRadar(float worldX, float worldY, const ImVec2 &radarMin, const ImVec2 &radarSize)
{
    float normalizedX = (worldX + kFieldExtentX) / (2.0f * kFieldExtentX);
    float normalizedY = (worldY + kFieldExtentY) / (2.0f * kFieldExtentY);
    return ImVec2(radarMin.x + normalizedX * radarSize.x, radarMin.y + normalizedY * radarSize.y);
}

static FVector ClampToField(const FVector &location)
{
    FVector clamped = location;
    clamped.X       = std::clamp(location.X, -kFieldExtentX, kFieldExtentX);
    clamped.Y       = std::clamp(location.Y, -kFieldExtentY, kFieldExtentY);
    return clamped;
}

Radar::Radar()
{
    m_enabled            = true;
    m_size               = 200.0f;
    m_position           = 0;
    m_backgroundColor[0] = 0.05f;
    m_backgroundColor[1] = 0.05f;
    m_backgroundColor[2] = 0.05f;
    m_backgroundColor[3] = 0.75f;
    m_ballColor[0]       = 1.0f;
    m_ballColor[1]       = 1.0f;
    m_ballColor[2]       = 1.0f;
    m_ballColor[3]       = 1.0f;
    m_boostPadColor[0]   = 1.0f;
    m_boostPadColor[1]   = 0.85f;
    m_boostPadColor[2]   = 0.0f;
    m_boostPadColor[3]   = 1.0f;
}

void Radar::DrawMenu()
{
    if (!ImGui::CollapsingHeader("Radar", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("Enabled", &m_enabled);

    if (!m_enabled)
    {
        return;
    }

    ImGui::SliderFloat("Size", &m_size, 100.0f, 400.0f);
    ImGui::Combo("Position", &m_position, "Top Left\0Top Center\0Top Right\0Left Center\0Right Center\0Bottom Left\0Bottom Center\0Bottom Right\0");
    ImGui::ColorEdit4("Background Color", m_backgroundColor);
    ImGui::ColorEdit4("Ball Color", m_ballColor);
    ImGui::ColorEdit4("Boost Pad Color", m_boostPadColor);
}

void Radar::Render(float screenWidth, float screenHeight)
{
    if (!m_enabled || !Match::IsActive())
    {
        return;
    }

    ImVec2 radarSize(m_size * (kFieldExtentX / kFieldExtentY), m_size);
    ImVec2 radarMin = ScreenPosition::Resolve(m_position, radarSize, screenWidth, screenHeight, kMargin);

    ImDrawList *drawList = ImGui::GetBackgroundDrawList();
    ImU32 backgroundColor =
            ImGui::ColorConvertFloat4ToU32(ImVec4(m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2], m_backgroundColor[3]));

    ImVec2 fieldOutline[8] = {
            WorldToRadar(-kFieldExtentX + kCornerCut, -kFieldExtentY, radarMin, radarSize),
            WorldToRadar(kFieldExtentX - kCornerCut, -kFieldExtentY, radarMin, radarSize),
            WorldToRadar(kFieldExtentX, -kFieldExtentY + kCornerCut, radarMin, radarSize),
            WorldToRadar(kFieldExtentX, kFieldExtentY - kCornerCut, radarMin, radarSize),
            WorldToRadar(kFieldExtentX - kCornerCut, kFieldExtentY, radarMin, radarSize),
            WorldToRadar(-kFieldExtentX + kCornerCut, kFieldExtentY, radarMin, radarSize),
            WorldToRadar(-kFieldExtentX, kFieldExtentY - kCornerCut, radarMin, radarSize),
            WorldToRadar(-kFieldExtentX, -kFieldExtentY + kCornerCut, radarMin, radarSize),
    };
    drawList->AddConvexPolyFilled(fieldOutline, 8, backgroundColor);

    ImVec2 blueGoalStart = WorldToRadar(-kGoalHalfWidth, -kFieldExtentY, radarMin, radarSize);
    ImVec2 blueGoalEnd   = WorldToRadar(kGoalHalfWidth, -kFieldExtentY, radarMin, radarSize);
    blueGoalStart.y += kGoalLineThickness / 2.0f;
    blueGoalEnd.y += kGoalLineThickness / 2.0f;
    drawList->AddLine(blueGoalStart, blueGoalEnd, kBlueTeamColor, kGoalLineThickness);

    ImVec2 orangeGoalStart = WorldToRadar(-kGoalHalfWidth, kFieldExtentY, radarMin, radarSize);
    ImVec2 orangeGoalEnd   = WorldToRadar(kGoalHalfWidth, kFieldExtentY, radarMin, radarSize);
    orangeGoalStart.y -= kGoalLineThickness / 2.0f;
    orangeGoalEnd.y -= kGoalLineThickness / 2.0f;
    drawList->AddLine(orangeGoalStart, orangeGoalEnd, kOrangeTeamColor, kGoalLineThickness);

    ImU32 boostPadColor = ImGui::ColorConvertFloat4ToU32(ImVec4(m_boostPadColor[0], m_boostPadColor[1], m_boostPadColor[2], m_boostPadColor[3]));
    for (const BoostPad &pad : BoostPadList::GetAll())
    {
        FVector location = ClampToField(pad.GetLocation());
        ImVec2 point     = WorldToRadar(location.X, location.Y, radarMin, radarSize);
        float radius     = pad.GetBoostAmount() >= kBigBoostAmount ? kBigBoostPadDotRadius : kBoostPadDotRadius;
        drawList->AddCircleFilled(point, radius, boostPadColor);
    }

    Ball ball = Ball::Find();
    if (ball)
    {
        FVector location = ClampToField(ball.GetLocation());
        ImVec2 point     = WorldToRadar(location.X, location.Y, radarMin, radarSize);
        ImU32 ballColor  = ImGui::ColorConvertFloat4ToU32(ImVec4(m_ballColor[0], m_ballColor[1], m_ballColor[2], m_ballColor[3]));
        drawList->AddCircleFilled(point, kBallDotRadius, ballColor);
    }

    for (const Car &car : CarList::GetAll())
    {
        FVector location  = ClampToField(car.GetLocation());
        FMatrix transform = car.GetRenderTransform();
        float angle       = atan2f(transform.XPlane.Y, transform.XPlane.X);
        ImVec2 center     = WorldToRadar(location.X, location.Y, radarMin, radarSize);
        ImU32 carColor    = car.GetTeam() == 0 ? kBlueTeamColor : kOrangeTeamColor;

        ImVec2 nose(center.x + cosf(angle) * kCarDotRadius * 2.0f, center.y + sinf(angle) * kCarDotRadius * 2.0f);
        ImVec2 left(center.x + cosf(angle + kCarArrowSpread) * kCarDotRadius, center.y + sinf(angle + kCarArrowSpread) * kCarDotRadius);
        ImVec2 right(center.x + cosf(angle - kCarArrowSpread) * kCarDotRadius, center.y + sinf(angle - kCarArrowSpread) * kCarDotRadius);

        drawList->AddTriangleFilled(nose, left, right, carColor);
    }
}

void Radar::SaveConfig(toml::table &table) const
{
    table.insert_or_assign("enabled", m_enabled);
    table.insert_or_assign("size", m_size);
    table.insert_or_assign("position", m_position);
    table.insert_or_assign("background_color_r", m_backgroundColor[0]);
    table.insert_or_assign("background_color_g", m_backgroundColor[1]);
    table.insert_or_assign("background_color_b", m_backgroundColor[2]);
    table.insert_or_assign("background_color_a", m_backgroundColor[3]);
    table.insert_or_assign("ball_color_r", m_ballColor[0]);
    table.insert_or_assign("ball_color_g", m_ballColor[1]);
    table.insert_or_assign("ball_color_b", m_ballColor[2]);
    table.insert_or_assign("ball_color_a", m_ballColor[3]);
    table.insert_or_assign("boost_pad_color_r", m_boostPadColor[0]);
    table.insert_or_assign("boost_pad_color_g", m_boostPadColor[1]);
    table.insert_or_assign("boost_pad_color_b", m_boostPadColor[2]);
    table.insert_or_assign("boost_pad_color_a", m_boostPadColor[3]);
}

void Radar::LoadConfig(const toml::table &table)
{
    m_enabled            = table["enabled"].value_or(m_enabled);
    m_size               = table["size"].value_or(m_size);
    m_position           = table["position"].value_or(m_position);
    m_backgroundColor[0] = table["background_color_r"].value_or(m_backgroundColor[0]);
    m_backgroundColor[1] = table["background_color_g"].value_or(m_backgroundColor[1]);
    m_backgroundColor[2] = table["background_color_b"].value_or(m_backgroundColor[2]);
    m_backgroundColor[3] = table["background_color_a"].value_or(m_backgroundColor[3]);
    m_ballColor[0]       = table["ball_color_r"].value_or(m_ballColor[0]);
    m_ballColor[1]       = table["ball_color_g"].value_or(m_ballColor[1]);
    m_ballColor[2]       = table["ball_color_b"].value_or(m_ballColor[2]);
    m_ballColor[3]       = table["ball_color_a"].value_or(m_ballColor[3]);
    m_boostPadColor[0]   = table["boost_pad_color_r"].value_or(m_boostPadColor[0]);
    m_boostPadColor[1]   = table["boost_pad_color_g"].value_or(m_boostPadColor[1]);
    m_boostPadColor[2]   = table["boost_pad_color_b"].value_or(m_boostPadColor[2]);
    m_boostPadColor[3]   = table["boost_pad_color_a"].value_or(m_boostPadColor[3]);
}
