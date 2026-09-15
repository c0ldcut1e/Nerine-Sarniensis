#include "offset_resolver.hpp"

#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "name_table.hpp"
#include "nerine_globals.hpp"
#include "nerine_memory.hpp"
#include "object_array.hpp"
#include "offsets.hpp"

#include "GameDefines.hpp"
#include "SdkHeaders.hpp"

#include <fmt/core.h>

uint64_t g_objectsOffset;
uint64_t g_namesOffset;
uint64_t g_buildDateOffset;
uint64_t g_psyonixBuildIdOffset;
uint64_t g_actorLocationOffset;
uint64_t g_actorHiddenOffset;
uint32_t g_actorHiddenFlag;
uint64_t g_rigidBodyStateOffset;
uint64_t g_rigidBodyRotationOffset;
uint64_t g_rigidBodyLocationOffset;
uint64_t g_rigidBodyLinearVelocityOffset;
uint64_t g_rigidBodyAngularVelocityOffset;
uint64_t g_rigidBodyMaxLinearSpeedOffset;
uint64_t g_rigidBodyMaxAngularSpeedOffset;
uint64_t g_rigidBodyRollingFrictionOffset;
uint64_t g_playerCameraOffset;
uint64_t g_cameraCacheOffset;
uint64_t g_cameraCachePovOffset;
uint64_t g_playerCarOffset;
uint64_t g_carVehicleSimOffset;
uint64_t g_carMeshOffset;
uint64_t g_carStateFlagsOffset;
uint64_t g_carPriOffset;
uint64_t g_carLocalCollisionOffset;
uint64_t g_carLocalCollisionExtentOffset;
uint64_t g_carBoostComponentOffset;
uint64_t g_vehicleSimWheelsOffset;
uint64_t g_wheelRadiusOffset;
uint64_t g_wheelLocalRestPositionOffset;
uint64_t g_boostMaxAmountOffset;
uint64_t g_boostCurrentAmountOffset;
uint64_t g_boostReplicatedAmountOffset;
uint64_t g_priTeamOffset;
uint64_t g_priCarOffset;
uint64_t g_teamIndexOffset;
uint32_t g_carOnGroundFlag;
uint32_t g_carDoubleJumpedFlag;
uint64_t g_primitiveComponentLocalToWorldOffset;
uint64_t g_ballMagnusCoefficientOffset;
uint64_t g_ballMagnusMinSpeedOffset;
uint64_t g_ballAngularVelocityDampingOffset;
uint64_t g_ballRadiusOffset;
uint64_t g_ballWorldBounceScaleOffset;
uint64_t g_ballGravityScaleOffset;
uint64_t g_ballMaxLinearSpeedScaleOffset;
uint64_t g_ballAirResistanceOffset;
uint64_t g_ballMinResistanceVelocityOffset;
uint64_t g_ballPredictedPositionsOffset;
uint64_t g_gameEventFlagsOffset;
uint32_t g_gameEventRoundActiveFlag;
uint32_t g_gameEventBallHasBeenHitFlag;
uint64_t g_gameEventCarsOffset;
uint64_t g_gameEventPrisOffset;
uint64_t g_pickupRespawnDelayOffset;
uint64_t g_pickupPickedUpOffset;
uint64_t g_playerInputOffset;
uint64_t g_gamepadBindingsOffset;
uint64_t g_playerBindingSize;
uint64_t g_playerBindingActionNameOffset;
uint64_t g_playerBindingKeyNameOffset;
uint64_t g_playerBindingAxisSignOffset;

constexpr uint64_t kUFieldNextOffset            = 0x60;
constexpr uint64_t kUStructSuperFieldOffset     = 0x80;
constexpr uint64_t kUStructChildrenOffset       = 0x88;
constexpr uint64_t kUPropertyElementSizeOffset  = 0x74;
constexpr uint64_t kUPropertyOffsetOffset       = 0x98;
constexpr uint64_t kUStructPropertyStructOffset = 0xC8;
constexpr uint64_t kUArrayPropertyInnerOffset   = 0xC8;
constexpr uint64_t kUBoolPropertyBitMaskOffset  = 0xC8;

OffsetResolver::OffsetResolver()
{
    m_rocketLeagueOffsetsResolved = false;
    m_offsetResolveStartTime      = std::chrono::steady_clock::now();
}

