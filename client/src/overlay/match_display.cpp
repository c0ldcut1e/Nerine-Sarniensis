#include "match_display.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

#include "overlay/screen_position.hpp"
#include "reflection/background_scanner.hpp"
#include "reflection/car.hpp"
#include "reflection/car_list.hpp"
#include "reflection/local_player.hpp"
#include "reflection/match.hpp"
#include "web/parse_bot_client.hpp"

#include <fmt/core.h>
#include <imgui.h>

constexpr int kScanIntervalMilliseconds = 5000;
constexpr float kMargin                 = 20.0f;
constexpr float kPadding                = 8.0f;
constexpr float kLineSpacing            = 4.0f;

struct EnemyRankEntry
{
    std::string playerName;
    PlayerRankInfo rankInfo;
};

static std::string g_apiKey;
static std::string g_playlistName;
static uint64_t g_lastGameEvent;
static std::unordered_map<std::string, PlayerRankInfo> g_rankCache;

static std::vector<EnemyRankEntry> ScanEnemyRanks()
{
    std::vector<EnemyRankEntry> entries;
    uint64_t gameEvent = Match::GetGameEvent();

    if (!gameEvent)
    {
        return entries;
    }

    if (gameEvent != g_lastGameEvent)
    {
        g_lastGameEvent = gameEvent;
        g_rankCache.clear();
        fmt::println("[MatchDisplay] New match detected, cleared rank cache");
    }

    if (g_apiKey.empty())
    {
        fmt::println("[MatchDisplay] No parse.bot API key set");
        return entries;
    }

    LocalPlayer player = LocalPlayer::Find();
    Car localCar       = player.GetCar();
    if (!player || !localCar)
    {
        fmt::println("[MatchDisplay] Local player or car not available yet");
        return entries;
    }

    int localTeam            = localCar.GetTeam();
    std::string playlistName = g_playlistName;
    std::vector<Car> cars    = CarList::GetAll();

    fmt::println("[MatchDisplay] Scanning {} cars (local team {}, playlist '{}')", cars.size(), localTeam, playlistName);

    for (const Car &car : cars)
    {
        if (car.GetTeam() == localTeam)
        {
            continue;
        }

        EnemyRankEntry entry;
        entry.playerName = car.GetPlayerName();
        if (entry.playerName.empty())
        {
            fmt::println("[MatchDisplay] Skipping enemy car {:#x} with empty player name", car.GetAddress());
            continue;
        }

        std::string cacheKey                                             = entry.playerName + "|" + playlistName;
        std::unordered_map<std::string, PlayerRankInfo>::iterator cached = g_rankCache.find(cacheKey);
        if (cached != g_rankCache.end())
        {
            entry.rankInfo = cached->second;
        }
        else
        {
            fmt::println("[MatchDisplay] Looking up enemy '{}'", entry.playerName);
            entry.rankInfo = ParseBotClient::LookupRank(g_apiKey, entry.playerName, playlistName);
            g_rankCache.emplace(cacheKey, entry.rankInfo);
            fmt::println("[MatchDisplay] '{}' found={} rank='{}' division='{}' mmr={}", entry.playerName, entry.rankInfo.found, entry.rankInfo.rank,
                         entry.rankInfo.division, entry.rankInfo.mmr);
        }

        if (entry.rankInfo.found)
        {
            entries.push_back(entry);
        }
    }

    fmt::println("[MatchDisplay] {} enemy entries resolved", entries.size());
    return entries;
}

static BackgroundScanner<std::vector<EnemyRankEntry>> g_scanner(ScanEnemyRanks, kScanIntervalMilliseconds);

MatchDisplay::MatchDisplay()
{
    m_enabled    = true;
    m_position   = 2;
    m_showApiKey = false;
    m_apiKey[0]  = '\0';
}

void MatchDisplay::DrawMenu()
{
    if (!ImGui::CollapsingHeader("Match Display", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("Enabled", &m_enabled);

    if (!m_enabled)
    {
        return;
    }

    ImGui::Combo("Position", &m_position, "Top Left\0Top Center\0Top Right\0Left Center\0Right Center\0Bottom Left\0Bottom Center\0Bottom Right\0");

    ImGuiInputTextFlags apiKeyFlags = m_showApiKey ? 0 : ImGuiInputTextFlags_Password;
    if (ImGui::InputText("Parse.bot API Key", m_apiKey, sizeof(m_apiKey), apiKeyFlags))
    {
        g_apiKey = m_apiKey;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Show API Key", &m_showApiKey);
}

void MatchDisplay::Render(float screenWidth, float screenHeight)
{
    if (!m_enabled || !Match::IsActive())
    {
        return;
    }

    std::string playlistName = Match::GetPlaylistName();
    if (playlistName != g_playlistName)
    {
        g_playlistName = playlistName;
        fmt::println("[MatchDisplay] Game reports playlist '{}'", g_playlistName);
    }

    std::vector<std::string> lines;
    lines.push_back("Playlist: " + (g_playlistName.empty() ? std::string("Unknown") : g_playlistName));

    for (const EnemyRankEntry &entry : g_scanner.Get())
    {
        lines.push_back(entry.playerName + ": " + entry.rankInfo.rank + " " + entry.rankInfo.division + " (" + std::to_string(entry.rankInfo.mmr) +
                        " MMR)");
    }

    float boxWidth  = 0.0f;
    float boxHeight = kPadding * 2.0f;

    for (const std::string &line : lines)
    {
        ImVec2 lineSize = ImGui::CalcTextSize(line.c_str());
        boxWidth        = std::max(boxWidth, lineSize.x);
        boxHeight += lineSize.y + kLineSpacing;
    }

    boxWidth += kPadding * 2.0f;

    ImVec2 boxSize(boxWidth, boxHeight);
    ImVec2 boxMin = ScreenPosition::Resolve(m_position, boxSize, screenWidth, screenHeight, kMargin);

    ImDrawList *drawList = ImGui::GetBackgroundDrawList();
    drawList->AddRectFilled(boxMin, ImVec2(boxMin.x + boxSize.x, boxMin.y + boxSize.y), IM_COL32(0, 0, 0, 180));

    float textY = boxMin.y + kPadding;
    for (const std::string &line : lines)
    {
        drawList->AddText(ImVec2(boxMin.x + kPadding, textY), IM_COL32_WHITE, line.c_str());
        textY += ImGui::CalcTextSize(line.c_str()).y + kLineSpacing;
    }
}

void MatchDisplay::SaveConfig(toml::table &table) const
{
    table.insert_or_assign("enabled", m_enabled);
    table.insert_or_assign("position", m_position);
    table.insert_or_assign("api_key", std::string(m_apiKey));
}

void MatchDisplay::LoadConfig(const toml::table &table)
{
    m_enabled          = table["enabled"].value_or(m_enabled);
    m_position         = table["position"].value_or(m_position);
    std::string apiKey = table["api_key"].value_or(std::string(m_apiKey));

    snprintf(m_apiKey, sizeof(m_apiKey), "%s", apiKey.c_str());
    g_apiKey = m_apiKey;
}
