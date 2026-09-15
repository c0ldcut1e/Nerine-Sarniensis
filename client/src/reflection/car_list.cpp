#include "car_list.hpp"

#include <vector>

#include "background_scanner.hpp"
#include "match.hpp"
#include "nerine_memory.hpp"
#include "offsets.hpp"

#include <fmt/base.h>

constexpr int kScanIntervalMilliseconds = 50;
constexpr int kMaxPlausibleCarCount     = 64;

static std::vector<uint64_t> ScanForLiveCarAddresses()
{
    uint64_t gameEvent = Match::GetGameEvent();

    if (!gameEvent)
    {
        return {};
    }

    int count = ReadMemory<int>(gameEvent + g_gameEventCarsOffset + 0x8);

    if (count <= 0 || count > kMaxPlausibleCarCount)
    {
        return {};
    }

    return ReadMemoryArray<uint64_t>(ReadMemory<uint64_t>(gameEvent + g_gameEventCarsOffset), (size_t) count);
}

static BackgroundScanner<std::vector<uint64_t>> g_scanner(ScanForLiveCarAddresses, kScanIntervalMilliseconds);

std::vector<FVector> CarList::GetPositions()
{
    std::vector<uint64_t> addresses = g_scanner.Get();
    std::vector<FVector> positions;
    positions.reserve(addresses.size());

    for (size_t i = 0; i < addresses.size(); i++)
    {
        Car car(addresses[i]);
        positions.push_back(car.GetLocation());
    }

    return positions;
}

std::vector<Car> CarList::GetAll()
{
    std::vector<uint64_t> addresses = g_scanner.Get();
    std::vector<Car> cars;
    cars.reserve(addresses.size());

    for (size_t i = 0; i < addresses.size(); i++)
    {
        cars.push_back(Car(addresses[i]));
    }

    return cars;
}
