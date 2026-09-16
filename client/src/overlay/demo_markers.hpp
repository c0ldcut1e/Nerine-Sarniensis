#ifndef DEMO_MARKERS_HPP
#define DEMO_MARKERS_HPP

#include <toml.hpp>

class DemoMarkers
{
public:
    DemoMarkers();

    void DrawMenu();
    void Render(float screenWidth, float screenHeight);
    void SaveConfig(toml::table &table) const;
    void LoadConfig(const toml::table &table);

private:
    bool m_enabled;
    bool m_showWheels;
    float m_duration;
    float m_lineWidth;
    float m_color[4];
};

#endif
