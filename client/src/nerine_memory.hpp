#ifndef NERINE_MEMORY_HPP
#define NERINE_MEMORY_HPP

#include <cstdint>
#include <vector>

#include "nerine_globals.hpp"

template<typename T>
T ReadMemory(uint64_t address)
{
    T value = {};

    if (!NerineGlobals::GetInstance().GetDevice().IsOpen())
    {
        return value;
    }

    NerineGlobals::GetInstance().GetDevice().ReadMemory(NerineGlobals::GetInstance().GetPid(), address, &value, sizeof(T));
    return value;
}

template<typename T>
std::vector<T> ReadMemoryArray(uint64_t address, size_t count)
{
    std::vector<T> values(count);

    if (!NerineGlobals::GetInstance().GetDevice().IsOpen())
    {
        return values;
    }

    constexpr size_t kChunkElements = 4096;
    size_t offset                   = 0;

    while (offset < count)
    {
        size_t chunkCount = count - offset;

        if (chunkCount > kChunkElements)
        {
            chunkCount = kChunkElements;
        }

        NerineGlobals::GetInstance().GetDevice().ReadMemory(NerineGlobals::GetInstance().GetPid(), address + offset * sizeof(T),
                                                            values.data() + offset, chunkCount * sizeof(T));

        offset += chunkCount;
    }

    return values;
}

#endif