OffsetResolver &OffsetResolver::GetInstance()
{
    static OffsetResolver instance;
    return instance;
}

bool OffsetResolver::Resolve()
{
    uint64_t moduleBase = NerineGlobals::GetInstance().GetModuleBase();
    if (g_objectsOffset == 0 || g_namesOffset == 0 || g_buildDateOffset == 0 || g_psyonixBuildIdOffset == 0)
    {
        uint32_t peHeaderOffset = ReadMemory<uint32_t>(moduleBase + 0x3C);
        uint32_t moduleSize     = ReadMemory<uint32_t>(moduleBase + peHeaderOffset + 0x50);
        if (moduleSize < 0x100000 || moduleSize > 0x10000000)
        {
            fmt::println(stderr, "Failed to read the RocketLeague.exe module size");
            exit(EXIT_FAILURE);
        }

        std::vector<uint8_t> module = ReadMemoryArray<uint8_t>(moduleBase, moduleSize);
        if (g_objectsOffset == 0)
        {
            uint8_t pattern[] = {0x48, 0x8B, 0xC8, 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x0C, 0xC8};
            size_t match      = FindPattern(module, pattern, "xxxxxx????xxxx");
            if (match != module.size())
            {
                int displacement;
                memcpy(&displacement, module.data() + match + 6, sizeof(displacement));

                int64_t offset = (int64_t) match + 10 + displacement;
                if (offset > 0 && (uint64_t) offset < module.size())
                {
                    g_objectsOffset = (uint64_t) offset;
                }
            }
        }

        if (g_namesOffset == 0)
        {
            uint8_t pattern[] = {0x48, 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x0C, 0xC1};
            size_t match      = FindPattern(module, pattern, "xxx????xxxx");
            if (match == module.size())
            {
                uint8_t fallbackPattern[] = {0x49, 0x63, 0x06, 0x48, 0x8D, 0x55, 0xE8, 0x48, 0x8B,
                                             0x0D, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x0C, 0xC1};
                match                     = FindPattern(module, fallbackPattern, "xxxxxxxxxx????xxxx");
                if (match != module.size())
                {
                    int displacement;
                    memcpy(&displacement, module.data() + match + 10, sizeof(displacement));
                    int64_t offset = (int64_t) match + 14 + displacement;
                    if (offset > 0 && (uint64_t) offset < module.size())
                    {
                        g_namesOffset = (uint64_t) offset;
                    }
                }
            }
            else
            {
                int displacement;
                memcpy(&displacement, module.data() + match + 3, sizeof(displacement));
                int64_t offset = (int64_t) match + 7 + displacement;
                if (offset > 0 && (uint64_t) offset < module.size())
                {
                    g_namesOffset = (uint64_t) offset;
                }
            }
        }

        if (g_buildDateOffset == 0 || g_psyonixBuildIdOffset == 0)
        {
            for (size_t stringOffset = 0; stringOffset + 20 <= module.size(); stringOffset++)
            {
                if (!isalpha((uint8_t) module[stringOffset]) || !isalpha((uint8_t) module[stringOffset + 1]) ||
                    !isalpha((uint8_t) module[stringOffset + 2]) || module[stringOffset + 3] != ' ' ||
                    !(isdigit((uint8_t) module[stringOffset + 4]) || module[stringOffset + 4] == ' ') ||
                    !isdigit((uint8_t) module[stringOffset + 5]) || module[stringOffset + 6] != ' ' || !isdigit((uint8_t) module[stringOffset + 7]) ||
                    !isdigit((uint8_t) module[stringOffset + 8]) || !isdigit((uint8_t) module[stringOffset + 9]) ||
                    !isdigit((uint8_t) module[stringOffset + 10]) || module[stringOffset + 11] != ' ' ||
                    !isdigit((uint8_t) module[stringOffset + 12]) || !isdigit((uint8_t) module[stringOffset + 13]) ||
                    module[stringOffset + 14] != ':' || !isdigit((uint8_t) module[stringOffset + 15]) ||
                    !isdigit((uint8_t) module[stringOffset + 16]) || module[stringOffset + 17] != ':' ||
                    !isdigit((uint8_t) module[stringOffset + 18]) || !isdigit((uint8_t) module[stringOffset + 19]))
                {
                    continue;
                }

                for (size_t pointerOffset = 0; pointerOffset + 0x38 <= module.size(); pointerOffset += 8)
                {
                    uint64_t buildDateAddress;
                    uint64_t buildIdAddress;
                    memcpy(&buildDateAddress, module.data() + pointerOffset, sizeof(buildDateAddress));
                    if (buildDateAddress != moduleBase + stringOffset)
                    {
                        continue;
                    }

                    memcpy(&buildIdAddress, module.data() + pointerOffset + 0x30, sizeof(buildIdAddress));
                    if (buildIdAddress == 0)
                    {
                        continue;
                    }

                    std::vector<char16_t> buildId = ReadMemoryArray<char16_t>(buildIdAddress, 20);
                    bool validBuildId             = true;
                    for (size_t i = 0; i < buildId.size(); i++)
                    {
                        if (i == 6 || i == 12)
                        {
                            if (buildId[i] != u'.')
                            {
                                validBuildId = false;
                                break;
                            }
                        }
                        else if (i == 19)
                        {
                            if (buildId[i] != 0)
                            {
                                validBuildId = false;
                            }
                        }
                        else if (!isdigit((uint8_t) buildId[i]))
                        {
                            validBuildId = false;
                            break;
                        }
                    }

                    if (validBuildId)
                    {
                        g_buildDateOffset      = pointerOffset;
                        g_psyonixBuildIdOffset = pointerOffset + 0x30;
                        break;
                    }
                }

                if (g_buildDateOffset != 0 && g_psyonixBuildIdOffset != 0)
                {
                    break;
                }
            }
        }
    }

    if (g_objectsOffset == 0 || g_namesOffset == 0 || g_buildDateOffset == 0 || g_psyonixBuildIdOffset == 0)
    {
        if (g_objectsOffset == 0)
        {
            fmt::println(stderr, "Failed to resolve GObjects");
        }

        if (g_namesOffset == 0)
        {
            fmt::println(stderr, "Failed to resolve GNames");
        }

        if (g_buildDateOffset == 0)
        {
            fmt::println(stderr, "Failed to resolve BuildDate");
        }

        if (g_psyonixBuildIdOffset == 0)
        {
            fmt::println(stderr, "Failed to resolve GPsyonixBuildID");
        }

        exit(EXIT_FAILURE);
    }

    if (!IsGObjectsValid() || !IsGNamesValid())
    {
        return false;
    }

    if (!m_rocketLeagueOffsetsResolved)
    {
        if (std::chrono::steady_clock::now() - m_offsetResolveStartTime >= std::chrono::seconds(60))
        {
            fmt::println(stderr, "Timed out while resolving Rocket League offsets");
            exit(EXIT_FAILURE);
        }

        if (!BuildClassAddresses())
        {
            return false;
        }

        if (!ValidateRlsdkLayout())
        {
            exit(EXIT_FAILURE);
        }

        fmt::println("GObjects: 0x{:X}", g_objectsOffset);
        fmt::println("GNames: 0x{:X}", g_namesOffset);
        fmt::println("BuildDate: 0x{:X}", g_buildDateOffset);
        fmt::println("GPsyonixBuildID: 0x{:X}", g_psyonixBuildIdOffset);
        ResolveRocketLeagueOffsets();
        m_rocketLeagueOffsetsResolved = true;
    }

    return true;
}

