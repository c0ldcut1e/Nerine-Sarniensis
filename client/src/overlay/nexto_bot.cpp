#include "nexto_bot.hpp"

#include <cmath>
#include <filesystem>

#include <linux/input.h>

#include "reflection/ball.hpp"
#include "reflection/boost_pad_list.hpp"
#include "reflection/car.hpp"
#include "reflection/car_list.hpp"
#include "reflection/engine_share.hpp"
#include "reflection/game_ui.hpp"
#include "reflection/input_bindings.hpp"
#include "reflection/local_player.hpp"
#include "reflection/match.hpp"

#include <fmt/base.h>
#include <imgui.h>

constexpr float kBoostLocations[34][3] = {
        {0.0f, -4240.0f, 70.0f},     {-1792.0f, -4184.0f, 70.0f}, {1792.0f, -4184.0f, 70.0f},  {-3072.0f, -4096.0f, 73.0f},
        {3072.0f, -4096.0f, 73.0f},  {-940.0f, -3308.0f, 70.0f},  {940.0f, -3308.0f, 70.0f},   {0.0f, -2816.0f, 70.0f},
        {-3584.0f, -2484.0f, 70.0f}, {3584.0f, -2484.0f, 70.0f},  {-1788.0f, -2300.0f, 70.0f}, {1788.0f, -2300.0f, 70.0f},
        {-2048.0f, -1036.0f, 70.0f}, {0.0f, -1024.0f, 70.0f},     {2048.0f, -1036.0f, 70.0f},  {-3584.0f, 0.0f, 73.0f},
        {-1024.0f, 0.0f, 70.0f},     {1024.0f, 0.0f, 70.0f},      {3584.0f, 0.0f, 73.0f},      {-2048.0f, 1036.0f, 70.0f},
        {0.0f, 1024.0f, 70.0f},      {2048.0f, 1036.0f, 70.0f},   {-1788.0f, 2300.0f, 70.0f},  {1788.0f, 2300.0f, 70.0f},
        {-3584.0f, 2484.0f, 70.0f},  {3584.0f, 2484.0f, 70.0f},   {0.0f, 2816.0f, 70.0f},      {-940.0f, 3310.0f, 70.0f},
        {940.0f, 3308.0f, 70.0f},    {-3072.0f, 4096.0f, 73.0f},  {3072.0f, 4096.0f, 73.0f},   {-1792.0f, 4184.0f, 70.0f},
        {1792.0f, 4184.0f, 70.0f},   {0.0f, 4240.0f, 70.0f}};

struct NextoKickoffStep
{
    int Ticks;
    float Action[8];
};

constexpr NextoKickoffStep kKickoffSteps[] = {
        {44, {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}}, {16, {1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f}},
        {8, {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f}},  {4, {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}},
        {4, {1.0f, 0.8f, -0.7f, 0.8f, 0.0f, 1.0f, 1.0f, 0.0f}}, {52, {1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}},
        {40, {1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f}}};

constexpr float kKickoffTakerMargin  = 10.0f;
constexpr int kKickoffIndexUndecided = -1;
constexpr int kKickoffIndexNotTaker  = -2;
constexpr int kTickSkip              = 8;
constexpr int kMaxTicksElapsed       = 30;
constexpr int kAxisSignPositive      = 1;
constexpr int kAxisSignNegative      = 2;

