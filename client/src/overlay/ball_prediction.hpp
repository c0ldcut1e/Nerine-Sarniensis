#ifndef BALL_PREDICTION_HPP
#define BALL_PREDICTION_HPP

#include <toml.hpp>

class BallPrediction
{
public:
    BallPrediction();

    void DrawMenu();
    void Render(float screenWidth, float screenHeight) const;
    void SaveConfig(toml::table &table) const;
    void LoadConfig(const toml::table &table);

private:
    bool m_enabled;
    bool m_showTrajectory;
    bool m_showTouchPredictions;
    bool m_highlightGoalTouches;
    int m_touchesToPredict;
    float m_trajectoryThickness;
    float m_touchCircleRadius;
    float m_trajectoryColor[4];
    float m_touchFillColor[4];
    float m_touchOutlineColor[4];
    float m_goalTouchColor[4];
};

#endif