void OffsetResolver::ResolveRocketLeagueOffsets()
{
    uint64_t actorClass                    = GetClassAddress("Actor");
    uint64_t rigidBodyActorClass           = GetClassAddress("RBActor_TA");
    uint64_t playerControllerClass         = GetClassAddress("PlayerController_TA");
    uint64_t cameraClass                   = GetClassAddress("Camera");
    uint64_t vehicleClass                  = GetClassAddress("Vehicle_TA");
    uint64_t vehicleSimClass               = GetClassAddress("VehicleSim_TA");
    uint64_t wheelClass                    = GetClassAddress("Wheel_TA");
    uint64_t boostComponentClass           = GetClassAddress("CarComponent_Boost_TA");
    uint64_t priClass                      = GetClassAddress("PRI_TA");
    uint64_t teamClass                     = GetClassAddress("Team_TA");
    uint64_t primitiveClass                = GetClassAddress("PrimitiveComponent");
    uint64_t ballClass                     = GetClassAddress("Ball_TA");
    uint64_t gameEventClass                = GetClassAddress("GameEvent_TA");
    uint64_t soccarGameEventClass          = GetClassAddress("GameEvent_Soccar_TA");
    uint64_t pickupClass                   = GetClassAddress("VehiclePickup_TA");
    uint64_t playerInputClass              = GetClassAddress("PlayerInput_TA");
    g_actorLocationOffset                  = GetPropertyOffset(actorClass, "Location");
    g_actorHiddenOffset                    = GetPropertyOffset(actorClass, "bHidden");
    g_actorHiddenFlag                      = GetBoolMask(actorClass, "bHidden");
    g_rigidBodyStateOffset                 = GetPropertyOffset(rigidBodyActorClass, "RBState");
    g_rigidBodyRotationOffset              = GetStructPropertyOffset(rigidBodyActorClass, "RBState", "Quaternion");
    g_rigidBodyLocationOffset              = GetStructPropertyOffset(rigidBodyActorClass, "RBState", "Location");
    g_rigidBodyLinearVelocityOffset        = GetStructPropertyOffset(rigidBodyActorClass, "RBState", "LinearVelocity");
    g_rigidBodyAngularVelocityOffset       = GetStructPropertyOffset(rigidBodyActorClass, "RBState", "AngularVelocity");
    g_rigidBodyMaxLinearSpeedOffset        = GetPropertyOffset(rigidBodyActorClass, "MaxLinearSpeed");
    g_rigidBodyMaxAngularSpeedOffset       = GetPropertyOffset(rigidBodyActorClass, "MaxAngularSpeed");
    g_rigidBodyRollingFrictionOffset       = GetPropertyOffset(rigidBodyActorClass, "RollingFriction");
    g_playerCameraOffset                   = GetPropertyOffset(playerControllerClass, "PlayerCamera");
    g_cameraCacheOffset                    = GetPropertyOffset(cameraClass, "CameraCache");
    g_cameraCachePovOffset                 = GetStructPropertyOffset(cameraClass, "CameraCache", "POV");
    g_playerCarOffset                      = GetPropertyOffset(playerControllerClass, "Car");
    g_carVehicleSimOffset                  = GetPropertyOffset(vehicleClass, "VehicleSim");
    g_carMeshOffset                        = GetPropertyOffset(vehicleClass, "CarMesh");
    g_carStateFlagsOffset                  = GetPropertyOffset(vehicleClass, "bOnGround");
    g_carPriOffset                         = GetPropertyOffset(vehicleClass, "PRI");
    g_carLocalCollisionOffset              = GetPropertyOffset(vehicleClass, "LocalCollisionOffset");
    g_carLocalCollisionExtentOffset        = GetPropertyOffset(vehicleClass, "LocalCollisionExtent");
    g_carBoostComponentOffset              = GetPropertyOffset(vehicleClass, "BoostComponent");
    g_vehicleSimWheelsOffset               = GetPropertyOffset(vehicleSimClass, "Wheels");
    g_wheelRadiusOffset                    = GetPropertyOffset(wheelClass, "WheelRadius");
    g_wheelLocalRestPositionOffset         = GetPropertyOffset(wheelClass, "LocalRestPosition");
    g_boostMaxAmountOffset                 = GetPropertyOffset(boostComponentClass, "MaxBoostAmount");
    g_boostCurrentAmountOffset             = GetPropertyOffset(boostComponentClass, "CurrentBoostAmount");
    g_boostReplicatedAmountOffset          = GetPropertyOffset(boostComponentClass, "ReplicatedBoostAmount");
    g_priTeamOffset                        = GetPropertyOffset(priClass, "Team");
    g_priCarOffset                         = GetPropertyOffset(priClass, "Car");
    g_teamIndexOffset                      = GetPropertyOffset(teamClass, "TeamIndex");
    g_carOnGroundFlag                      = GetBoolMask(vehicleClass, "bOnGround");
    g_carDoubleJumpedFlag                  = GetBoolMask(vehicleClass, "bDoubleJumped");
    g_primitiveComponentLocalToWorldOffset = GetPropertyOffset(primitiveClass, "LocalToWorld");
    g_ballMagnusCoefficientOffset          = GetPropertyOffset(ballClass, "MagnusCoefficient");
    g_ballMagnusMinSpeedOffset             = GetPropertyOffset(ballClass, "MagnusMinSpeed");
    g_ballAngularVelocityDampingOffset     = GetPropertyOffset(ballClass, "AngularVelocityDamping");
    g_ballRadiusOffset                     = GetPropertyOffset(ballClass, "Radius");
    g_ballWorldBounceScaleOffset           = GetPropertyOffset(ballClass, "ReplicatedWorldBounceScale");
    g_ballGravityScaleOffset               = GetPropertyOffset(ballClass, "ReplicatedBallGravityScale");
    g_ballMaxLinearSpeedScaleOffset        = GetPropertyOffset(ballClass, "ReplicatedBallMaxLinearSpeedScale");
    g_ballAirResistanceOffset              = GetPropertyOffset(ballClass, "AirResistance");
    g_ballMinResistanceVelocityOffset      = GetPropertyOffset(ballClass, "MinResistanceVelocity");
    g_ballPredictedPositionsOffset         = GetPropertyOffset(ballClass, "PredictedPositions");
    g_gameEventFlagsOffset                 = GetPropertyOffset(soccarGameEventClass, "bRoundActive");
    g_gameEventRoundActiveFlag             = GetBoolMask(soccarGameEventClass, "bRoundActive");
    g_gameEventBallHasBeenHitFlag          = GetBoolMask(soccarGameEventClass, "bBallHasBeenHit");
    g_gameEventCarsOffset                  = GetPropertyOffset(gameEventClass, "Cars");
    g_gameEventPrisOffset                  = GetPropertyOffset(gameEventClass, "PRIs");
    g_pickupRespawnDelayOffset             = GetPropertyOffset(pickupClass, "RespawnDelay");
    g_pickupPickedUpOffset =
            GetPropertyOffset(pickupClass, "NewReplicatedPickupData") + GetStructPropertyOffset(pickupClass, "NewReplicatedPickupData", "PickedUp");
    g_playerInputOffset             = GetPropertyOffset(playerControllerClass, "PlayerInput");
    g_gamepadBindingsOffset         = GetPropertyOffset(playerInputClass, "GamepadBindings");
    g_playerBindingSize             = GetArrayStructElementSize(playerInputClass, "GamepadBindings");
    g_playerBindingActionNameOffset = GetArrayStructPropertyOffset(playerInputClass, "GamepadBindings", "Action");
    g_playerBindingKeyNameOffset    = GetArrayStructPropertyOffset(playerInputClass, "GamepadBindings", "Key");
    g_playerBindingAxisSignOffset   = GetArrayStructPropertyOffset(playerInputClass, "GamepadBindings", "AxisSign");

    const char *offsetNames[] = {"g_actorLocationOffset",
                                 "g_actorHiddenOffset",
                                 "g_actorHiddenFlag",
                                 "g_rigidBodyStateOffset",
                                 "g_rigidBodyRotationOffset",
                                 "g_rigidBodyLocationOffset",
                                 "g_rigidBodyLinearVelocityOffset",
                                 "g_rigidBodyAngularVelocityOffset",
                                 "g_rigidBodyMaxLinearSpeedOffset",
                                 "g_rigidBodyMaxAngularSpeedOffset",
                                 "g_rigidBodyRollingFrictionOffset",
                                 "g_playerCameraOffset",
                                 "g_cameraCacheOffset",
                                 "g_cameraCachePovOffset",
                                 "g_playerCarOffset",
                                 "g_carVehicleSimOffset",
                                 "g_carMeshOffset",
                                 "g_carStateFlagsOffset",
                                 "g_carPriOffset",
                                 "g_carLocalCollisionOffset",
                                 "g_carLocalCollisionExtentOffset",
                                 "g_carBoostComponentOffset",
                                 "g_vehicleSimWheelsOffset",
                                 "g_wheelRadiusOffset",
                                 "g_wheelLocalRestPositionOffset",
                                 "g_boostMaxAmountOffset",
                                 "g_boostCurrentAmountOffset",
                                 "g_boostReplicatedAmountOffset",
                                 "g_priTeamOffset",
                                 "g_priCarOffset",
                                 "g_teamIndexOffset",
                                 "g_carOnGroundFlag",
                                 "g_carDoubleJumpedFlag",
                                 "g_primitiveComponentLocalToWorldOffset",
                                 "g_ballMagnusCoefficientOffset",
                                 "g_ballMagnusMinSpeedOffset",
                                 "g_ballAngularVelocityDampingOffset",
                                 "g_ballRadiusOffset",
                                 "g_ballWorldBounceScaleOffset",
                                 "g_ballGravityScaleOffset",
                                 "g_ballMaxLinearSpeedScaleOffset",
                                 "g_ballAirResistanceOffset",
                                 "g_ballMinResistanceVelocityOffset",
                                 "g_ballPredictedPositionsOffset",
                                 "g_gameEventFlagsOffset",
                                 "g_gameEventRoundActiveFlag",
                                 "g_gameEventBallHasBeenHitFlag",
                                 "g_gameEventCarsOffset",
                                 "g_gameEventPrisOffset",
                                 "g_pickupRespawnDelayOffset",
                                 "g_pickupPickedUpOffset",
                                 "g_playerInputOffset",
                                 "g_gamepadBindingsOffset",
                                 "g_playerBindingSize",
                                 "g_playerBindingActionNameOffset",
                                 "g_playerBindingKeyNameOffset",
                                 "g_playerBindingAxisSignOffset"};
    uint64_t offsetValues[]   = {g_actorLocationOffset,
                                 g_actorHiddenOffset,
                                 (uint64_t) g_actorHiddenFlag,
                                 g_rigidBodyStateOffset,
                                 g_rigidBodyRotationOffset,
                                 g_rigidBodyLocationOffset,
                                 g_rigidBodyLinearVelocityOffset,
                                 g_rigidBodyAngularVelocityOffset,
                                 g_rigidBodyMaxLinearSpeedOffset,
                                 g_rigidBodyMaxAngularSpeedOffset,
                                 g_rigidBodyRollingFrictionOffset,
                                 g_playerCameraOffset,
                                 g_cameraCacheOffset,
                                 g_cameraCachePovOffset,
                                 g_playerCarOffset,
                                 g_carVehicleSimOffset,
                                 g_carMeshOffset,
                                 g_carStateFlagsOffset,
                                 g_carPriOffset,
                                 g_carLocalCollisionOffset,
                                 g_carLocalCollisionExtentOffset,
                                 g_carBoostComponentOffset,
                                 g_vehicleSimWheelsOffset,
                                 g_wheelRadiusOffset,
                                 g_wheelLocalRestPositionOffset,
                                 g_boostMaxAmountOffset,
                                 g_boostCurrentAmountOffset,
                                 g_boostReplicatedAmountOffset,
                                 g_priTeamOffset,
                                 g_priCarOffset,
                                 g_teamIndexOffset,
                                 (uint64_t) g_carOnGroundFlag,
                                 (uint64_t) g_carDoubleJumpedFlag,
                                 g_primitiveComponentLocalToWorldOffset,
                                 g_ballMagnusCoefficientOffset,
                                 g_ballMagnusMinSpeedOffset,
                                 g_ballAngularVelocityDampingOffset,
                                 g_ballRadiusOffset,
                                 g_ballWorldBounceScaleOffset,
                                 g_ballGravityScaleOffset,
                                 g_ballMaxLinearSpeedScaleOffset,
                                 g_ballAirResistanceOffset,
                                 g_ballMinResistanceVelocityOffset,
                                 g_ballPredictedPositionsOffset,
                                 g_gameEventFlagsOffset,
                                 (uint64_t) g_gameEventRoundActiveFlag,
                                 (uint64_t) g_gameEventBallHasBeenHitFlag,
                                 g_gameEventCarsOffset,
                                 g_gameEventPrisOffset,
                                 g_pickupRespawnDelayOffset,
                                 g_pickupPickedUpOffset,
                                 g_playerInputOffset,
                                 g_gamepadBindingsOffset,
                                 g_playerBindingSize,
                                 g_playerBindingActionNameOffset,
                                 g_playerBindingKeyNameOffset,
                                 g_playerBindingAxisSignOffset};
    for (size_t i = 0; i < sizeof(offsetNames) / sizeof(offsetNames[0]); i++)
    {
        fmt::println("{}: 0x{:X}", offsetNames[i], offsetValues[i]);
    }
}

