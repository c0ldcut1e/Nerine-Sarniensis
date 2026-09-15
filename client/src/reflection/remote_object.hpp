#ifndef REMOTE_OBJECT_HPP
#define REMOTE_OBJECT_HPP

#include <cstdint>
#include <string>

class RemoteObject
{
public:
    RemoteObject();
    explicit RemoteObject(uint64_t address);

    uint64_t GetAddress() const
    {
        return m_address;
    }

    bool IsValidAddress() const;

    uint64_t GetClassAddress() const;
    uint64_t GetFlags() const;
    bool HasAnyFlags(uint64_t flags) const;
    bool IsDefaultObject() const;
    bool IsPendingKillOrBad() const;
    int GetNameId() const;
    std::string GetName() const;

    bool IsValidInstanceOf(uint64_t classAddress) const;

    explicit operator bool() const
    {
        return IsValidAddress();
    }

protected:
    uint64_t m_address;
};

#endif
