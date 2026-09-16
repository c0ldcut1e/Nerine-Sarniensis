#include "car_hitbox.hpp"

#include <vector>

#include "SdkHeaders.hpp"
#include "overlay/hitbox_renderer.hpp"
#include "reflection/car.hpp"
#include "reflection/car_list.hpp"
#include "reflection/local_player.hpp"

#include <imgui.h>

CarHitbox::CarHitbox()
{
    m_enabled                       = true;
    m_showEveryCar                  = false;
    m_showWheels                    = true;
    m_lineWidth                     = 2.0f;
    m_color[0]                      = 1.0f;
    m_color[1]                      = 1.0f;
    m_color[2]                      = 0.0f;
    m_color[3]                      = 0.9f;
    m_supersonicColorEnabled        = false;
    m_excludeLocalCarFromSupersonic = false;
    m_supersonicColor[0]            = 0.0f;
    m_supersonicColor[1]            = 0.6f;
    m_supersonicColor[2]            = 1.0f;
    m_supersonicColor[3]            = 0.9f;
}

void CarHitbox::DrawMenu()
{
    if (!ImGui::CollapsingHeader("Car Hitboxes", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("Enabled", &m_enabled);

    if (!m_enabled)
    {
        return;
    }

    ImGui::Checkbox("Show Every Car", &m_showEveryCar);
    ImGui::Checkbox("Show Wheels", &m_showWheels);
    ImGui::SliderFloat("Line Width", &m_lineWidth, 1.0f, 10.0f);
    ImGui::ColorEdit4("Color", m_color);
    ImGui::Checkbox("Supersonic Color", &m_supersonicColorEnabled);
    ImGui::Checkbox("Exclude Own Car", &m_excludeLocalCarFromSupersonic);
    ImGui::ColorEdit4("Supersonic Car Color", m_supersonicColor);
}

void CarHitbox::Render(float screenWidth, float screenHeight)
{
    if (!m_enabled)
    {
        return;
    }

    LocalPlayer player = LocalPlayer::Find();
    if (!player)
    {
        return;
    }

    FTPOV pov = player.GetCameraPOV();

    Car localCar = player.GetCar();
    if (!localCar)
    {
        return;
    }

    ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(m_color[0], m_color[1], m_color[2], m_color[3]));
    ImU32 supersonicColor =
            ImGui::ColorConvertFloat4ToU32(ImVec4(m_supersonicColor[0], m_supersonicColor[1], m_supersonicColor[2], m_supersonicColor[3]));

    if (!m_showEveryCar)
    {
        FMatrix transform = localCar.GetRenderTransform();

        FVector collisionExtent = localCar.GetLocalCollisionExtent();
        bool useSupersonicColor = m_supersonicColorEnabled && !m_excludeLocalCarFromSupersonic && localCar.IsSupersonic();
        ImU32 carColor          = useSupersonicColor ? supersonicColor : color;

        HitboxRenderer::DrawBox(transform, collisionExtent.X, collisionExtent.Y, collisionExtent.Z, localCar.GetLocalCollisionOffset(), pov,
                                screenWidth, screenHeight, carColor, m_lineWidth);

        if (m_showWheels)
        {
            for (int i = 0; i < localCar.GetWheelCount(); i++)
            {
                HitboxRenderer::DrawWheel(transform, localCar.GetWheelLocalRestPosition(i), localCar.GetWheelRadius(i), pov, screenWidth,
                                          screenHeight, carColor, m_lineWidth);
            }
        }

        return;
    }

    std::vector<Car> cars = CarList::GetAll();

    for (const Car &car : cars)
    {
        FMatrix transform       = car.GetRenderTransform();
        FVector collisionExtent = car.GetLocalCollisionExtent();
        bool isLocalCar         = car.GetAddress() == localCar.GetAddress();
        bool useSupersonicColor = m_supersonicColorEnabled && !(isLocalCar && m_excludeLocalCarFromSupersonic) && car.IsSupersonic();
        ImU32 carColor          = useSupersonicColor ? supersonicColor : color;

        HitboxRenderer::DrawBox(transform, collisionExtent.X, collisionExtent.Y, collisionExtent.Z, car.GetLocalCollisionOffset(), pov, screenWidth,
                                screenHeight, carColor, m_lineWidth);

        if (m_showWheels)
        {
            for (int i = 0; i < car.GetWheelCount(); i++)
            {
                HitboxRenderer::DrawWheel(transform, car.GetWheelLocalRestPosition(i), car.GetWheelRadius(i), pov, screenWidth, screenHeight,
                                          carColor, m_lineWidth);
            }
        }
    }
}

void CarHitbox::SaveConfig(toml::table &table) const
{
    table.insert_or_assign("enabled", m_enabled);
    table.insert_or_assign("show_every_car", m_showEveryCar);
    table.insert_or_assign("show_wheels", m_showWheels);
    table.insert_or_assign("line_width", m_lineWidth);
    table.insert_or_assign("color_r", m_color[0]);
    table.insert_or_assign("color_g", m_color[1]);
    table.insert_or_assign("color_b", m_color[2]);
    table.insert_or_assign("color_a", m_color[3]);
    table.insert_or_assign("supersonic_color_enabled", m_supersonicColorEnabled);
    table.insert_or_assign("exclude_local_car_from_supersonic", m_excludeLocalCarFromSupersonic);
    table.insert_or_assign("supersonic_color_r", m_supersonicColor[0]);
    table.insert_or_assign("supersonic_color_g", m_supersonicColor[1]);
    table.insert_or_assign("supersonic_color_b", m_supersonicColor[2]);
    table.insert_or_assign("supersonic_color_a", m_supersonicColor[3]);
}

void CarHitbox::LoadConfig(const toml::table &table)
{
    m_enabled                       = table["enabled"].value_or(m_enabled);
    m_showEveryCar                  = table["show_every_car"].value_or(m_showEveryCar);
    m_showWheels                    = table["show_wheels"].value_or(m_showWheels);
    m_lineWidth                     = table["line_width"].value_or(m_lineWidth);
    m_color[0]                      = table["color_r"].value_or(m_color[0]);
    m_color[1]                      = table["color_g"].value_or(m_color[1]);
    m_color[2]                      = table["color_b"].value_or(m_color[2]);
    m_color[3]                      = table["color_a"].value_or(m_color[3]);
    m_supersonicColorEnabled        = table["supersonic_color_enabled"].value_or(m_supersonicColorEnabled);
    m_excludeLocalCarFromSupersonic = table["exclude_local_car_from_supersonic"].value_or(m_excludeLocalCarFromSupersonic);
    m_supersonicColor[0]            = table["supersonic_color_r"].value_or(m_supersonicColor[0]);
    m_supersonicColor[1]            = table["supersonic_color_g"].value_or(m_supersonicColor[1]);
    m_supersonicColor[2]            = table["supersonic_color_b"].value_or(m_supersonicColor[2]);
    m_supersonicColor[3]            = table["supersonic_color_a"].value_or(m_supersonicColor[3]);
}
