#ifndef OFFSETS_HPP
#define OFFSETS_HPP

#include <cstdint>

constexpr uint64_t kGObjectsOffset        = 0x02418190;
constexpr uint64_t kGNamesOffset          = 0x02418148;
constexpr uint64_t kBuildDateOffset       = 0x021DD0F8;
constexpr uint64_t kGPsyonixBuildIdOffset = 0x021DD128;

constexpr uint64_t kObjectFlagsOffset = 0x10;
constexpr uint64_t kObjectNameOffset  = 0x48;
constexpr uint64_t kObjectClassOffset = 0x50;

constexpr uint64_t kActorLocationOffset = 0x90;
constexpr uint64_t kActorHiddenOffset   = 0xF8;
constexpr uint32_t kActorHiddenFlag     = 1 << 1;

constexpr uint64_t kNameEntryNameOffset = 0x18;

constexpr uint64_t kRigidBodyStateOffset           = 0x5D0;
constexpr uint64_t kRigidBodyRotationOffset        = 0x0;
constexpr uint64_t kRigidBodyLocationOffset        = 0x10;
constexpr uint64_t kRigidBodyLinearVelocityOffset  = 0x1C;
constexpr uint64_t kRigidBodyAngularVelocityOffset = 0x28;

constexpr uint64_t kRigidBodyMaxLinearSpeedOffset  = 0x560;
constexpr uint64_t kRigidBodyMaxAngularSpeedOffset = 0x564;
constexpr uint64_t kRigidBodyRollingFrictionOffset = 0x568;

constexpr uint64_t kPlayerCameraOffset   = 0x480;
constexpr uint64_t kCameraCacheOffset    = 0x438;
constexpr uint64_t kCameraCachePovOffset = 0x4;
constexpr uint64_t kPlayerCarOffset      = 0x9A0;

constexpr uint64_t kCarVehicleSimOffset           = 0x7E0;
constexpr uint64_t kCarMeshOffset                 = 0x7D8;
constexpr uint64_t kCarStateFlagsOffset           = 0x7F8;
constexpr uint64_t kCarPriOffset                  = 0x830;
constexpr uint64_t kCarLocalCollisionOffset       = 0x850;
constexpr uint64_t kCarLocalCollisionExtentOffset = 0x85C;
constexpr uint64_t kCarBoostComponentOffset       = 0x870;
constexpr uint64_t kVehicleSimWheelsOffset        = 0xA0;
constexpr uint64_t kWheelRadiusOffset             = 0x74;
constexpr uint64_t kWheelLocalRestPositionOffset  = 0x144;

constexpr uint64_t kBoostMaxAmountOffset        = 0x32C;
constexpr uint64_t kBoostCurrentAmountOffset    = 0x338;
constexpr uint64_t kBoostReplicatedAmountOffset = 0x361;

constexpr uint64_t kPriTeamOffset       = 0x2B0;
constexpr uint64_t kPriCarOffset        = 0x498;
constexpr uint64_t kTeamIndexOffset     = 0x280;
constexpr uint32_t kCarOnGroundFlag     = 1 << 4;
constexpr uint32_t kCarDoubleJumpedFlag = 1 << 3;

constexpr uint64_t kPrimitiveComponentLocalToWorldOffset = 0xD0;

constexpr uint64_t kBallMagnusCoefficientOffset      = 0x864;
constexpr uint64_t kBallMagnusMinSpeedOffset         = 0x870;
constexpr uint64_t kBallAngularVelocityDampingOffset = 0x874;
constexpr uint64_t kBallRadiusOffset                 = 0x888;
constexpr uint64_t kBallWorldBounceScaleOffset       = 0x8D8;
constexpr uint64_t kBallGravityScaleOffset           = 0x8DC;
constexpr uint64_t kBallMaxLinearSpeedScaleOffset    = 0x8E0;
constexpr uint64_t kBallAirResistanceOffset          = 0x9E8;
constexpr uint64_t kBallMinResistanceVelocityOffset  = 0x9F4;

constexpr uint64_t kGameEventFlagsOffset        = 0x828;
constexpr uint32_t kGameEventRoundActiveFlag    = 1 << 2;
constexpr uint32_t kGameEventBallHasBeenHitFlag = 1 << 4;
constexpr uint64_t kGameEventCarsOffset         = 0x350;
constexpr uint64_t kGameEventPrisOffset         = 0x340;

constexpr uint64_t kPickupRespawnDelayOffset = 0x26C;
constexpr uint64_t kPickupPickedUpOffset     = 0x2B0;

#endif
