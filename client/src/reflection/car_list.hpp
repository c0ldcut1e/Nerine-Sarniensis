#ifndef CAR_LIST_HPP
#define CAR_LIST_HPP

#include <vector>

#include "SdkHeaders.hpp"
#include "car.hpp"
#include "SDK_HEADERS/Core_structs.hpp"

class CarList
{
public:
    static std::vector<FVector> GetPositions();
    static std::vector<Car> GetAll();
    static void Debug();
};

#endif
