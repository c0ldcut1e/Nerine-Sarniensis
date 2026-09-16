#include "ball_prediction.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "SdkHeaders.hpp"
#include "reflection/background_scanner.hpp"
#include "reflection/ball.hpp"
#include "reflection/local_player.hpp"
#include "reflection/projection.hpp"
#include "reflection/trajectory.hpp"

#include <imgui.h>

constexpr int kTouchCircleSegments               = 32;
constexpr int kSimulationIntervalMilliseconds    = 30;
constexpr float kMinInverseScaleRadiusMultiplier = 0.15;
constexpr float kMaxInverseScaleRadiusMultiplier = 0.3;
constexpr float kFieldExtentY                    = 5120.0f;
constexpr float kGoalHalfWidth                   = 892.755f;

struct TrajectoryPrediction
{
    std::vector<FVector> path;
    std::vector<FVector> touches;
};

static int g_touchesToPredict = 3;

static bool IsInGoal(const FVector &location)
{
    return fabsf(location.Y) > kFieldExtentY && fabsf(location.X) <= kGoalHalfWidth;
}

static TrajectoryPrediction SimulateTrajectory()
{
    TrajectoryPrediction prediction;
    Ball ball = Ball::Find();

    if (ball)
    {
        prediction.path = Trajectory::Simulate(ball, prediction.touches, g_touchesToPredict);
    }

    return prediction;
}

static BackgroundScanner<TrajectoryPrediction> g_scanner(SimulateTrajectory, kSimulationIntervalMilliseconds);

BallPrediction::BallPrediction()
{
    m_enabled              = true;
    m_showTrajectory       = true;
    m_showTouchPredictions = true;
    m_highlightGoalTouches = false;
    m_touchesToPredict     = 3;
    m_trajectoryThickness  = 3.0f;
    m_touchCircleRadius    = 40.0f;
    m_trajectoryColor[0]   = 0.0f;
    m_trajectoryColor[1]   = 1.0f;
    m_trajectoryColor[2]   = 0.0f;
    m_trajectoryColor[3]   = 0.86f;
    m_touchFillColor[0]    = 1.0f;
    m_touchFillColor[1]    = 0.0f;
    m_touchFillColor[2]    = 0.0f;
    m_touchFillColor[3]    = 0.35f;
    m_touchOutlineColor[0] = 1.0f;
    m_touchOutlineColor[1] = 0.0f;
    m_touchOutlineColor[2] = 0.0f;
    m_touchOutlineColor[3] = 1.0f;
    m_goalTouchColor[0]    = 1.0f;
    m_goalTouchColor[1]    = 1.0f;
    m_goalTouchColor[2]    = 0.0f;
    m_goalTouchColor[3]    = 1.0f;
}

