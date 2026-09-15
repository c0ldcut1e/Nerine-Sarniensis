#ifndef CAR_HITBOX_HPP
#define CAR_HITBOX_HPP

#include <toml.hpp>

class CarHitbox
{
public:
    CarHitbox();

    void DrawMenu();
    void Render(float screenWidth, float screenHeight);
    void SaveConfig(toml::table &table) const;
    void LoadConfig(const toml::table &table);

private:
    bool m_enabled;
    bool m_showEveryCar;
    float m_color[4];
};

#endif
