#ifndef OFFSETS_HPP
#define OFFSETS_HPP

#include <cstdint>

extern uint64_t g_objectsOffset;
extern uint64_t g_namesOffset;
extern uint64_t g_buildDateOffset;
extern uint64_t g_psyonixBuildIdOffset;

constexpr uint64_t kObjectFlagsOffset   = 0x10;
constexpr uint64_t kObjectNameOffset    = 0x48;
constexpr uint64_t kObjectClassOffset   = 0x50;
constexpr uint64_t kNameEntryNameOffset = 0x18;

extern uint64_t g_actorLocationOffset;
extern uint64_t g_actorHiddenOffset;
extern uint32_t g_actorHiddenFlag;
extern uint64_t g_rigidBodyStateOffset;
extern uint64_t g_rigidBodyRotationOffset;
extern uint64_t g_rigidBodyLocationOffset;
extern uint64_t g_rigidBodyLinearVelocityOffset;
extern uint64_t g_rigidBodyAngularVelocityOffset;
extern uint64_t g_rigidBodyMaxLinearSpeedOffset;
extern uint64_t g_rigidBodyMaxAngularSpeedOffset;
extern uint64_t g_rigidBodyRollingFrictionOffset;
extern uint64_t g_playerCameraOffset;
extern uint64_t g_cameraCacheOffset;
extern uint64_t g_cameraCachePovOffset;
extern uint64_t g_playerCarOffset;
extern uint64_t g_carVehicleSimOffset;
extern uint64_t g_carMeshOffset;
extern uint64_t g_carStateFlagsOffset;
extern uint64_t g_carPriOffset;
extern uint64_t g_carLocalCollisionOffset;
extern uint64_t g_carLocalCollisionExtentOffset;
extern uint64_t g_carBoostComponentOffset;
extern uint64_t g_vehicleSimWheelsOffset;
extern uint64_t g_wheelRadiusOffset;
extern uint64_t g_wheelLocalRestPositionOffset;
extern uint64_t g_boostMaxAmountOffset;
extern uint64_t g_boostCurrentAmountOffset;
extern uint64_t g_boostReplicatedAmountOffset;
extern uint64_t g_priTeamOffset;
extern uint64_t g_priCarOffset;
extern uint64_t g_teamIndexOffset;
extern uint32_t g_carOnGroundFlag;
extern uint32_t g_carDoubleJumpedFlag;
extern uint64_t g_primitiveComponentLocalToWorldOffset;
extern uint64_t g_ballMagnusCoefficientOffset;
extern uint64_t g_ballMagnusMinSpeedOffset;
extern uint64_t g_ballAngularVelocityDampingOffset;
extern uint64_t g_ballRadiusOffset;
extern uint64_t g_ballWorldBounceScaleOffset;
extern uint64_t g_ballGravityScaleOffset;
extern uint64_t g_ballMaxLinearSpeedScaleOffset;
extern uint64_t g_ballAirResistanceOffset;
extern uint64_t g_ballMinResistanceVelocityOffset;
extern uint64_t g_ballPredictedPositionsOffset;
extern uint64_t g_gameEventFlagsOffset;
extern uint32_t g_gameEventRoundActiveFlag;
extern uint32_t g_gameEventBallHasBeenHitFlag;
extern uint64_t g_gameEventCarsOffset;
extern uint64_t g_gameEventPrisOffset;
extern uint64_t g_pickupRespawnDelayOffset;
extern uint64_t g_pickupPickedUpOffset;
extern uint64_t g_playerInputOffset;
extern uint64_t g_gamepadBindingsOffset;
extern uint64_t g_playerBindingSize;
extern uint64_t g_playerBindingActionNameOffset;
extern uint64_t g_playerBindingKeyNameOffset;
extern uint64_t g_playerBindingAxisSignOffset;

#endif
