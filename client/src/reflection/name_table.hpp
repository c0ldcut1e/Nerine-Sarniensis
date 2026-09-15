#ifndef NAME_TABLE_HPP
#define NAME_TABLE_HPP

#include <cstdint>
#include <string>

class NameTable
{
public:
    static std::string Resolve(int32_t nameId);
};

#endif
