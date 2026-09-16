#ifndef CAR_BOOST_AMOUNTS_HPP
#define CAR_BOOST_AMOUNTS_HPP

#include <toml.hpp>

class CarBoostAmounts
{
public:
    CarBoostAmounts();

    void DrawMenu();
    void Render(float screenWidth, float screenHeight);
    void SaveConfig(toml::table &table) const;
    void LoadConfig(const toml::table &table);

private:
    bool m_enabled;
    float m_circleRadius;
    float m_heightOffset;
};

#endif
