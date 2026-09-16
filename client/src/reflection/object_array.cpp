#include "object_array.hpp"

#include "core/nerine_globals.hpp"
#include "core/nerine_memory.hpp"
#include "offset_resolver.hpp"

constexpr int kMaxPlausibleObjectCount = 2000000;

int ObjectArray::GetCount()
{
    uint64_t base = NerineGlobals::GetInstance()->GetModuleBase();
    return ReadMemory<int>(base + OffsetResolver::GetInstance()->GetGObjectsOffset() + 0x8);
}

uint64_t ObjectArray::GetData()
{
    uint64_t base = NerineGlobals::GetInstance()->GetModuleBase();
    return ReadMemory<uint64_t>(base + OffsetResolver::GetInstance()->GetGObjectsOffset());
}

std::vector<uint64_t> ObjectArray::GetAllObjectAddresses()
{
    int count = GetCount();
    if (count <= 0 || count > kMaxPlausibleObjectCount)
    {
        return {};
    }

    uint64_t data = GetData();
    return ReadMemoryArray<uint64_t>(data, (size_t) count);
}
