#ifndef TRAJECTORY_HPP
#define TRAJECTORY_HPP

#include <string>
#include <vector>

#include "SdkHeaders.hpp"
#include "ball.hpp"
#include "SDK_HEADERS/Core_structs.hpp"

class Trajectory
{
public:
    static std::vector<FVector> Simulate(const Ball &ball, std::vector<FVector> &mapTouches, int maxTouches);
    static const std::string &GetCollisionMeshesFolder();
    static void SetCollisionMeshesFolder(const std::string &folder);
};

#endif