void BallPrediction::DrawMenu()
{
    if (!ImGui::CollapsingHeader("Ball Prediction", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("Enabled", &m_enabled);

    if (!m_enabled)
    {
        return;
    }

    ImGui::Checkbox("Show Trajectory", &m_showTrajectory);
    ImGui::Checkbox("Show Touch Predictions", &m_showTouchPredictions);
    ImGui::Checkbox("Highlight Goal Touches", &m_highlightGoalTouches);
    ImGui::SliderInt("Touches To Predict", &m_touchesToPredict, 1, 5);
    ImGui::SliderFloat("Trajectory Thickness", &m_trajectoryThickness, 1.0f, 10.0f);
    ImGui::SliderFloat("Touch Circle Size", &m_touchCircleRadius, 10.0f, 100.0f);
    ImGui::ColorEdit4("Trajectory Color", m_trajectoryColor);
    ImGui::ColorEdit4("Touch Fill Color", m_touchFillColor);
    ImGui::ColorEdit4("Touch Outline Color", m_touchOutlineColor);
    ImGui::ColorEdit4("Goal Touch Color", m_goalTouchColor);
}

void BallPrediction::Render(float screenWidth, float screenHeight) const
{
    if (!m_enabled)
    {
        return;
    }

    LocalPlayer player = LocalPlayer::Find();
    Ball ball          = Ball::Find();
    if (!player || !ball)
    {
        return;
    }

    if (IsInGoal(ball.GetLocation()))
    {
        return;
    }

    g_touchesToPredict = m_touchesToPredict;

    TrajectoryPrediction prediction = g_scanner.Get();
    std::vector<FVector> mapTouches = prediction.touches;
    std::vector<FVector> path       = prediction.path;

    if (m_highlightGoalTouches)
    {
        for (size_t i = 0; i < mapTouches.size(); i++)
        {
            if (IsInGoal(mapTouches[i]))
            {
                mapTouches.resize(i + 1);
                break;
            }
        }

        for (size_t i = 0; i < path.size(); i++)
        {
            if (IsInGoal(path[i]))
            {
                path.resize(i);
                break;
            }
        }

        if (!mapTouches.empty() && IsInGoal(mapTouches.back()))
        {
            path.push_back(mapTouches.back());
        }
    }

    FTPOV pov = player.GetCameraPOV();

    ImU32 trajectoryColor =
            ImGui::ColorConvertFloat4ToU32(ImVec4(m_trajectoryColor[0], m_trajectoryColor[1], m_trajectoryColor[2], m_trajectoryColor[3]));
    if (m_showTrajectory)
    {
        float prevX  = 0.0f;
        float prevY  = 0.0f;
        bool hasPrev = false;

        for (const FVector &point : path)
        {
            float screenX;
            float screenY;
            float scale;

            if (!Projection::WorldToScreen(point, pov, screenWidth, screenHeight, screenX, screenY, scale))
            {
                hasPrev = false;
                continue;
            }

            if (hasPrev)
            {
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(prevX, prevY), ImVec2(screenX, screenY), trajectoryColor, m_trajectoryThickness);
            }

            prevX   = screenX;
            prevY   = screenY;
            hasPrev = true;
        }
    }

    if (m_showTouchPredictions)
    {
        ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(m_touchFillColor[0], m_touchFillColor[1], m_touchFillColor[2], m_touchFillColor[3]));
        ImU32 outlineColor = ImGui::ColorConvertFloat4ToU32(
                ImVec4(m_touchOutlineColor[0], m_touchOutlineColor[1], m_touchOutlineColor[2], m_touchOutlineColor[3]));
        ImU32 goalTouchColor =
                ImGui::ColorConvertFloat4ToU32(ImVec4(m_goalTouchColor[0], m_goalTouchColor[1], m_goalTouchColor[2], m_goalTouchColor[3]));

        for (const FVector &touch : mapTouches)
        {
            float screenX;
            float screenY;
            float scale;

            if (!Projection::WorldToScreen(touch, pov, screenWidth, screenHeight, screenX, screenY, scale))
            {
                continue;
            }

            bool touchInGoal     = m_highlightGoalTouches && IsInGoal(touch);
            ImU32 touchFillColor = touchInGoal ? goalTouchColor : fillColor;

            ImVec2 center(screenX, screenY);
            float radius = std::clamp(m_touchCircleRadius / scale, m_touchCircleRadius * kMinInverseScaleRadiusMultiplier,
                                      m_touchCircleRadius * kMaxInverseScaleRadiusMultiplier);

            ImGui::GetBackgroundDrawList()->AddCircleFilled(center, radius, touchFillColor, kTouchCircleSegments);
            ImGui::GetBackgroundDrawList()->AddCircle(center, radius, outlineColor, kTouchCircleSegments, 2.0f);
        }
    }
}

