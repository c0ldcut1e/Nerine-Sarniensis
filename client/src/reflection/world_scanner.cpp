#include "world_scanner.hpp"

#include <chrono>
#include <mutex>
#include <thread>

#include "ball.hpp"
#include "object_array.hpp"
#include "remote_object.hpp"

constexpr int kScanIntervalMilliseconds = 50;
constexpr float kMinPlausibleBallZ      = 1.0f;

static uint64_t g_ballClassAddress;
static uint64_t g_gameEventClassAddress;
static uint64_t g_playerClassAddress;
static uint64_t g_boostPadClassAddress;

static std::mutex g_mutex;
static uint64_t g_ballAddress;
static uint64_t g_gameEventAddress;
static uint64_t g_localPlayerAddress;
static std::vector<uint64_t> g_boostPadAddresses;

static void ScanLoop()
{
    while (true)
    {
        if (!g_ballClassAddress)
        {
            g_ballClassAddress = ObjectArray::FindClassByName("Ball_TA");
        }

        if (!g_gameEventClassAddress)
        {
            g_gameEventClassAddress = ObjectArray::FindClassByName("GameEvent_Soccar_TA");
        }

        if (!g_playerClassAddress)
        {
            g_playerClassAddress = ObjectArray::FindClassByName("PlayerController_TA");
        }

        if (!g_boostPadClassAddress)
        {
            g_boostPadClassAddress = ObjectArray::FindClassByName("VehiclePickup_Boost_TA");
        }

        std::vector<uint64_t> addresses = ObjectArray::GetAllObjectAddresses();
        uint64_t ballFallback           = 0;
        uint64_t ballAddress            = 0;
        uint64_t gameEventAddress       = 0;
        uint64_t playerAddress          = 0;
        std::vector<uint64_t> boostPadAddresses;

        for (size_t i = 0; i < addresses.size(); i++)
        {
            RemoteObject object(addresses[i]);
            uint64_t classAddress = object.GetClassAddress();
            if (g_ballClassAddress && classAddress == g_ballClassAddress && object.IsValidInstanceOf(g_ballClassAddress))
            {
                if (!ballFallback)
                {
                    ballFallback = object.GetAddress();
                }

                if (!ballAddress)
                {
                    Ball candidate(object.GetAddress());

                    if (candidate.GetLocation().Z >= kMinPlausibleBallZ)
                    {
                        ballAddress = object.GetAddress();
                    }
                }
            }
            else if (g_gameEventClassAddress && classAddress == g_gameEventClassAddress && !gameEventAddress &&
                     object.IsValidInstanceOf(g_gameEventClassAddress))
            {
                gameEventAddress = object.GetAddress();
            }
            else if (g_playerClassAddress && classAddress == g_playerClassAddress && !playerAddress && object.IsValidInstanceOf(g_playerClassAddress))
            {
                playerAddress = object.GetAddress();
            }
            else if (g_boostPadClassAddress && classAddress == g_boostPadClassAddress && object.IsValidInstanceOf(g_boostPadClassAddress))
            {
                boostPadAddresses.push_back(object.GetAddress());
            }
        }

        if (!ballAddress)
        {
            ballAddress = ballFallback;
        }

        if (!gameEventAddress)
        {
            ballAddress = 0;
        }

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_ballAddress        = ballAddress;
            g_gameEventAddress   = gameEventAddress;
            g_localPlayerAddress = playerAddress;
            g_boostPadAddresses  = boostPadAddresses;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(kScanIntervalMilliseconds));
    }
}

static void StartThreadIfNeeded()
{
    static bool started = false;
    if (!started)
    {
        started = true;
        std::thread(ScanLoop).detach();
    }
}

uint64_t WorldScanner::GetBallAddress()
{
    StartThreadIfNeeded();
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_ballAddress;
}

uint64_t WorldScanner::GetGameEventAddress()
{
    StartThreadIfNeeded();
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_gameEventAddress;
}

uint64_t WorldScanner::GetLocalPlayerAddress()
{
    StartThreadIfNeeded();
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_localPlayerAddress;
}

std::vector<uint64_t> WorldScanner::GetBoostPadAddresses()
{
    StartThreadIfNeeded();
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_boostPadAddresses;
}