NextoBot::NextoBot()
{
    m_virtualGamepad       = NULL;
    m_status               = "Bot is disabled";
    m_ticks                = kTickSkip;
    m_kickoffIndex         = kKickoffIndexUndecided;
    m_previousPhysicsFrame = 0;
    m_matchActive          = false;
    m_botInputActive       = false;
    m_enabled              = false;
    m_action               = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

    for (int throttle = -1; throttle <= 1; throttle++)
    {
        for (int steer = -1; steer <= 1; steer++)
        {
            for (int boost = 0; boost <= 1; boost++)
            {
                for (int handbrake = 0; handbrake <= 1; handbrake++)
                {
                    if (boost && throttle != 1)
                    {
                        continue;
                    }

                    m_actions.push_back({(float) (throttle != 0 ? throttle : boost), (float) steer, 0.0f, (float) steer, 0.0f, 0.0f, (float) boost,
                                         (float) handbrake});
                }
            }
        }
    }

    for (int pitch = -1; pitch <= 1; pitch++)
    {
        for (int yaw = -1; yaw <= 1; yaw++)
        {
            for (int roll = -1; roll <= 1; roll++)
            {
                for (int jump = 0; jump <= 1; jump++)
                {
                    for (int boost = 0; boost <= 1; boost++)
                    {
                        if ((jump && yaw != 0) || (pitch == 0 && roll == 0 && jump == 0))
                        {
                            continue;
                        }

                        m_actions.push_back({(float) boost, (float) yaw, (float) pitch, (float) yaw, (float) roll, (float) jump, (float) boost,
                                             (float) (jump && (pitch != 0 || yaw != 0 || roll != 0))});
                    }
                }
            }
        }
    }
}

void NextoBot::Initialize(VirtualGamepad &virtualGamepad)
{
    m_virtualGamepad = &virtualGamepad;
}

void NextoBot::DrawMenu()
{
    if (!ImGui::CollapsingHeader("Bot"))
    {
        return;
    }

    ImGui::TextUnformatted(m_status.c_str());
    bool enabled = m_enabled;
    if (ImGui::Checkbox("Enable Bot", &enabled))
    {
        SetEnabled(enabled);
    }

    ImGui::TextUnformatted("Private and offline matches only.");
}

void NextoBot::Tick()
{
    if (!m_enabled)
    {
        return;
    }

    m_matchActive     = Match::IsActive();
    bool gameMenuOpen = GameUi::IsMenuOpen();
    if (!m_matchActive || !Match::IsRoundActive() || gameMenuOpen)
    {
        if (m_botInputActive)
        {
            m_virtualGamepad->SetBotInputEnabled(false);
            m_botInputActive = false;
        }

        m_status = gameMenuOpen ? "Bot input paused while the game menu is open" : "Bot input paused outside an active round";
        return;
    }

    if (!m_botInputActive)
    {
        if (!m_virtualGamepad->SetBotInputEnabled(true))
        {
            m_status = "Virtual gamepad is not ready";
            SetEnabled(false);
            return;
        }

        m_botInputActive = true;
        m_ticks          = kTickSkip;
    }

    int physicsFrame = EngineShare::GetPhysicsFrame();
    int ticksElapsed = physicsFrame - m_previousPhysicsFrame;
    if (ticksElapsed < 0 || ticksElapsed > kMaxTicksElapsed)
    {
        ticksElapsed = kMaxTicksElapsed;
    }

    m_previousPhysicsFrame = physicsFrame;
    m_ticks += ticksElapsed;

    LocalPlayer localPlayer = LocalPlayer::Find();
    if (!localPlayer)
    {
        m_status = "Waiting for the local player";
        return;
    }

    Car localCar = localPlayer.GetCar();
    Ball ball    = Ball::Find();
    if (!localCar || !ball)
    {
        m_status = "Waiting for the local car and ball";
        return;
    }

    if (!localCar.HasStaticData())
    {
        m_status = "Waiting for the local car data";
        return;
    }

    int localTeam = localCar.GetTeam();
    if (localTeam < 0 || localTeam > 1)
    {
        m_status = "Local car has an invalid team index";
        SetEnabled(false);
        return;
    }

    std::vector<Car> cars = CarList::GetAll();
    FVector ballLocation  = ball.GetLocation();
    if (m_ticks >= kTickSkip)
    {
        std::vector<NextoPlayerState> players;
        players.reserve(cars.size());
        players.push_back(GetPlayerState(localCar));
        for (size_t i = 0; i < cars.size(); i++)
        {
            if (cars[i].GetAddress() != localCar.GetAddress() && cars[i].GetTeam() == localTeam)
            {
                players.push_back(GetPlayerState(cars[i]));
            }
        }

        for (size_t i = 0; i < cars.size(); i++)
        {
            if (cars[i].GetAddress() != localCar.GetAddress() && cars[i].GetTeam() != localTeam)
            {
                players.push_back(GetPlayerState(cars[i]));
            }
        }

        try
        {
            m_action = GetAction(ballLocation, ball.GetLinearVelocity(), ball.GetAngularVelocity(), players, GetBoostAvailability(), localTeam);
            m_ticks -= kTickSkip;
            if (m_ticks < 0 || m_ticks >= kTickSkip)
            {
                m_ticks = 0;
            }

            m_status = "Running";
        }
        catch (const c10::Error &error)
        {
            m_status = error.what_without_backtrace();
            fmt::println(stderr, "Bot inference failed: {}", m_status);
            SetEnabled(false);
            return;
        }
    }

    UpdateKickoff(ticksElapsed, localCar, cars, ballLocation, localTeam);

    if (!UpdateController())
    {
        SetEnabled(false);
    }
}