bool OffsetResolver::BuildClassAddresses()
{
    const char *classNames[]        = {"Actor",
                                       "RBActor_TA",
                                       "PlayerController_TA",
                                       "Camera",
                                       "Vehicle_TA",
                                       "VehicleSim_TA",
                                       "Wheel_TA",
                                       "CarComponent_Boost_TA",
                                       "PRI_TA",
                                       "Team_TA",
                                       "PrimitiveComponent",
                                       "Ball_TA",
                                       "GameEvent_TA",
                                       "GameEvent_Soccar_TA",
                                       "VehiclePickup_TA",
                                       "PlayerInput_TA"};
    std::vector<uint64_t> addresses = ObjectArray::GetAllObjectAddresses();
    for (size_t i = 0; i < addresses.size(); i++)
    {
        RemoteObject object(addresses[i]);
        if (!object.IsValidAddress())
        {
            continue;
        }

        std::string name = object.GetName();
        for (size_t classIndex = 0; classIndex < sizeof(classNames) / sizeof(classNames[0]); classIndex++)
        {
            if (name == classNames[classIndex])
            {
                m_classAddresses.emplace(name, object.GetAddress());
                break;
            }
        }
    }

    return m_classAddresses.size() == sizeof(classNames) / sizeof(classNames[0]);
}

