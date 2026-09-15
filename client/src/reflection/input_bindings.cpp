#include "input_bindings.hpp"

#include "local_player.hpp"
#include "name_table.hpp"
#include "nerine_memory.hpp"
#include "offsets.hpp"

#include <fmt/base.h>

constexpr uint64_t kPlayerInputOffset        = 0x5B0;
constexpr uint64_t kGamepadBindingsOffset    = 0x388;
constexpr int kMaxGamepadBindings            = 256;
constexpr int kPlayerBindingSize             = 0x2C;
constexpr int kPlayerBindingActionNameOffset = 0x0;
constexpr int kPlayerBindingKeyNameOffset    = 0x8;
constexpr int kPlayerBindingAxisSignOffset   = 0x10;

struct RemoteArray
{
    uint64_t Data;
    int Count;
    int Capacity;
};

std::vector<GamepadBinding> InputBindings::GetGamepadBindings()
{
    LocalPlayer localPlayer = LocalPlayer::Find();
    std::vector<GamepadBinding> bindings;

    if (!localPlayer)
    {
        return bindings;
    }

    uint64_t playerInput = ReadMemory<uint64_t>(localPlayer.GetAddress() + kPlayerInputOffset);
    if (!playerInput)
    {
        return bindings;
    }

    RemoteArray bindingArray = ReadMemory<RemoteArray>(playerInput + kGamepadBindingsOffset);
    if (!bindingArray.Data || bindingArray.Count <= 0 || bindingArray.Count > kMaxGamepadBindings || bindingArray.Capacity < bindingArray.Count ||
        bindingArray.Capacity > kMaxGamepadBindings)
    {
        return bindings;
    }

    bindings.reserve(bindingArray.Count);
    for (int i = 0; i < bindingArray.Count; i++)
    {
        uint64_t bindingAddress = bindingArray.Data + i * kPlayerBindingSize;
        GamepadBinding binding;

        binding.Action   = NameTable::Resolve(ReadMemory<int>(bindingAddress + kPlayerBindingActionNameOffset));
        binding.Key      = NameTable::Resolve(ReadMemory<int>(bindingAddress + kPlayerBindingKeyNameOffset));
        binding.AxisSign = ReadMemory<uint8_t>(bindingAddress + kPlayerBindingAxisSignOffset);
        bindings.push_back(binding);
    }

    return bindings;
}

void InputBindings::Debug()
{
    std::vector<GamepadBinding> bindings = GetGamepadBindings();

    fmt::println("--- InputBindings::Debug ---");
    fmt::println("gamepad binding count = {}", bindings.size());
    for (size_t i = 0; i < bindings.size(); i++)
    {
        fmt::println("#{} action '{}' key '{}' axis sign {}", i, bindings[i].Action, bindings[i].Key, bindings[i].AxisSign);
    }
}
