#include "car_list.hpp"

#include <vector>

#include "background_scanner.hpp"
#include "match.hpp"
#include "nerine_memory.hpp"
#include "offsets.hpp"

#include <fmt/base.h>

constexpr int kScanIntervalMilliseconds = 50;
constexpr int32_t kMaxPlausibleCarCount = 64;

static std::vector<uint64_t> ScanForLiveCarAddresses()
{
    uint64_t gameEvent = Match::GetGameEvent();

    if (!gameEvent)
    {
        return {};
    }

    int32_t count = ReadMemory<int32_t>(gameEvent + kGameEventCarsOffset + 0x8);

    if (count <= 0 || count > kMaxPlausibleCarCount)
    {
        return {};
    }

    return ReadMemoryArray<uint64_t>(ReadMemory<uint64_t>(gameEvent + kGameEventCarsOffset), (size_t) count);
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

void CarList::Debug()
{
    std::vector<Car> cars = GetAll();

    fmt::println("--- CarList::Debug ---");
    fmt::println("car count = {}", cars.size());

    for (size_t i = 0; i < cars.size(); i++)
    {
        uint64_t boostComponent  = ReadMemory<uint64_t>(cars[i].GetAddress() + kCarBoostComponentOffset);
        FVector location         = cars[i].GetLocation();
        float maxAmount          = ReadMemory<float>(boostComponent + kBoostMaxAmountOffset);
        float currentAmount      = ReadMemory<float>(boostComponent + kBoostCurrentAmountOffset);
        uint8_t replicatedAmount = ReadMemory<uint8_t>(boostComponent + kBoostReplicatedAmountOffset);

        fmt::println("car #{} at {:#x}, location ({:.0f}, {:.0f}, {:.0f}), boost component {:#x}, max {:.2f}, current {:.2f} ({:.2f}%), "
                     "replicated {} ({:.2f}%)",
                     i, cars[i].GetAddress(), location.X, location.Y, location.Z, boostComponent, maxAmount, currentAmount,
                     currentAmount * 100.0f / maxAmount, replicatedAmount, replicatedAmount * 100.0f / 255.0f);
    }
}