void BallPrediction::SaveConfig(toml::table &table) const
{
    table.insert_or_assign("enabled", m_enabled);
    table.insert_or_assign("show_trajectory", m_showTrajectory);
    table.insert_or_assign("show_touch_predictions", m_showTouchPredictions);
    table.insert_or_assign("highlight_goal_touches", m_highlightGoalTouches);
    table.insert_or_assign("touches_to_predict", m_touchesToPredict);
    table.insert_or_assign("trajectory_thickness", m_trajectoryThickness);
    table.insert_or_assign("touch_circle_radius", m_touchCircleRadius);
    table.insert_or_assign("trajectory_color_r", m_trajectoryColor[0]);
    table.insert_or_assign("trajectory_color_g", m_trajectoryColor[1]);
    table.insert_or_assign("trajectory_color_b", m_trajectoryColor[2]);
    table.insert_or_assign("trajectory_color_a", m_trajectoryColor[3]);
    table.insert_or_assign("touch_fill_color_r", m_touchFillColor[0]);
    table.insert_or_assign("touch_fill_color_g", m_touchFillColor[1]);
    table.insert_or_assign("touch_fill_color_b", m_touchFillColor[2]);
    table.insert_or_assign("touch_fill_color_a", m_touchFillColor[3]);
    table.insert_or_assign("touch_outline_color_r", m_touchOutlineColor[0]);
    table.insert_or_assign("touch_outline_color_g", m_touchOutlineColor[1]);
    table.insert_or_assign("touch_outline_color_b", m_touchOutlineColor[2]);
    table.insert_or_assign("touch_outline_color_a", m_touchOutlineColor[3]);
    table.insert_or_assign("goal_touch_color_r", m_goalTouchColor[0]);
    table.insert_or_assign("goal_touch_color_g", m_goalTouchColor[1]);
    table.insert_or_assign("goal_touch_color_b", m_goalTouchColor[2]);
    table.insert_or_assign("goal_touch_color_a", m_goalTouchColor[3]);
}

void BallPrediction::LoadConfig(const toml::table &table)
{
    m_enabled              = table["enabled"].value_or(m_enabled);
    m_showTrajectory       = table["show_trajectory"].value_or(m_showTrajectory);
    m_showTouchPredictions = table["show_touch_predictions"].value_or(m_showTouchPredictions);
    m_highlightGoalTouches = table["highlight_goal_touches"].value_or(m_highlightGoalTouches);
    m_touchesToPredict     = table["touches_to_predict"].value_or(m_touchesToPredict);
    m_trajectoryThickness  = table["trajectory_thickness"].value_or(m_trajectoryThickness);
    m_touchCircleRadius    = table["touch_circle_radius"].value_or(m_touchCircleRadius);
    m_trajectoryColor[0]   = table["trajectory_color_r"].value_or(m_trajectoryColor[0]);
    m_trajectoryColor[1]   = table["trajectory_color_g"].value_or(m_trajectoryColor[1]);
    m_trajectoryColor[2]   = table["trajectory_color_b"].value_or(m_trajectoryColor[2]);
    m_trajectoryColor[3]   = table["trajectory_color_a"].value_or(m_trajectoryColor[3]);
    m_touchFillColor[0]    = table["touch_fill_color_r"].value_or(m_touchFillColor[0]);
    m_touchFillColor[1]    = table["touch_fill_color_g"].value_or(m_touchFillColor[1]);
    m_touchFillColor[2]    = table["touch_fill_color_b"].value_or(m_touchFillColor[2]);
    m_touchFillColor[3]    = table["touch_fill_color_a"].value_or(m_touchFillColor[3]);
    m_touchOutlineColor[0] = table["touch_outline_color_r"].value_or(m_touchOutlineColor[0]);
    m_touchOutlineColor[1] = table["touch_outline_color_g"].value_or(m_touchOutlineColor[1]);
    m_touchOutlineColor[2] = table["touch_outline_color_b"].value_or(m_touchOutlineColor[2]);
    m_touchOutlineColor[3] = table["touch_outline_color_a"].value_or(m_touchOutlineColor[3]);
    m_goalTouchColor[0]    = table["goal_touch_color_r"].value_or(m_goalTouchColor[0]);
    m_goalTouchColor[1]    = table["goal_touch_color_g"].value_or(m_goalTouchColor[1]);
    m_goalTouchColor[2]    = table["goal_touch_color_b"].value_or(m_goalTouchColor[2]);
    m_goalTouchColor[3]    = table["goal_touch_color_a"].value_or(m_goalTouchColor[3]);
}