void NextoBot::UpdateKickoff(int ticksElapsed, const Car &localCar, const std::vector<Car> &cars, const FVector &ballLocation, int localTeam)
{
    if (Match::HasBallBeenHit())
    {
        m_kickoffIndex = kKickoffIndexUndecided;
        return;
    }

    if (m_kickoffIndex >= 0)
    {
        m_kickoffIndex += ticksElapsed;
    }
    else if (m_kickoffIndex == kKickoffIndexUndecided)
    {
        FVector localLocation = localCar.GetLocation();
        float localDeltaX     = localLocation.X - ballLocation.X;
        float localDeltaY     = localLocation.Y - ballLocation.Y;
        float localDistance   = sqrtf(localDeltaX * localDeltaX + localDeltaY * localDeltaY);
        float nearestDistance = localDistance;
        std::vector<float> distances;
        distances.reserve(cars.size());
        for (size_t i = 0; i < cars.size(); i++)
        {
            FVector location = cars[i].GetLocation();
            float deltaX     = location.X - ballLocation.X;
            float deltaY     = location.Y - ballLocation.Y;
            distances.push_back(sqrtf(deltaX * deltaX + deltaY * deltaY));
            if (distances[i] < nearestDistance)
            {
                nearestDistance = distances[i];
            }
        }

        bool isKickoffTaker = localDistance - nearestDistance <= kKickoffTakerMargin;
        for (size_t i = 0; isKickoffTaker && i < cars.size(); i++)
        {
            if (cars[i].GetAddress() == localCar.GetAddress() || cars[i].GetTeam() != localTeam ||
                fabsf(distances[i] - localDistance) > kKickoffTakerMargin)
            {
                continue;
            }

            FVector location = cars[i].GetLocation();
            bool isLeft      = localTeam == 0 ? location.X < localLocation.X : location.X > localLocation.X;
            if (!isLeft)
            {
                isKickoffTaker = false;
            }
        }

        m_kickoffIndex = isKickoffTaker ? 0 : kKickoffIndexNotTaker;
    }

    if (m_kickoffIndex < 0 || ballLocation.Y != 0.0f)
    {
        return;
    }

    int step = 0;
    for (size_t i = 0; i < std::size(kKickoffSteps); i++)
    {
        if (m_kickoffIndex < step + kKickoffSteps[i].Ticks)
        {
            for (int field = 0; field < 8; field++)
            {
                m_action[field] = kKickoffSteps[i].Action[field];
            }

            m_status = "Kickoff";
            return;
        }

        step += kKickoffSteps[i].Ticks;
    }
}

std::array<float, 34> NextoBot::GetBoostAvailability() const
{
    constexpr float kMatchDistance = 200.0f;
    std::array<float, 34> availability;
    availability.fill(1.0f);
    for (const BoostPad &pad : BoostPadList::GetAll())
    {
        if ((pad.GetPickupState() & 1) == 0)
        {
            continue;
        }

        FVector location      = pad.GetLocation();
        int nearest           = -1;
        float nearestDistance = kMatchDistance;
        for (int i = 0; i < 34; i++)
        {
            float deltaX   = kBoostLocations[i][0] - location.X;
            float deltaY   = kBoostLocations[i][1] - location.Y;
            float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearest         = i;
            }
        }

        if (nearest >= 0)
        {
            availability[nearest] = 0.0f;
        }
    }

    return availability;
}

