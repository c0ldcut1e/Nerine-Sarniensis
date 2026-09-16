#include "car_list.hpp"

#include <vector>

#include "background_scanner.hpp"
#include "match.hpp"
#include "core/nerine_memory.hpp"
#include "offset_resolver.hpp"

constexpr int kScanIntervalMilliseconds = 50;
constexpr int kMaxPlausibleCarCount     = 64;

static std::vector<uint64_t> ScanForLiveCarAddresses()
{
    uint64_t gameEvent = Match::GetGameEvent();
    if (!gameEvent)
    {
        return {};
    }

    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t gameEventClass        = offsetResolver->GetClassAddress("GameEvent_TA");
    uint64_t carsOffset            = offsetResolver->GetPropertyOffsetCached(gameEventClass, "Cars");
    int count                      = ReadMemory<int>(gameEvent + carsOffset + 0x8);
    if (count <= 0 || count > kMaxPlausibleCarCount)
    {
        return {};
    }

    return ReadMemoryArray<uint64_t>(ReadMemory<uint64_t>(gameEvent + carsOffset), (size_t) count);
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
