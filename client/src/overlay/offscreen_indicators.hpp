#ifndef OFFSCREEN_INDICATORS_HPP
#define OFFSCREEN_INDICATORS_HPP

#include <toml.hpp>

class OffscreenIndicators
{
public:
    OffscreenIndicators();

    void DrawMenu();
    void Render(float screenWidth, float screenHeight);
    void SaveConfig(toml::table &table) const;
    void LoadConfig(const toml::table &table);

private:
    bool m_enabled;
    bool m_showTeammates;
    float m_size;
    float m_margin;
};

#endif
