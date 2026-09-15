#include "offset_resolver.hpp"

#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "name_table.hpp"
#include "nerine_globals.hpp"
#include "nerine_memory.hpp"
#include "object_array.hpp"
#include "remote_object.hpp"
#include "ue3_offsets.hpp"

#include "GameDefines.hpp"
#include "SdkHeaders.hpp"

#include <fmt/core.h>

constexpr size_t kDefaultCacheSizeCap = 128;

uint64_t g_objectsOffset;
uint64_t g_namesOffset;
uint64_t g_buildDateOffset;
uint64_t g_psyonixBuildIdOffset;

OffsetResolver::OffsetResolver()
{
    m_layoutValidated        = false;
    m_offsetResolveStartTime = std::chrono::steady_clock::now();
    m_cacheSizeCap           = kDefaultCacheSizeCap;
}

OffsetResolver &OffsetResolver::GetInstance()
{
    static OffsetResolver instance;
    return instance;
}

uint64_t OffsetResolver::GetGObjectsOffset() const
{
    return g_objectsOffset;
}

uint64_t OffsetResolver::GetGNamesOffset() const
{
    return g_namesOffset;
}

uint64_t OffsetResolver::GetBuildDateOffset() const
{
    return g_buildDateOffset;
}

uint64_t OffsetResolver::GetGPsyonixBuildIdOffset() const
{
    return g_psyonixBuildIdOffset;
}

