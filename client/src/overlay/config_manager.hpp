#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <filesystem>
#include <limits.h>
#include <string>
#include <vector>

#include "overlay/ball_prediction.hpp"
#include "overlay/boost_pad_timers.hpp"
#include "overlay/car_boost_amounts.hpp"
#include "overlay/car_hitbox.hpp"
#include "overlay/demo_markers.hpp"
#include "overlay/match_display.hpp"
#include "overlay/offscreen_indicators.hpp"
#include "overlay/radar.hpp"

#include <gtk/gtk.h>

class ConfigManager
{
public:
    ConfigManager();

    void Initialize(BallPrediction &ballPrediction, CarBoostAmounts &carBoostAmounts, CarHitbox &carHitbox, BoostPadTimers &boostPadTimers,
                    Radar &radar, MatchDisplay &matchDisplay, OffscreenIndicators &offscreenIndicators, DemoMarkers &demoMarkers);
    void DrawMenu();

private:
    static void OnCollisionMeshesDirectorySelected(GObject *source, GAsyncResult *result, gpointer data);
    static void OnConfigDirectorySelected(GObject *source, GAsyncResult *result, gpointer data);

    void ApplyConfigDirectory();
    void ApplyCollisionMeshesDirectory();
    void DeleteConfig();
    void LoadConfigDirectory();
    void LoadConfig();
    void RefreshConfigs();
    void SaveConfig() const;
    void SaveConfigDirectory() const;
    void SelectCollisionMeshesDirectory();
    void SelectConfigDirectory();

    std::filesystem::path m_executableDirectory;
    BallPrediction *m_ballPrediction;
    CarBoostAmounts *m_carBoostAmounts;
    CarHitbox *m_carHitbox;
    BoostPadTimers *m_boostPadTimers;
    Radar *m_radar;
    MatchDisplay *m_matchDisplay;
    OffscreenIndicators *m_offscreenIndicators;
    DemoMarkers *m_demoMarkers;
    char m_configDirectory[PATH_MAX];
    char m_collisionMeshesDirectory[PATH_MAX];
    char m_profileName[128];
    char m_profileNameInput[128];
    std::vector<std::string> m_configNames;
};

#endif