std::array<float, 8> NextoBot::GetAction(const FVector &ballLocation, const FVector &ballLinearVelocity, const FVector &ballAngularVelocity,
                                         const std::vector<NextoPlayerState> &players, const std::array<float, 34> &boostAvailability, int localTeam)
{
    int entityCount        = (int) players.size() + 35;
    torch::Tensor query    = torch::zeros({1, 1, 32}, torch::TensorOptions().dtype(torch::kFloat32));
    torch::Tensor keyValue = torch::zeros({1, entityCount, 24}, torch::TensorOptions().dtype(torch::kFloat32));
    torch::Tensor mask     = torch::zeros({1, entityCount}, torch::TensorOptions().dtype(torch::kFloat32));
    auto queryData         = query.accessor<float, 3>();
    auto keyValueData      = keyValue.accessor<float, 3>();
    float normalizer[24]   = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2300.0f, 2300.0f, 2300.0f, 2300.0f, 2300.0f, 2300.0f, 1.0f,
                              1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 5.5f,    5.5f,    5.5f,    1.0f,    1.0f,    1.0f,    1.0f};
    float teamSign         = localTeam == 1 ? -1.0f : 1.0f;
    int ballEntity         = (int) players.size();

    keyValueData[0][ballEntity][3]  = 1.0f;
    keyValueData[0][ballEntity][5]  = ballLocation.X * teamSign;
    keyValueData[0][ballEntity][6]  = ballLocation.Y * teamSign;
    keyValueData[0][ballEntity][7]  = ballLocation.Z;
    keyValueData[0][ballEntity][8]  = ballLinearVelocity.X * teamSign;
    keyValueData[0][ballEntity][9]  = ballLinearVelocity.Y * teamSign;
    keyValueData[0][ballEntity][10] = ballLinearVelocity.Z;
    keyValueData[0][ballEntity][17] = ballAngularVelocity.X * teamSign;
    keyValueData[0][ballEntity][18] = ballAngularVelocity.Y * teamSign;
    keyValueData[0][ballEntity][19] = ballAngularVelocity.Z;
    for (int i = 0; i < 34; i++)
    {
        int entity                  = ballEntity + i + 1;
        keyValueData[0][entity][4]  = 1.0f;
        keyValueData[0][entity][5]  = kBoostLocations[i][0] * teamSign;
        keyValueData[0][entity][6]  = kBoostLocations[i][1] * teamSign;
        keyValueData[0][entity][7]  = kBoostLocations[i][2];
        keyValueData[0][entity][20] = kBoostLocations[i][2] > 72.0f ? 1.0f : 0.12f;
        keyValueData[0][entity][21] = boostAvailability[i];
    }

    for (size_t i = 0; i < players.size(); i++)
    {
        keyValueData[0][i][0]  = i == 0;
        keyValueData[0][i][1]  = players[i].Team == localTeam;
        keyValueData[0][i][2]  = players[i].Team != localTeam;
        keyValueData[0][i][5]  = players[i].Location.X * teamSign;
        keyValueData[0][i][6]  = players[i].Location.Y * teamSign;
        keyValueData[0][i][7]  = players[i].Location.Z;
        keyValueData[0][i][8]  = players[i].LinearVelocity.X * teamSign;
        keyValueData[0][i][9]  = players[i].LinearVelocity.Y * teamSign;
        keyValueData[0][i][10] = players[i].LinearVelocity.Z;
        keyValueData[0][i][11] = players[i].Forward.X * teamSign;
        keyValueData[0][i][12] = players[i].Forward.Y * teamSign;
        keyValueData[0][i][13] = players[i].Forward.Z;
        keyValueData[0][i][14] = players[i].Up.X * teamSign;
        keyValueData[0][i][15] = players[i].Up.Y * teamSign;
        keyValueData[0][i][16] = players[i].Up.Z;
        keyValueData[0][i][17] = players[i].AngularVelocity.X * teamSign;
        keyValueData[0][i][18] = players[i].AngularVelocity.Y * teamSign;
        keyValueData[0][i][19] = players[i].AngularVelocity.Z;
        keyValueData[0][i][20] = players[i].BoostAmount / 100.0f;
        keyValueData[0][i][21] = players[i].Demoed;
        keyValueData[0][i][22] = players[i].OnGround;
        keyValueData[0][i][23] = players[i].HasFlip;
    }

    for (int entity = 0; entity < entityCount; entity++)
    {
        for (int field = 0; field < 24; field++)
        {
            keyValueData[0][entity][field] /= normalizer[field];
        }
    }

    for (int field = 0; field < 24; field++)
    {
        queryData[0][0][field] = keyValueData[0][0][field];
    }

    for (int field = 0; field < 8; field++)
    {
        queryData[0][0][field + 24] = m_action[field];
    }

    for (int entity = 0; entity < entityCount; entity++)
    {
        keyValueData[0][entity][5] -= queryData[0][0][5];
        keyValueData[0][entity][6] -= queryData[0][0][6];
        keyValueData[0][entity][7] -= queryData[0][0][7];
    }

    float angle  = atan2f(queryData[0][0][11], queryData[0][0][12]);
    float cosine = cosf(angle);
    float sine   = sinf(angle);
    for (int entity = 0; entity < entityCount; entity++)
    {
        for (int field = 5; field < 20; field += 3)
        {
            float x                            = keyValueData[0][entity][field];
            float y                            = keyValueData[0][entity][field + 1];
            keyValueData[0][entity][field]     = cosine * x - sine * y;
            keyValueData[0][entity][field + 1] = sine * x + cosine * y;
        }
    }

    torch::NoGradGuard noGrad;
    torch::IValue output = m_model->forward({c10::ivalue::Tuple::create({query, keyValue, mask})});
    torch::Tensor logits = output.toTuple()->elements()[0].toTensor();
    int actionIndex      = logits.argmax().item<int>();
    return m_actions[actionIndex];
}