bool OffsetResolver::Resolve()
{
    uint64_t moduleBase = NerineGlobals::GetInstance().GetModuleBase();
    if (g_objectsOffset == 0 || g_namesOffset == 0 || g_buildDateOffset == 0 || g_psyonixBuildIdOffset == 0)
    {
        uint32_t peHeaderOffset = ReadMemory<uint32_t>(moduleBase + 0x3C);
        uint32_t moduleSize     = ReadMemory<uint32_t>(moduleBase + peHeaderOffset + 0x50);
        if (moduleSize < 0x100000 || moduleSize > 0x10000000)
        {
            fmt::println(stderr, "Failed to read the RocketLeague.exe module size");
            exit(EXIT_FAILURE);
        }

        std::vector<uint8_t> module = ReadMemoryArray<uint8_t>(moduleBase, moduleSize);
        if (g_objectsOffset == 0)
        {
            uint8_t pattern[] = {0x48, 0x8B, 0xC8, 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x0C, 0xC8};
            size_t match      = FindPattern(module, pattern, "xxxxxx????xxxx");
            if (match != module.size())
            {
                int displacement;
                memcpy(&displacement, module.data() + match + 6, sizeof(displacement));

                int64_t offset = (int64_t) match + 10 + displacement;
                if (offset > 0 && (uint64_t) offset < module.size())
                {
                    g_objectsOffset = (uint64_t) offset;
                }
            }
        }

        if (g_namesOffset == 0)
        {
            uint8_t pattern[] = {0x48, 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x0C, 0xC1};
            size_t match      = FindPattern(module, pattern, "xxx????xxxx");
            if (match == module.size())
            {
                uint8_t fallbackPattern[] = {0x49, 0x63, 0x06, 0x48, 0x8D, 0x55, 0xE8, 0x48, 0x8B,
                                             0x0D, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x0C, 0xC1};
                match                     = FindPattern(module, fallbackPattern, "xxxxxxxxxx????xxxx");
                if (match != module.size())
                {
                    int displacement;
                    memcpy(&displacement, module.data() + match + 10, sizeof(displacement));
                    int64_t offset = (int64_t) match + 14 + displacement;
                    if (offset > 0 && (uint64_t) offset < module.size())
                    {
                        g_namesOffset = (uint64_t) offset;
                    }
                }
            }
            else
            {
                int displacement;
                memcpy(&displacement, module.data() + match + 3, sizeof(displacement));
                int64_t offset = (int64_t) match + 7 + displacement;
                if (offset > 0 && (uint64_t) offset < module.size())
                {
                    g_namesOffset = (uint64_t) offset;
                }
            }
        }

        if (g_buildDateOffset == 0 || g_psyonixBuildIdOffset == 0)
        {
            for (size_t stringOffset = 0; stringOffset + 20 <= module.size(); stringOffset++)
            {
                if (!isalpha((uint8_t) module[stringOffset]) || !isalpha((uint8_t) module[stringOffset + 1]) ||
                    !isalpha((uint8_t) module[stringOffset + 2]) || module[stringOffset + 3] != ' ' ||
                    !(isdigit((uint8_t) module[stringOffset + 4]) || module[stringOffset + 4] == ' ') ||
                    !isdigit((uint8_t) module[stringOffset + 5]) || module[stringOffset + 6] != ' ' || !isdigit((uint8_t) module[stringOffset + 7]) ||
                    !isdigit((uint8_t) module[stringOffset + 8]) || !isdigit((uint8_t) module[stringOffset + 9]) ||
                    !isdigit((uint8_t) module[stringOffset + 10]) || module[stringOffset + 11] != ' ' ||
                    !isdigit((uint8_t) module[stringOffset + 12]) || !isdigit((uint8_t) module[stringOffset + 13]) ||
                    module[stringOffset + 14] != ':' || !isdigit((uint8_t) module[stringOffset + 15]) ||
                    !isdigit((uint8_t) module[stringOffset + 16]) || module[stringOffset + 17] != ':' ||
                    !isdigit((uint8_t) module[stringOffset + 18]) || !isdigit((uint8_t) module[stringOffset + 19]))
                {
                    continue;
                }

                for (size_t pointerOffset = 0; pointerOffset + 0x38 <= module.size(); pointerOffset += 8)
                {
                    uint64_t buildDateAddress;
                    uint64_t buildIdAddress;
                    memcpy(&buildDateAddress, module.data() + pointerOffset, sizeof(buildDateAddress));
                    if (buildDateAddress != moduleBase + stringOffset)
                    {
                        continue;
                    }

                    memcpy(&buildIdAddress, module.data() + pointerOffset + 0x30, sizeof(buildIdAddress));
                    if (buildIdAddress == 0)
                    {
                        continue;
                    }

                    std::vector<char16_t> buildId = ReadMemoryArray<char16_t>(buildIdAddress, 20);
                    bool validBuildId             = true;
                    for (size_t i = 0; i < buildId.size(); i++)
                    {
                        if (i == 6 || i == 12)
                        {
                            if (buildId[i] != u'.')
                            {
                                validBuildId = false;
                                break;
                            }
                        }
                        else if (i == 19)
                        {
                            if (buildId[i] != 0)
                            {
                                validBuildId = false;
                            }
                        }
                        else if (!isdigit((uint8_t) buildId[i]))
                        {
                            validBuildId = false;
                            break;
                        }
                    }

                    if (validBuildId)
                    {
                        g_buildDateOffset      = pointerOffset;
                        g_psyonixBuildIdOffset = pointerOffset + 0x30;
                        break;
                    }
                }

                if (g_buildDateOffset != 0 && g_psyonixBuildIdOffset != 0)
                {
                    break;
                }
            }
        }
    }

    if (g_objectsOffset == 0 || g_namesOffset == 0 || g_buildDateOffset == 0 || g_psyonixBuildIdOffset == 0)
    {
        if (g_objectsOffset == 0)
        {
            fmt::println(stderr, "Failed to resolve GObjects");
        }

        if (g_namesOffset == 0)
        {
            fmt::println(stderr, "Failed to resolve GNames");
        }

        if (g_buildDateOffset == 0)
        {
            fmt::println(stderr, "Failed to resolve BuildDate");
        }

        if (g_psyonixBuildIdOffset == 0)
        {
            fmt::println(stderr, "Failed to resolve GPsyonixBuildID");
        }

        exit(EXIT_FAILURE);
    }

    if (!IsGObjectsValid() || !IsGNamesValid())
    {
        return false;
    }

    if (!m_layoutValidated)
    {
        if (std::chrono::steady_clock::now() - m_offsetResolveStartTime >= std::chrono::seconds(60))
        {
            fmt::println(stderr, "Timed out while resolving Rocket League offsets");
            exit(EXIT_FAILURE);
        }

        if (TryGetClassAddress("Actor") == 0)
        {
            return false;
        }

        if (!ValidateRlsdkLayout())
        {
            m_classAddresses.erase("Actor");
            return false;
        }

        fmt::println("GObjects: 0x{:X}", g_objectsOffset);
        fmt::println("GNames: 0x{:X}", g_namesOffset);
        fmt::println("BuildDate: 0x{:X}", g_buildDateOffset);
        fmt::println("GPsyonixBuildID: 0x{:X}", g_psyonixBuildIdOffset);
        m_layoutValidated = true;
    }

    return true;
}

