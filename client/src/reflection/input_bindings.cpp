#include "input_bindings.hpp"

#include "core/nerine_memory.hpp"
#include "local_player.hpp"
#include "name_table.hpp"
#include "offset_resolver.hpp"

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

    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t playerControllerClass = offsetResolver->GetClassAddress("PlayerController_TA");
    uint64_t playerInputClass      = offsetResolver->GetClassAddress("PlayerInput_TA");
    uint64_t playerInput =
            ReadMemory<uint64_t>(localPlayer.GetAddress() + offsetResolver->GetPropertyOffsetCached(playerControllerClass, "PlayerInput"));
    if (!playerInput)
    {
        return bindings;
    }

    RemoteArray bindingArray = ReadMemory<RemoteArray>(playerInput + offsetResolver->GetPropertyOffsetCached(playerInputClass, "GamepadBindings"));
    if (!bindingArray.Data || bindingArray.Count <= 0 || bindingArray.Count > kMaxGamepadBindings || bindingArray.Capacity < bindingArray.Count ||
        bindingArray.Capacity > kMaxGamepadBindings)
    {
        return bindings;
    }

    bindings.reserve(bindingArray.Count);
    for (int i = 0; i < bindingArray.Count; i++)
    {
        uint64_t bindingAddress = bindingArray.Data + i * offsetResolver->GetArrayStructElementSizeCached(playerInputClass, "GamepadBindings");
        GamepadBinding binding;

        binding.Action = NameTable::Resolve(
                ReadMemory<int>(bindingAddress + offsetResolver->GetArrayStructPropertyOffsetCached(playerInputClass, "GamepadBindings", "Action")));
        binding.Key = NameTable::Resolve(
                ReadMemory<int>(bindingAddress + offsetResolver->GetArrayStructPropertyOffsetCached(playerInputClass, "GamepadBindings", "Key")));
        binding.AxisSign = ReadMemory<uint8_t>(bindingAddress +
                                               offsetResolver->GetArrayStructPropertyOffsetCached(playerInputClass, "GamepadBindings", "AxisSign"));
        bindings.push_back(binding);
    }

    return bindings;
}