uint64_t OffsetResolver::GetClassAddress(const char *className) const
{
    auto classAddress = m_classAddresses.find(className);
    if (classAddress == m_classAddresses.end())
    {
        fmt::println(stderr, "Failed to resolve class {}", className);
        exit(EXIT_FAILURE);
    }

    return classAddress->second;
}

uint64_t OffsetResolver::FindPropertyAddress(uint64_t classAddress, const char *propertyName) const
{
    while (classAddress != 0)
    {
        uint64_t propertyAddress = ReadMemory<uint64_t>(classAddress + kUStructChildrenOffset);
        while (propertyAddress != 0)
        {
            if (NameTable::Resolve(ReadMemory<int>(propertyAddress + kObjectNameOffset)) == propertyName)
            {
                return propertyAddress;
            }

            propertyAddress = ReadMemory<uint64_t>(propertyAddress + kUFieldNextOffset);
        }

        classAddress = ReadMemory<uint64_t>(classAddress + kUStructSuperFieldOffset);
    }

    return 0;
}

uint64_t OffsetResolver::GetPropertyOffset(uint64_t classAddress, const char *propertyName) const
{
    uint64_t propertyAddress = FindPropertyAddress(classAddress, propertyName);
    if (propertyAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    int offset = ReadMemory<int>(propertyAddress + kUPropertyOffsetOffset);
    if (offset < 0)
    {
        fmt::println(stderr, "Failed to resolve offset for property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    return (uint64_t) offset;
}

uint64_t OffsetResolver::GetStructPropertyOffset(uint64_t classAddress, const char *propertyName, const char *memberName) const
{
    uint64_t propertyAddress = FindPropertyAddress(classAddress, propertyName);
    if (propertyAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    uint64_t memberAddress = FindPropertyAddress(ReadMemory<uint64_t>(propertyAddress + kUStructPropertyStructOffset), memberName);
    if (memberAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}.{}", propertyName, memberName);
        exit(EXIT_FAILURE);
    }

    int offset = ReadMemory<int>(memberAddress + kUPropertyOffsetOffset);
    if (offset < 0)
    {
        fmt::println(stderr, "Failed to resolve offset for property {}.{}", propertyName, memberName);
        exit(EXIT_FAILURE);
    }

    return (uint64_t) offset;
}

uint64_t OffsetResolver::GetArrayStructPropertyOffset(uint64_t classAddress, const char *propertyName, const char *memberName) const
{
    uint64_t propertyAddress = FindPropertyAddress(classAddress, propertyName);
    if (propertyAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    uint64_t innerPropertyAddress = ReadMemory<uint64_t>(propertyAddress + kUArrayPropertyInnerOffset);
    uint64_t memberAddress        = FindPropertyAddress(ReadMemory<uint64_t>(innerPropertyAddress + kUStructPropertyStructOffset), memberName);
    if (memberAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}.{}", propertyName, memberName);
        exit(EXIT_FAILURE);
    }

    int offset = ReadMemory<int>(memberAddress + kUPropertyOffsetOffset);
    if (offset < 0)
    {
        fmt::println(stderr, "Failed to resolve offset for property {}.{}", propertyName, memberName);
        exit(EXIT_FAILURE);
    }

    return (uint64_t) offset;
}

uint64_t OffsetResolver::GetArrayStructElementSize(uint64_t classAddress, const char *propertyName) const
{
    uint64_t propertyAddress = FindPropertyAddress(classAddress, propertyName);
    if (propertyAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    int elementSize = ReadMemory<int>(ReadMemory<uint64_t>(propertyAddress + kUArrayPropertyInnerOffset) + kUPropertyElementSizeOffset);
    if (elementSize <= 0)
    {
        fmt::println(stderr, "Failed to resolve element size for property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    return (uint64_t) elementSize;
}

uint32_t OffsetResolver::GetBoolMask(uint64_t classAddress, const char *propertyName) const
{
    uint64_t propertyAddress = FindPropertyAddress(classAddress, propertyName);
    if (propertyAddress == 0)
    {
        fmt::println(stderr, "Failed to resolve property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    uint32_t bitMask = ReadMemory<uint32_t>(propertyAddress + kUBoolPropertyBitMaskOffset);
    if (bitMask == 0)
    {
        fmt::println(stderr, "Failed to resolve bit mask for property {}", propertyName);
        exit(EXIT_FAILURE);
    }

    return bitMask;
}

bool OffsetResolver::ValidateRlsdkLayout() const
{
    uint64_t actorClassAddress = GetClassAddress("Actor");
    uint64_t classAddress      = ReadMemory<uint64_t>(actorClassAddress + kObjectClassOffset);
    if (classAddress == 0 || NameTable::Resolve(ReadMemory<int>(classAddress + kObjectNameOffset)) != "Class")
    {
        fmt::println(stderr, "RLSDK validation failed: UObject::Class is invalid");
        return false;
    }

    uint64_t flags = ReadMemory<uint64_t>(actorClassAddress + kObjectFlagsOffset);
    if ((flags & RF_BadObjectFlags) != 0)
    {
        fmt::println(stderr, "RLSDK validation failed: GameDefines object flags are invalid");
        return false;
    }

    uint64_t propertyAddress = FindPropertyAddress(actorClassAddress, "Location");
    if (propertyAddress == 0)
    {
        fmt::println(stderr, "RLSDK validation failed: Actor.Location is missing");
        return false;
    }

    int propertyOffset = ReadMemory<int>(propertyAddress + kUPropertyOffsetOffset);
    if (propertyOffset < 0 || propertyOffset > 0x100000 || ReadMemory<int>(propertyAddress + kUPropertyElementSizeOffset) != (int) sizeof(FVector))
    {
        fmt::println(stderr, "RLSDK validation failed: UProperty layout is invalid");
        return false;
    }

    return true;
}

size_t OffsetResolver::FindPattern(const std::vector<uint8_t> &module, const uint8_t *pattern, const char *mask) const
{
    size_t patternSize = strlen(mask);
    for (size_t moduleOffset = 0; moduleOffset + patternSize <= module.size(); moduleOffset++)
    {
        size_t patternOffset = 0;
        for (; patternOffset < patternSize; patternOffset++)
        {
            if (mask[patternOffset] != '?' && module[moduleOffset + patternOffset] != pattern[patternOffset])
            {
                break;
            }
        }

        if (patternOffset == patternSize)
        {
            return moduleOffset;
        }
    }

    return module.size();
}

bool OffsetResolver::IsGObjectsValid() const
{
    if (g_objectsOffset == 0)
    {
        return false;
    }

    uint64_t moduleBase = NerineGlobals::GetInstance().GetModuleBase();
    uint64_t data       = ReadMemory<uint64_t>(moduleBase + g_objectsOffset);
    int count           = ReadMemory<int>(moduleBase + g_objectsOffset + 0x8);
    return data != 0 && count > 0 && count < 2000000;
}

bool OffsetResolver::IsGNamesValid() const
{
    if (g_namesOffset == 0)
    {
        return false;
    }

    uint64_t moduleBase = NerineGlobals::GetInstance().GetModuleBase();
    uint64_t data       = ReadMemory<uint64_t>(moduleBase + g_namesOffset);
    if (data == 0)
    {
        return false;
    }

    uint64_t entry = ReadMemory<uint64_t>(data);
    return entry != 0 && ReadMemory<char16_t>(entry + 0x18) == u'N' && ReadMemory<char16_t>(entry + 0x1A) == u'o' &&
           ReadMemory<char16_t>(entry + 0x1C) == u'n' && ReadMemory<char16_t>(entry + 0x1E) == u'e';
}