bool NextoBot::LoadControllerBindings()
{
    std::vector<GamepadBinding> gamepadBindings = InputBindings::GetGamepadBindings();
    bool actionFound[8]                         = {false, false, false, false, false, false, false, false};
    const char *actionNames[8]                  = {"Throttle", "Steer", "Pitch", "Yaw", "Air Roll", "Jump", "Boost", "Handbrake"};
    std::string missingActions;
    m_controllerBindings.clear();
    for (size_t i = 0; i < gamepadBindings.size(); i++)
    {
        NextoControllerBinding binding;
        binding.Action    = -1;
        binding.Type      = -1;
        binding.Code      = -1;
        binding.AxisSign  = gamepadBindings[i].AxisSign;
        binding.Direction = 0;
        if (gamepadBindings[i].Action.find("Throttle") != std::string::npos)
        {
            binding.Action    = 0;
            binding.Direction = gamepadBindings[i].Action == "ThrottleForward" ? 1 : gamepadBindings[i].Action == "ThrottleReverse" ? -1 : 0;
        }
        else if (gamepadBindings[i].Action.find("Steer") != std::string::npos)
        {
            binding.Action = 1;
        }
        else if (gamepadBindings[i].Action.find("Pitch") != std::string::npos)
        {
            binding.Action = 2;
        }
        else if (gamepadBindings[i].Action.find("Yaw") != std::string::npos)
        {
            binding.Action = 3;
        }
        else if (gamepadBindings[i].Action.find("AirRollLeft") != std::string::npos || gamepadBindings[i].Action == "RollLeft")
        {
            binding.Action    = 4;
            binding.Direction = -1;
        }
        else if (gamepadBindings[i].Action.find("AirRollRight") != std::string::npos || gamepadBindings[i].Action == "RollRight")
        {
            binding.Action    = 4;
            binding.Direction = 1;
        }
        else if (gamepadBindings[i].Action.find("AirRoll") != std::string::npos || gamepadBindings[i].Action == "Roll")
        {
            binding.Action = 4;
        }
        else if (gamepadBindings[i].Action.find("Jump") != std::string::npos)
        {
            binding.Action = 5;
        }
        else if (gamepadBindings[i].Action.find("Boost") != std::string::npos)
        {
            binding.Action = 6;
        }
        else if (gamepadBindings[i].Action.find("Handbrake") != std::string::npos)
        {
            binding.Action = 7;
        }

        if (gamepadBindings[i].Key == "XboxTypeS_A")
        {
            binding.Type = EV_KEY;
            binding.Code = BTN_SOUTH;
        }
        else if (gamepadBindings[i].Key == "XboxTypeS_B")
        {
            binding.Type = EV_KEY;
            binding.Code = BTN_EAST;
        }
        else if (gamepadBindings[i].Key == "XboxTypeS_X")
        {
            binding.Type = EV_KEY;
            binding.Code = BTN_X;
        }
        else if (gamepadBindings[i].Key == "XboxTypeS_Y")
        {
            binding.Type = EV_KEY;
            binding.Code = BTN_Y;
        }
        else if (gamepadBindings[i].Key == "XboxTypeS_LeftShoulder")
        {
            binding.Type = EV_KEY;
            binding.Code = BTN_TL;
        }
        else if (gamepadBindings[i].Key == "XboxTypeS_RightShoulder")
        {
            binding.Type = EV_KEY;
            binding.Code = BTN_TR;
        }
        else if (gamepadBindings[i].Key == "XboxTypeS_LeftX")
        {
            binding.Type = EV_ABS;
            binding.Code = ABS_X;
        }
        else if (gamepadBindings[i].Key == "XboxTypeS_LeftY")
        {
            binding.Type = EV_ABS;
            binding.Code = ABS_Y;
        }
        else if (gamepadBindings[i].Key == "XboxTypeS_RightX")
        {
            binding.Type = EV_ABS;
            binding.Code = ABS_RX;
        }
        else if (gamepadBindings[i].Key == "XboxTypeS_RightY")
        {
            binding.Type = EV_ABS;
            binding.Code = ABS_RY;
        }
        else if (gamepadBindings[i].Key == "XboxTypeS_LeftTrigger" || gamepadBindings[i].Key == "XboxTypeS_LeftTriggerAxis")
        {
            binding.Type = EV_ABS;
            binding.Code = ABS_Z;
        }
        else if (gamepadBindings[i].Key == "XboxTypeS_RightTrigger" || gamepadBindings[i].Key == "XboxTypeS_RightTriggerAxis")
        {
            binding.Type = EV_ABS;
            binding.Code = ABS_RZ;
        }

        if (binding.Action < 0 || binding.Type < 0)
        {
            continue;
        }

        if (!binding.Direction && (binding.Type == EV_KEY || binding.Code == ABS_Z || binding.Code == ABS_RZ))
        {
            binding.Direction = binding.AxisSign == kAxisSignPositive ? 1 : binding.AxisSign == kAxisSignNegative ? -1 : 0;
        }

        actionFound[binding.Action] = true;
        m_controllerBindings.push_back(binding);
    }

    for (int i = 0; i < 8; i++)
    {
        if (actionFound[i])
        {
            continue;
        }

        if (!missingActions.empty())
        {
            missingActions += ", ";
        }

        missingActions += actionNames[i];
    }

    if (!missingActions.empty())
    {
        m_status = "Missing gamepad bindings: " + missingActions;
        fmt::println(stderr, "Nexto {}", m_status);
        return false;
    }

    return true;
}

