#ifndef RADAR_HPP
#define RADAR_HPP

#include <toml.hpp>

class Radar
{
public:
    Radar();

    void DrawMenu();
    void Render(float screenWidth, float screenHeight);
    void SaveConfig(toml::table &table) const;
    void LoadConfig(const toml::table &table);

private:
    bool m_enabled;
    float m_size;
    int m_position;
    float m_backgroundColor[4];
    float m_ballColor[4];
    float m_boostPadColor[4];
};

#endif
