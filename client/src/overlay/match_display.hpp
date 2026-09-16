#ifndef MATCH_DISPLAY_HPP
#define MATCH_DISPLAY_HPP

#include <toml.hpp>

class MatchDisplay
{
public:
    MatchDisplay();

    void DrawMenu();
    void Render(float screenWidth, float screenHeight);
    void SaveConfig(toml::table &table) const;
    void LoadConfig(const toml::table &table);

private:
    bool m_enabled;
    int m_position;
    bool m_showApiKey;
    char m_apiKey[256];
};

#endif
