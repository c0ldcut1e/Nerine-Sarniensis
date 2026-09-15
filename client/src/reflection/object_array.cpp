#include "object_array.hpp"

#include "nerine_globals.hpp"
#include "nerine_memory.hpp"
#include "offsets.hpp"

constexpr int kMaxPlausibleObjectCount = 2000000;

int ObjectArray::GetCount()
{
    uint64_t base = NerineGlobals::GetInstance().GetModuleBase();

    return ReadMemory<int>(base + g_objectsOffset + 0x8);
}

uint64_t ObjectArray::GetData()
{
    uint64_t base = NerineGlobals::GetInstance().GetModuleBase();

    return ReadMemory<uint64_t>(base + g_objectsOffset);
}

RemoteObject ObjectArray::GetObjectAt(uint64_t data, int index)
{
    uint64_t obj = ReadMemory<uint64_t>(data + index * 8);

    return RemoteObject(obj);
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

uint64_t ObjectArray::FindClassByName(const std::string &className)
{
    std::vector<uint64_t> addresses = GetAllObjectAddresses();

    for (size_t i = 0; i < addresses.size(); i++)
    {
        RemoteObject object(addresses[i]);

        if (object.IsValidAddress() && object.GetName() == className)
        {
            return object.GetAddress();
        }
    }

    return 0;
}

RemoteObject ObjectArray::FindFirstInstanceOf(uint64_t classAddress)
{
    std::vector<uint64_t> addresses = GetAllObjectAddresses();

    for (size_t i = 0; i < addresses.size(); i++)
    {
        RemoteObject object(addresses[i]);

        if (object.IsValidInstanceOf(classAddress))
        {
            return object;
        }
    }

    return RemoteObject();
}
