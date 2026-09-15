#ifndef OFFSET_RESOLVER_HPP
#define OFFSET_RESOLVER_HPP

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class OffsetResolver
{
public:
    static OffsetResolver &GetInstance();

    OffsetResolver(const OffsetResolver &other)            = delete;
    OffsetResolver &operator=(const OffsetResolver &other) = delete;

    bool Resolve();

private:
    OffsetResolver();

    size_t FindPattern(const std::vector<uint8_t> &module, const uint8_t *pattern, const char *mask) const;
    void ResolveRocketLeagueOffsets();
    bool BuildClassAddresses();
    uint64_t GetClassAddress(const char *className) const;
    uint64_t FindPropertyAddress(uint64_t classAddress, const char *propertyName) const;
    uint64_t GetPropertyOffset(uint64_t classAddress, const char *propertyName) const;
    uint64_t GetStructPropertyOffset(uint64_t classAddress, const char *propertyName, const char *memberName) const;
    uint64_t GetArrayStructPropertyOffset(uint64_t classAddress, const char *propertyName, const char *memberName) const;
    uint64_t GetArrayStructElementSize(uint64_t classAddress, const char *propertyName) const;
    uint32_t GetBoolMask(uint64_t classAddress, const char *propertyName) const;
    bool ValidateRlsdkLayout() const;
    bool IsGObjectsValid() const;
    bool IsGNamesValid() const;

    bool m_rocketLeagueOffsetsResolved;
    std::chrono::steady_clock::time_point m_offsetResolveStartTime;
    std::unordered_map<std::string, uint64_t> m_classAddresses;
};

#endif
