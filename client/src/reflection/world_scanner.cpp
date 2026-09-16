#include "world_scanner.hpp"

#include <chrono>
#include <mutex>
#include <thread>

#include "boost_pad.hpp"
#include "car.hpp"
#include "core/nerine_memory.hpp"
#include "object_array.hpp"
#include "offset_resolver.hpp"
#include "remote_object.hpp"

#include <fmt/core.h>

constexpr int kScanIntervalMilliseconds     = 50;
constexpr int kFullScanIntervalMilliseconds = 1000;
constexpr int kMaxPlausibleBallCount        = 32;

static uint64_t g_gameEventClassAddress;
static uint64_t g_playerClassAddress;
static uint64_t g_boostPadClassAddress;

static uint64_t g_scannedBallAddress;
static uint64_t g_scannedGameEventAddress;
static uint64_t g_scannedLocalPlayerAddress;
static std::vector<uint64_t> g_scannedBoostPadAddresses;
static uint64_t g_boostPadScanGameEvent;

static std::mutex g_mutex;
static uint64_t g_ballAddress;
static uint64_t g_gameEventAddress;
static uint64_t g_localPlayerAddress;
static std::vector<uint64_t> g_boostPadAddresses;

static void ResolveClassAddresses()
{
    if (!g_gameEventClassAddress)
    {
        g_gameEventClassAddress = OffsetResolver::GetInstance()->TryGetClassAddress("GameEvent_Soccar_TA");
    }

    if (!g_playerClassAddress)
    {
        g_playerClassAddress = OffsetResolver::GetInstance()->TryGetClassAddress("PlayerController_TA");
    }

    if (!g_boostPadClassAddress)
    {
        g_boostPadClassAddress = OffsetResolver::GetInstance()->TryGetClassAddress("VehiclePickup_Boost_TA");
    }
}

static bool IsLocalPlayerController(uint64_t address)
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    return ReadMemory<uint64_t>(address + offsetResolver->GetPropertyOffsetCached(g_playerClassAddress, "Player")) != 0;
}

static uint64_t ReadFirstGameBall()
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t ballsOffset           = offsetResolver->GetPropertyOffsetCached(g_gameEventClassAddress, "GameBalls");
    int count                      = ReadMemory<int>(g_scannedGameEventAddress + ballsOffset + sizeof(uint64_t));

    if (count <= 0 || count > kMaxPlausibleBallCount)
    {
        return 0;
    }

    return ReadMemory<uint64_t>(ReadMemory<uint64_t>(g_scannedGameEventAddress + ballsOffset));
}

static void DeriveWorldState()
{
    if (g_scannedLocalPlayerAddress &&
        (!RemoteObject(g_scannedLocalPlayerAddress).IsValidInstanceOf(g_playerClassAddress) || !IsLocalPlayerController(g_scannedLocalPlayerAddress)))
    {
        fmt::println("[WorldScanner] Local player controller {:#x} is no longer valid", g_scannedLocalPlayerAddress);
        g_scannedLocalPlayerAddress = 0;
    }

    g_scannedGameEventAddress = 0;
    g_scannedBallAddress      = 0;

    if (!g_scannedLocalPlayerAddress || !g_gameEventClassAddress)
    {
        return;
    }

    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t gameEvent =
            ReadMemory<uint64_t>(g_scannedLocalPlayerAddress + offsetResolver->GetPropertyOffsetCached(g_playerClassAddress, "SoccarGameEvent"));
    if (!gameEvent || !RemoteObject(gameEvent).IsValidInstanceOf(g_gameEventClassAddress))
    {
        return;
    }

    g_scannedGameEventAddress = gameEvent;
    g_scannedBallAddress      = ReadFirstGameBall();
}

static void FullScan()
{
    std::vector<uint64_t> addresses = ObjectArray::GetAllObjectAddresses();

    Car::ClearStaticCache();
    BoostPad::ClearStaticCache();
    g_scannedBoostPadAddresses.clear();

    for (size_t i = 0; i < addresses.size(); i++)
    {
        RemoteObject object(addresses[i]);
        uint64_t classAddress = object.GetClassAddress();
        if (g_playerClassAddress && classAddress == g_playerClassAddress && !g_scannedLocalPlayerAddress &&
            object.IsValidInstanceOf(g_playerClassAddress) && IsLocalPlayerController(object.GetAddress()))
        {
            g_scannedLocalPlayerAddress = object.GetAddress();
        }
        else if (g_boostPadClassAddress && classAddress == g_boostPadClassAddress && object.IsValidInstanceOf(g_boostPadClassAddress))
        {
            g_scannedBoostPadAddresses.push_back(object.GetAddress());
        }
    }

    fmt::println("[WorldScanner] Full scan of {} objects: local player {:#x}, {} boost pads", addresses.size(), g_scannedLocalPlayerAddress,
                 g_scannedBoostPadAddresses.size());
}

static void LogStateChanges()
{
    static uint64_t lastLocalPlayer;
    static uint64_t lastGameEvent;
    static uint64_t lastBall;

    if (g_scannedLocalPlayerAddress != lastLocalPlayer)
    {
        lastLocalPlayer = g_scannedLocalPlayerAddress;
        fmt::println("[WorldScanner] Local player -> {:#x}", lastLocalPlayer);
    }

    if (g_scannedGameEventAddress != lastGameEvent)
    {
        lastGameEvent = g_scannedGameEventAddress;
        fmt::println("[WorldScanner] Game event -> {:#x}", lastGameEvent);
    }

    if (g_scannedBallAddress != lastBall)
    {
        lastBall = g_scannedBallAddress;
        fmt::println("[WorldScanner] Ball -> {:#x}", lastBall);
    }
}

static void ScanLoop()
{
    std::chrono::steady_clock::time_point lastFullScan;

    while (true)
    {
        ResolveClassAddresses();
        DeriveWorldState();

        if (!g_scannedGameEventAddress && g_boostPadScanGameEvent)
        {
            g_boostPadScanGameEvent = 0;
            g_scannedBoostPadAddresses.clear();
            BoostPad::ClearStaticCache();
            Car::ClearStaticCache();
        }

        bool needsFullScan = !g_scannedLocalPlayerAddress || (g_scannedGameEventAddress && g_scannedGameEventAddress != g_boostPadScanGameEvent);
        if (needsFullScan && std::chrono::steady_clock::now() - lastFullScan >= std::chrono::milliseconds(kFullScanIntervalMilliseconds))
        {
            FullScan();
            DeriveWorldState();
            g_boostPadScanGameEvent = g_scannedGameEventAddress;
            lastFullScan            = std::chrono::steady_clock::now();
        }

        LogStateChanges();

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_ballAddress        = g_scannedBallAddress;
            g_gameEventAddress   = g_scannedGameEventAddress;
            g_localPlayerAddress = g_scannedLocalPlayerAddress;
            g_boostPadAddresses  = g_scannedBoostPadAddresses;
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
