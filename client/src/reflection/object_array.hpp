#ifndef OBJECT_ARRAY_HPP
#define OBJECT_ARRAY_HPP

#include <cstdint>
#include <vector>

class ObjectArray
{
public:
    static int GetCount();
    static uint64_t GetData();
    static std::vector<uint64_t> GetAllObjectAddresses();
};

#endif
