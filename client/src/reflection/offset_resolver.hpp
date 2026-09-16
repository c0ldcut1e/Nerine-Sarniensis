#ifndef OFFSET_RESOLVER_HPP
#define OFFSET_RESOLVER_HPP

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class OffsetResolver
{
public:
    static OffsetResolver *GetInstance();

    OffsetResolver(const OffsetResolver &other)            = delete;
    OffsetResolver &operator=(const OffsetResolver &other) = delete;

    bool Resolve();
    uint64_t GetGObjectsOffset() const;
    uint64_t GetGNamesOffset() const;
    uint64_t GetBuildDateOffset() const;
    uint64_t GetGPsyonixBuildIdOffset() const;
    uint64_t GetClassAddress(const char *className);
    uint64_t TryGetClassAddress(const char *className);

    uint64_t GetPropertyOffset(uint64_t classAddress, const char *propertyName) const;
    uint64_t GetPropertyOffsetCached(uint64_t classAddress, const char *propertyName);

    uint64_t GetStructPropertyOffset(uint64_t classAddress, const char *propertyName, const char *memberName) const;
    uint64_t GetStructPropertyOffsetCached(uint64_t classAddress, const char *propertyName, const char *memberName);

    uint64_t GetArrayStructPropertyOffset(uint64_t classAddress, const char *propertyName, const char *memberName) const;
    uint64_t GetArrayStructPropertyOffsetCached(uint64_t classAddress, const char *propertyName, const char *memberName);

    uint64_t GetArrayStructElementSize(uint64_t classAddress, const char *propertyName) const;
    uint64_t GetArrayStructElementSizeCached(uint64_t classAddress, const char *propertyName);

    uint32_t GetBoolMask(uint64_t classAddress, const char *propertyName) const;
    uint32_t GetBoolMaskCached(uint64_t classAddress, const char *propertyName);

    void SetCacheSizeCap(size_t cacheSizeCap);
    void InvalidateCachedOffsets();

private:
    OffsetResolver();

    size_t FindPattern(const std::vector<uint8_t> &module, const uint8_t *pattern, const char *mask) const;
    uint64_t FindClassAddress(const char *className) const;
    uint64_t FindPropertyAddress(uint64_t classAddress, const char *propertyName) const;
    std::string GetCacheKey(uint64_t classAddress, const char *propertyName, const char *memberName) const;
    size_t GetCachedOffsetCount() const;
    void InvalidateCachedOffsetsIfNeeded();
    void InvalidateCachedOffsetsUnsafe();
    bool ValidateRlsdkLayout();
    bool IsGObjectsValid() const;
    bool IsGNamesValid() const;

    bool m_layoutValidated;
    std::chrono::steady_clock::time_point m_offsetResolveStartTime;
    size_t m_cacheSizeCap;
    std::mutex m_offsetCacheMutex;
    std::unordered_map<std::string, uint64_t> m_classAddresses;
    std::unordered_map<std::string, uint64_t> m_propertyOffsetCache;
    std::unordered_map<std::string, uint64_t> m_structPropertyOffsetCache;
    std::unordered_map<std::string, uint64_t> m_arrayStructPropertyOffsetCache;
    std::unordered_map<std::string, uint64_t> m_arrayStructElementSizeCache;
    std::unordered_map<std::string, uint32_t> m_boolMaskCache;
};

#endif