uint64_t OffsetResolver::FindClassAddress(const char *className) const
{
    std::vector<uint64_t> addresses = ObjectArray::GetAllObjectAddresses();
    for (size_t i = 0; i < addresses.size(); i++)
    {
        RemoteObject object(addresses[i]);
        if (!object.IsValidAddress() || object.GetName() != className)
        {
            continue;
        }

        RemoteObject metaClass(object.GetClassAddress());
        if (!metaClass.IsValidAddress() || metaClass.GetName() != "Class")
        {
            continue;
        }

        return object.GetAddress();
    }

    return 0;
}

uint64_t OffsetResolver::TryGetClassAddress(const char *className)
{
    auto cachedAddress = m_classAddresses.find(className);
    if (cachedAddress != m_classAddresses.end())
    {
        return cachedAddress->second;
    }

    uint64_t classAddress = FindClassAddress(className);
    if (classAddress != 0)
    {
        m_classAddresses.emplace(className, classAddress);
    }

    return classAddress;
}

uint64_t OffsetResolver::GetClassAddress(const char *className)
{
    uint64_t classAddress = TryGetClassAddress(className);
    if (classAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve class {}", className);
        exit(EXIT_FAILURE);
    }

    return classAddress;
}

uint64_t OffsetResolver::FindPropertyAddress(uint64_t classAddress, const char *propertyName) const
{
    while (classAddress != 0)
    {
        uint64_t propertyAddress = ReadMemory<uint64_t>(classAddress + kUStructChildrenOffset);
        while (propertyAddress != 0)
        {
            if (NameTable::Resolve(ReadMemory<int>(propertyAddress + kObjectNameOffset)) == propertyName)
            {
                return propertyAddress;
            }

            propertyAddress = ReadMemory<uint64_t>(propertyAddress + kUFieldNextOffset);
        }

        classAddress = ReadMemory<uint64_t>(classAddress + kUStructSuperFieldOffset);
    }

    return 0;
}