bool NextoBot::UpdateController()
{
    for (size_t i = 0; i < m_controllerBindings.size(); i++)
    {
        int eventValue;
        if (m_controllerBindings[i].Type == EV_KEY)
        {
            bool emitted = false;
            for (size_t binding = 0; binding < i; binding++)
            {
                if (m_controllerBindings[binding].Type == EV_KEY && m_controllerBindings[binding].Code == m_controllerBindings[i].Code)
                {
                    emitted = true;
                    break;
                }
            }

            if (emitted)
            {
                continue;
            }

            eventValue = 0;
            for (size_t binding = i; binding < m_controllerBindings.size(); binding++)
            {
                if (m_controllerBindings[binding].Type != EV_KEY || m_controllerBindings[binding].Code != m_controllerBindings[i].Code)
                {
                    continue;
                }

                float value = m_action[m_controllerBindings[binding].Action];
                if (m_controllerBindings[binding].Direction < 0)
                {
                    value = value < 0.0f ? -value : 0.0f;
                }
                else if (m_controllerBindings[binding].Direction > 0)
                {
                    value = value > 0.0f ? value : 0.0f;
                }

                if (m_controllerBindings[binding].Action == 4 && !m_controllerBindings[binding].Direction ? value != 0.0f : value > 0.0f)
                {
                    eventValue = 1;
                    break;
                }
            }
        }
        else
        {
            float value = m_action[m_controllerBindings[i].Action];
            if (m_controllerBindings[i].Direction < 0)
            {
                value = value < 0.0f ? -value : 0.0f;
            }
            else if (m_controllerBindings[i].Direction > 0)
            {
                value = value > 0.0f ? value : 0.0f;
            }

            if (m_controllerBindings[i].Code == ABS_Z || m_controllerBindings[i].Code == ABS_RZ)
            {
                eventValue = (int) (value * 255.0f);
            }
            else
            {
                eventValue = (int) (value * 32767.0f);
            }
        }

        if (!m_virtualGamepad->UpdateBotInput(m_controllerBindings[i].Type, m_controllerBindings[i].Code, eventValue))
        {
            m_status = "Virtual gamepad stopped accepting bot input";
            return false;
        }
    }

    return true;
}

