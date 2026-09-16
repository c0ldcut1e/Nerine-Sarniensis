#include "parse_bot_client.hpp"

#include <vector>

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include "web/http_client.hpp"

constexpr const char *kMmrUrl = "https://api.parse.bot/scraper/43963aeb-490b-4485-a43a-7d4a7c09a5ee/get_player_mmr";

static bool PlaylistMatches(const std::string &playlistDisplayName, const std::string &gamePlaylistName)
{
    if (playlistDisplayName.empty() || gamePlaylistName.empty())
    {
        return false;
    }

    return playlistDisplayName.find(gamePlaylistName) != std::string::npos || gamePlaylistName.find(playlistDisplayName) != std::string::npos;
}

static PlayerRankInfo ParseMmrResponse(const std::string &body, const std::string &playlistName)
{
    PlayerRankInfo info;
    info.found = false;
    info.mmr   = 0;

    nlohmann::json root = nlohmann::json::parse(body, nullptr, false);
    if (root.is_discarded())
    {
        fmt::println("[ParseBot] Response was not valid JSON: {}", body.substr(0, 200));
        return info;
    }

    if (root.value("status", "") != "success")
    {
        fmt::println("[ParseBot] Response status not success: {}", body.substr(0, 200));
        return info;
    }

    nlohmann::json data = root.value("data", nlohmann::json::object());
    if (!data.contains("playlists") || !data["playlists"].is_array())
    {
        fmt::println("[ParseBot] Response has no playlists array");
        return info;
    }

    for (const nlohmann::json &playlist : data["playlists"])
    {
        std::string playlistDisplayName = playlist.value("playlist_name", "");
        if (!PlaylistMatches(playlistDisplayName, playlistName))
        {
            fmt::println("[ParseBot] Playlist '{}' does not match game playlist '{}'", playlistDisplayName, playlistName);
            continue;
        }

        info.rank     = playlist.value("tier_name", "");
        info.division = playlist.value("division_name", "");
        info.mmr      = playlist.value("mmr", 0);
        info.found    = true;
        return info;
    }

    return info;
}

PlayerRankInfo ParseBotClient::LookupRank(const std::string &apiKey, const std::string &playerName, const std::string &playlistName)
{
    PlayerRankInfo notFound;
    notFound.found = false;
    notFound.mmr   = 0;

    if (playerName.empty())
    {
        return notFound;
    }

    constexpr const char *kPlatforms[] = {"epic", "steam", "xbox", "psn"};
    std::vector<std::string> headers   = {"X-API-Key: " + apiKey};

    for (const char *platform : kPlatforms)
    {
        std::string url       = std::string(kMmrUrl) + "?platform=" + platform + "&identifier=" + HttpClient::UrlEncode(playerName);
        HttpResponse response = HttpClient::Get(url, headers);

        fmt::println("[ParseBot] {} '{}' -> http {} ({} bytes)", platform, playerName, response.statusCode, response.body.size());

        if (!response.success)
        {
            fmt::println("[ParseBot] {} lookup failed: {}", platform, response.body.substr(0, 200));
            continue;
        }

        PlayerRankInfo info = ParseMmrResponse(response.body, playlistName);
        if (info.found)
        {
            return info;
        }
    }

    return notFound;
}
