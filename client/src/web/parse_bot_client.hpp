#ifndef PARSE_BOT_CLIENT_HPP
#define PARSE_BOT_CLIENT_HPP

#include <string>

struct PlayerRankInfo
{
    bool found;
    std::string rank;
    std::string division;
    int mmr;
};

class ParseBotClient
{
public:
    static PlayerRankInfo LookupRank(const std::string &apiKey, const std::string &playerName, const std::string &playlistName);
};

#endif
