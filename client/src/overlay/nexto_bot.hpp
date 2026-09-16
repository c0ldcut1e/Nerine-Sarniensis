#ifndef NEXTO_BOT_HPP
#define NEXTO_BOT_HPP

#include <array>
#include <optional>
#include <string>
#include <torch/script.h>
#include <vector>

#include "SdkHeaders.hpp"
#include "overlay/virtual_gamepad.hpp"

#include "reflection/car.hpp"

struct NextoPlayerState
{
    FVector Location;
    FVector LinearVelocity;
    FVector AngularVelocity;
    FVector Forward;
    FVector Up;
    float BoostAmount;
    int Team;
    bool OnGround;
    bool HasFlip;
    bool Demoed;
};

struct NextoControllerBinding
{
    int Action;
    int Type;
    int Code;
    int AxisSign;
    int Direction;
};

class NextoBot
{
public:
    NextoBot();

    void DrawMenu();
    void Initialize(VirtualGamepad &virtualGamepad);
    void Tick();

private:
    std::array<float, 8> GetAction(const FVector &ballLocation, const FVector &ballLinearVelocity, const FVector &ballAngularVelocity,
                                   const std::vector<NextoPlayerState> &players, const std::array<float, 34> &boostAvailability, int localTeam);
    std::array<float, 34> GetBoostAvailability() const;
    bool LoadControllerBindings();
    bool LoadModel();
    bool UpdateController();
    void UpdateKickoff(int ticksElapsed, const Car &localCar, const std::vector<Car> &cars, const FVector &ballLocation, int localTeam);
    NextoPlayerState GetPlayerState(const Car &car) const;
    void SetEnabled(bool enabled);

    VirtualGamepad *m_virtualGamepad;
    std::optional<torch::jit::script::Module> m_model;
    std::vector<std::array<float, 8>> m_actions;
    std::vector<NextoControllerBinding> m_controllerBindings;
    std::array<float, 8> m_action;
    std::string m_status;
    int m_ticks;
    int m_kickoffIndex;
    int m_previousPhysicsFrame;
    bool m_matchActive;
    bool m_botInputActive;
    bool m_enabled;
};

#endif
