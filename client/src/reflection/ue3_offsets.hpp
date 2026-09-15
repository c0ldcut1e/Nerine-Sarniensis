#ifndef UE3_OFFSETS_HPP
#define UE3_OFFSETS_HPP

#include <cstdint>

constexpr uint64_t kObjectFlagsOffset           = 0x10;
constexpr uint64_t kObjectNameOffset            = 0x48;
constexpr uint64_t kObjectClassOffset           = 0x50;
constexpr uint64_t kNameEntryNameOffset         = 0x18;
constexpr uint64_t kUFieldNextOffset            = 0x60;
constexpr uint64_t kUStructSuperFieldOffset     = 0x80;
constexpr uint64_t kUStructChildrenOffset       = 0x88;
constexpr uint64_t kUPropertyElementSizeOffset  = 0x74;
constexpr uint64_t kUPropertyOffsetOffset       = 0x98;
constexpr uint64_t kUStructPropertyStructOffset = 0xC8;
constexpr uint64_t kUArrayPropertyInnerOffset   = 0xC8;
constexpr uint64_t kUBoolPropertyBitMaskOffset  = 0xC8;

#endif