NextoPlayerState NextoBot::GetPlayerState(const Car &car) const
{
    FQuat rotation = car.GetRotation();
    NextoPlayerState player;
    player.Location        = car.GetLocation();
    player.LinearVelocity  = car.GetLinearVelocity();
    player.AngularVelocity = car.GetAngularVelocity();
    player.Forward.X       = 1.0f - 2.0f * (rotation.Y * rotation.Y + rotation.Z * rotation.Z);
    player.Forward.Y       = 2.0f * (rotation.X * rotation.Y + rotation.Z * rotation.W);
    player.Forward.Z       = 2.0f * (rotation.X * rotation.Z - rotation.Y * rotation.W);
    player.Up.X            = 2.0f * (rotation.X * rotation.Z + rotation.Y * rotation.W);
    player.Up.Y            = 2.0f * (rotation.Y * rotation.Z - rotation.X * rotation.W);
    player.Up.Z            = 1.0f - 2.0f * (rotation.X * rotation.X + rotation.Y * rotation.Y);
    player.BoostAmount     = car.GetBoostAmount();
    player.Team            = car.GetTeam();
    player.OnGround        = car.IsOnGround();
    player.HasFlip         = car.HasFlip();
    player.Demoed          = car.IsDemolished();
    return player;
}

bool NextoBot::LoadModel()
{
    try
    {
        std::filesystem::path executableDirectory = std::filesystem::read_symlink("/proc/self/exe").parent_path();
        m_model                                   = torch::jit::load(executableDirectory / "nexto-model.pt");
        m_model->eval();
        m_status = "Nexto model loaded";
        return true;
    }
    catch (const c10::Error &error)
    {
        m_status = error.what_without_backtrace();
        fmt::println(stderr, "Failed to load bot model: {}", m_status);
        return false;
    }
}

void NextoBot::SetEnabled(bool enabled)
{
    if (!enabled)
    {
        m_enabled        = false;
        m_botInputActive = false;
        m_action         = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        m_virtualGamepad->SetBotInputEnabled(false);
        m_model.reset();
        m_status = "Bot disabled; physical controller is active";
        return;
    }

    if (!m_virtualGamepad->SetBotInputEnabled(true))
    {
        m_status = "Virtual gamepad is not ready";
        return;
    }

    if (!m_model && !LoadModel())
    {
        m_virtualGamepad->SetBotInputEnabled(false);
        m_botInputActive = false;
        return;
    }

    if (!LoadControllerBindings())
    {
        m_model.reset();
        m_virtualGamepad->SetBotInputEnabled(false);
        m_botInputActive = false;
        return;
    }

    m_enabled              = true;
    m_botInputActive       = true;
    m_ticks                = kTickSkip;
    m_previousPhysicsFrame = EngineShare::GetPhysicsFrame();
    m_action               = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    m_status               = "Nexto enabled; waiting for an active match";
    fmt::println("Nexto enabled");
}
