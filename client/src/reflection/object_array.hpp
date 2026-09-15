#ifndef OBJECT_ARRAY_HPP
#define OBJECT_ARRAY_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "remote_object.hpp"

class ObjectArray
{
public:
    static int GetCount();
    static uint64_t GetData();
    static RemoteObject GetObjectAt(uint64_t data, int index);
    static std::vector<uint64_t> GetAllObjectAddresses();
    static uint64_t FindClassByName(const std::string &className);
    static RemoteObject FindFirstInstanceOf(uint64_t classAddress);
};

#endif
