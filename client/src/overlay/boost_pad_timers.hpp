#ifndef BOOST_PAD_TIMERS_HPP
#define BOOST_PAD_TIMERS_HPP

#include <chrono>
#include <cstdint>
#include <unordered_map>

#include <toml.hpp>

class BoostPadTimers
{
public:
    BoostPadTimers();

    void DrawMenu();
    void Render(float screenWidth, float screenHeight);
    void SaveConfig(toml::table &table) const;
    void LoadConfig(const toml::table &table);

private:
    bool m_enabled;
    float m_circleRadius;
    std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> m_pickupTimestamps;
    std::unordered_map<uint64_t, uint8_t> m_lastPickupStates;
};

#endif
