#include "input_bindings.hpp"

#include "local_player.hpp"
#include "name_table.hpp"
#include "nerine_memory.hpp"
#include "offsets.hpp"

constexpr int kMaxGamepadBindings = 256;

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

    uint64_t playerInput = ReadMemory<uint64_t>(localPlayer.GetAddress() + g_playerInputOffset);
    if (!playerInput)
    {
        return bindings;
    }

    RemoteArray bindingArray = ReadMemory<RemoteArray>(playerInput + g_gamepadBindingsOffset);
    if (!bindingArray.Data || bindingArray.Count <= 0 || bindingArray.Count > kMaxGamepadBindings || bindingArray.Capacity < bindingArray.Count ||
        bindingArray.Capacity > kMaxGamepadBindings)
    {
        return bindings;
    }

    bindings.reserve(bindingArray.Count);
    for (int i = 0; i < bindingArray.Count; i++)
    {
        uint64_t bindingAddress = bindingArray.Data + i * g_playerBindingSize;
        GamepadBinding binding;

        binding.Action   = NameTable::Resolve(ReadMemory<int>(bindingAddress + g_playerBindingActionNameOffset));
        binding.Key      = NameTable::Resolve(ReadMemory<int>(bindingAddress + g_playerBindingKeyNameOffset));
        binding.AxisSign = ReadMemory<uint8_t>(bindingAddress + g_playerBindingAxisSignOffset);
        bindings.push_back(binding);
    }

    return bindings;
}
