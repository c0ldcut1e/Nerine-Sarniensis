#include "name_table.hpp"

#include "nerine_globals.hpp"
#include "nerine_memory.hpp"
#include "offsets.hpp"

std::string NameTable::Resolve(int32_t nameId)
{
    struct NameBuffer
    {
        char16_t chars[64];
    };

    uint64_t base      = NerineGlobals::GetInstance().GetModuleBase();
    uint64_t namesData = ReadMemory<uint64_t>(base + kGNamesOffset);
    uint64_t entry     = ReadMemory<uint64_t>(namesData + nameId * 8);
    if (!entry)
    {
        return "";
    }

    NameBuffer buffer = ReadMemory<NameBuffer>(entry + kNameEntryNameOffset);
    std::string result;

    for (size_t i = 0; buffer.chars[i]; i++)
    {
        result += (char) buffer.chars[i];
    }

    return result;
}
