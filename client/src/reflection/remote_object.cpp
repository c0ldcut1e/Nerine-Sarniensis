#include "remote_object.hpp"

#include "GameDefines.hpp"
#include "name_table.hpp"
#include "core/nerine_memory.hpp"
#include "ue3_offsets.hpp"

RemoteObject::RemoteObject()
{
    m_address = 0;
}

RemoteObject::RemoteObject(uint64_t address)
{
    m_address = address;
}

bool RemoteObject::IsValidAddress() const
{
    return m_address && m_address > 0x10000 && m_address < 0x0000800000000000;
}

uint64_t RemoteObject::GetClassAddress() const
{
    return ReadMemory<uint64_t>(m_address + kObjectClassOffset);
}

uint64_t RemoteObject::GetFlags() const
{
    return ReadMemory<uint64_t>(m_address + kObjectFlagsOffset);
}

bool RemoteObject::HasAnyFlags(uint64_t flags) const
{
    return (GetFlags() & flags) != 0;
}

bool RemoteObject::IsDefaultObject() const
{
    return HasAnyFlags(RF_ClassDefaultObject);
}

bool RemoteObject::IsPendingKillOrBad() const
{
    return HasAnyFlags(RF_BadObjectFlags);
}

int RemoteObject::GetNameId() const
{
    return ReadMemory<int>(m_address + kObjectNameOffset);
}

std::string RemoteObject::GetName() const
{
    return NameTable::Resolve(GetNameId());
}

bool RemoteObject::IsValidInstanceOf(uint64_t classAddress) const
{
    if (!IsValidAddress())
    {
        return false;
    }

    if (GetClassAddress() != classAddress)
    {
        return false;
    }

    if (IsDefaultObject() || IsPendingKillOrBad())
    {
        return false;
    }

    return true;
}