uint64_t OffsetResolver::GetPropertyOffset(uint64_t classAddress, const char *propertyName) const
{
    uint64_t propertyAddress = FindPropertyAddress(classAddress, propertyName);
    if (propertyAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    int offset = ReadMemory<int>(propertyAddress + kUPropertyOffsetOffset);
    if (offset < 0)
    {
        fmt::println(stderr, "Failed to resolve offset for property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    return (uint64_t) offset;
}

uint64_t OffsetResolver::GetStructPropertyOffset(uint64_t classAddress, const char *propertyName, const char *memberName) const
{
    uint64_t propertyAddress = FindPropertyAddress(classAddress, propertyName);
    if (propertyAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    uint64_t memberAddress = FindPropertyAddress(ReadMemory<uint64_t>(propertyAddress + kUStructPropertyStructOffset), memberName);
    if (memberAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}.{}", propertyName, memberName);
        exit(EXIT_FAILURE);
    }

    int offset = ReadMemory<int>(memberAddress + kUPropertyOffsetOffset);
    if (offset < 0)
    {
        fmt::println(stderr, "Failed to resolve offset for property {}.{}", propertyName, memberName);
        exit(EXIT_FAILURE);
    }

    return (uint64_t) offset;
}

uint64_t OffsetResolver::GetArrayStructPropertyOffset(uint64_t classAddress, const char *propertyName, const char *memberName) const
{
    uint64_t propertyAddress = FindPropertyAddress(classAddress, propertyName);
    if (propertyAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    uint64_t innerPropertyAddress = ReadMemory<uint64_t>(propertyAddress + kUArrayPropertyInnerOffset);
    uint64_t memberAddress        = FindPropertyAddress(ReadMemory<uint64_t>(innerPropertyAddress + kUStructPropertyStructOffset), memberName);
    if (memberAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}.{}", propertyName, memberName);
        exit(EXIT_FAILURE);
    }

    int offset = ReadMemory<int>(memberAddress + kUPropertyOffsetOffset);
    if (offset < 0)
    {
        fmt::println(stderr, "Failed to resolve offset for property {}.{}", propertyName, memberName);
        exit(EXIT_FAILURE);
    }

    return (uint64_t) offset;
}

uint64_t OffsetResolver::GetArrayStructElementSize(uint64_t classAddress, const char *propertyName) const
{
    uint64_t propertyAddress = FindPropertyAddress(classAddress, propertyName);
    if (propertyAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    int elementSize = ReadMemory<int>(ReadMemory<uint64_t>(propertyAddress + kUArrayPropertyInnerOffset) + kUPropertyElementSizeOffset);
    if (elementSize <= 0)
    {
        fmt::println(stderr, "Failed to resolve element size for property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    return (uint64_t) elementSize;
}

uint32_t OffsetResolver::GetBoolMask(uint64_t classAddress, const char *propertyName) const
{
    uint64_t propertyAddress = FindPropertyAddress(classAddress, propertyName);
    if (propertyAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    uint32_t bitMask = ReadMemory<uint32_t>(propertyAddress + kUBoolPropertyBitMaskOffset);
    if (bitMask == 0)
    {
        fmt::println(stderr, "Failed to resolve bit mask for property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    return bitMask;
}

uint64_t OffsetResolver::GetPropertyOffsetCached(uint64_t classAddress, const char *propertyName)
{
    std::lock_guard<std::mutex> lock(m_offsetCacheMutex);
    if (m_cacheSizeCap == 0)
    {
        return GetPropertyOffset(classAddress, propertyName);
    }

    std::string cacheKey = GetCacheKey(classAddress, propertyName, nullptr);
    auto cachedOffset    = m_propertyOffsetCache.find(cacheKey);
    if (cachedOffset != m_propertyOffsetCache.end())
    {
        return cachedOffset->second;
    }

    InvalidateCachedOffsetsIfNeeded();
    uint64_t offset = GetPropertyOffset(classAddress, propertyName);
    m_propertyOffsetCache.emplace(cacheKey, offset);
    fmt::println("Resolved property offset 0x{:X}.{}: 0x{:X}", classAddress, propertyName, offset);
    return offset;
}

uint64_t OffsetResolver::GetStructPropertyOffsetCached(uint64_t classAddress, const char *propertyName, const char *memberName)
{
    std::lock_guard<std::mutex> lock(m_offsetCacheMutex);
    if (m_cacheSizeCap == 0)
    {
        return GetStructPropertyOffset(classAddress, propertyName, memberName);
    }

    std::string cacheKey = GetCacheKey(classAddress, propertyName, memberName);
    auto cachedOffset    = m_structPropertyOffsetCache.find(cacheKey);
    if (cachedOffset != m_structPropertyOffsetCache.end())
    {
        return cachedOffset->second;
    }

    InvalidateCachedOffsetsIfNeeded();
    uint64_t offset = GetStructPropertyOffset(classAddress, propertyName, memberName);
    m_structPropertyOffsetCache.emplace(cacheKey, offset);
    fmt::println("Resolved struct property offset 0x{:X}.{}.{}, 0x{:X}", classAddress, propertyName, memberName, offset);
    return offset;
}

uint64_t OffsetResolver::GetArrayStructPropertyOffsetCached(uint64_t classAddress, const char *propertyName, const char *memberName)
{
    std::lock_guard<std::mutex> lock(m_offsetCacheMutex);
    if (m_cacheSizeCap == 0)
    {
        return GetArrayStructPropertyOffset(classAddress, propertyName, memberName);
    }

    std::string cacheKey = GetCacheKey(classAddress, propertyName, memberName);
    auto cachedOffset    = m_arrayStructPropertyOffsetCache.find(cacheKey);
    if (cachedOffset != m_arrayStructPropertyOffsetCache.end())
    {
        return cachedOffset->second;
    }

    InvalidateCachedOffsetsIfNeeded();
    uint64_t offset = GetArrayStructPropertyOffset(classAddress, propertyName, memberName);
    m_arrayStructPropertyOffsetCache.emplace(cacheKey, offset);
    fmt::println("Resolved array struct property offset 0x{:X}.{}.{}, 0x{:X}", classAddress, propertyName, memberName, offset);
    return offset;
}

uint64_t OffsetResolver::GetArrayStructElementSizeCached(uint64_t classAddress, const char *propertyName)
{
    std::lock_guard<std::mutex> lock(m_offsetCacheMutex);
    if (m_cacheSizeCap == 0)
    {
        return GetArrayStructElementSize(classAddress, propertyName);
    }

    std::string cacheKey = GetCacheKey(classAddress, propertyName, nullptr);
    auto cachedOffset    = m_arrayStructElementSizeCache.find(cacheKey);
    if (cachedOffset != m_arrayStructElementSizeCache.end())
    {
        return cachedOffset->second;
    }

    InvalidateCachedOffsetsIfNeeded();
    uint64_t elementSize = GetArrayStructElementSize(classAddress, propertyName);
    m_arrayStructElementSizeCache.emplace(cacheKey, elementSize);
    fmt::println("Resolved array struct element size 0x{:X}.{}: 0x{:X}", classAddress, propertyName, elementSize);
    return elementSize;
}

uint32_t OffsetResolver::GetBoolMaskCached(uint64_t classAddress, const char *propertyName)
{
    std::lock_guard<std::mutex> lock(m_offsetCacheMutex);
    if (m_cacheSizeCap == 0)
    {
        return GetBoolMask(classAddress, propertyName);
    }

    std::string cacheKey = GetCacheKey(classAddress, propertyName, nullptr);
    auto cachedMask      = m_boolMaskCache.find(cacheKey);
    if (cachedMask != m_boolMaskCache.end())
    {
        return cachedMask->second;
    }

    InvalidateCachedOffsetsIfNeeded();
    uint32_t bitMask = GetBoolMask(classAddress, propertyName);
    m_boolMaskCache.emplace(cacheKey, bitMask);
    fmt::println("Resolved bool mask 0x{:X}.{}: 0x{:X}", classAddress, propertyName, bitMask);
    return bitMask;
}

void OffsetResolver::SetCacheSizeCap(size_t cacheSizeCap)
{
    std::lock_guard<std::mutex> lock(m_offsetCacheMutex);
    m_cacheSizeCap = cacheSizeCap;
    if (m_cacheSizeCap == 0 || GetCachedOffsetCount() > m_cacheSizeCap)
    {
        InvalidateCachedOffsetsUnsafe();
    }
}

void OffsetResolver::InvalidateCachedOffsets()
{
    std::lock_guard<std::mutex> lock(m_offsetCacheMutex);
    InvalidateCachedOffsetsUnsafe();
}

void OffsetResolver::InvalidateCachedOffsetsUnsafe()
{
    m_propertyOffsetCache.clear();
    m_propertyOffsetCache.rehash(0);
    m_structPropertyOffsetCache.clear();
    m_structPropertyOffsetCache.rehash(0);
    m_arrayStructPropertyOffsetCache.clear();
    m_arrayStructPropertyOffsetCache.rehash(0);
    m_arrayStructElementSizeCache.clear();
    m_arrayStructElementSizeCache.rehash(0);
    m_boolMaskCache.clear();
    m_boolMaskCache.rehash(0);
}

std::string OffsetResolver::GetCacheKey(uint64_t classAddress, const char *propertyName, const char *memberName) const
{
    std::string cacheKey = std::to_string(classAddress);
    cacheKey += ':';
    cacheKey += propertyName;
    if (memberName)
    {
        cacheKey += ':';
        cacheKey += memberName;
    }

    return cacheKey;
}

size_t OffsetResolver::GetCachedOffsetCount() const
{
    return m_propertyOffsetCache.size() + m_structPropertyOffsetCache.size() + m_arrayStructPropertyOffsetCache.size() +
           m_arrayStructElementSizeCache.size() + m_boolMaskCache.size();
}

void OffsetResolver::InvalidateCachedOffsetsIfNeeded()
{
    if (GetCachedOffsetCount() < m_cacheSizeCap)
    {
        return;
    }

    fmt::println("Offset cache cap of {} reached, invalidating cached offsets", m_cacheSizeCap);
    InvalidateCachedOffsetsUnsafe();
}

bool OffsetResolver::ValidateRlsdkLayout()
{
    uint64_t actorClassAddress = GetClassAddress("Actor");
    uint64_t classAddress      = ReadMemory<uint64_t>(actorClassAddress + kObjectClassOffset);
    if (classAddress == 0 || NameTable::Resolve(ReadMemory<int>(classAddress + kObjectNameOffset)) != "Class")
    {
        fmt::println(stderr, "RLSDK validation failed: UObject::Class is invalid");
        return false;
    }

    uint64_t flags = ReadMemory<uint64_t>(actorClassAddress + kObjectFlagsOffset);
    if ((flags & RF_BadObjectFlags) != 0)
    {
        fmt::println(stderr, "RLSDK validation failed: GameDefines object flags are invalid");
        return false;
    }

    uint64_t propertyAddress = FindPropertyAddress(actorClassAddress, "Location");
    if (propertyAddress == 0)
    {
        fmt::println(stderr, "RLSDK validation failed: Actor.Location is missing");
        return false;
    }

    int propertyOffset = ReadMemory<int>(propertyAddress + kUPropertyOffsetOffset);
    if (propertyOffset < 0 || propertyOffset > 0x100000 || ReadMemory<int>(propertyAddress + kUPropertyElementSizeOffset) != (int) sizeof(FVector))
    {
        fmt::println(stderr, "RLSDK validation failed: UProperty layout is invalid");
        return false;
    }

    return true;
}

size_t OffsetResolver::FindPattern(const std::vector<uint8_t> &module, const uint8_t *pattern, const char *mask) const
{
    size_t patternSize = strlen(mask);
    for (size_t moduleOffset = 0; moduleOffset + patternSize <= module.size(); moduleOffset++)
    {
        size_t patternOffset = 0;
        for (; patternOffset < patternSize; patternOffset++)
        {
            if (mask[patternOffset] != '?' && module[moduleOffset + patternOffset] != pattern[patternOffset])
            {
                break;
            }
        }

        if (patternOffset == patternSize)
        {
            return moduleOffset;
        }
    }

    return module.size();
}

bool OffsetResolver::IsGObjectsValid() const
{
    if (g_objectsOffset == 0)
    {
        return false;
    }

    uint64_t moduleBase = NerineGlobals::GetInstance().GetModuleBase();
    uint64_t data       = ReadMemory<uint64_t>(moduleBase + g_objectsOffset);
    int count           = ReadMemory<int>(moduleBase + g_objectsOffset + 0x8);
    return data != 0 && count > 0 && count < 2000000;
}

bool OffsetResolver::IsGNamesValid() const
{
    if (g_namesOffset == 0)
    {
        return false;
    }

    uint64_t moduleBase = NerineGlobals::GetInstance().GetModuleBase();
    uint64_t data       = ReadMemory<uint64_t>(moduleBase + g_namesOffset);
    if (data == 0)
    {
        return false;
    }

    uint64_t entry = ReadMemory<uint64_t>(data);
    return entry != 0 && ReadMemory<char16_t>(entry + 0x18) == u'N' && ReadMemory<char16_t>(entry + 0x1A) == u'o' &&
           ReadMemory<char16_t>(entry + 0x1C) == u'n' && ReadMemory<char16_t>(entry + 0x1E) == u'e';
}
