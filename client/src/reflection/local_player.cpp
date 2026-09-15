#include "local_player.hpp"

#include "nerine_memory.hpp"
#include "offset_resolver.hpp"
#include "world_scanner.hpp"

LocalPlayer::LocalPlayer()
{}

LocalPlayer::LocalPlayer(uint64_t address)
{
    m_address = address;
}

FTPOV LocalPlayer::GetCameraPOV() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t playerControllerClass = offsetResolver.GetClassAddress("PlayerController_TA");
    uint64_t cameraClass           = offsetResolver.GetClassAddress("Camera");
    uint64_t camera                = ReadMemory<uint64_t>(m_address + offsetResolver.GetPropertyOffsetCached(playerControllerClass, "PlayerCamera"));
    return ReadMemory<FTPOV>(camera + offsetResolver.GetPropertyOffsetCached(cameraClass, "CameraCache") +
                             offsetResolver.GetStructPropertyOffsetCached(cameraClass, "CameraCache", "POV"));
}

Car LocalPlayer::GetCar() const
{
    OffsetResolver &offsetResolver = OffsetResolver::GetInstance();
    uint64_t playerControllerClass = offsetResolver.GetClassAddress("PlayerController_TA");
    return Car(ReadMemory<uint64_t>(m_address + offsetResolver.GetPropertyOffsetCached(playerControllerClass, "Car")));
}

LocalPlayer LocalPlayer::Find()
{
    return LocalPlayer(WorldScanner::GetLocalPlayerAddress());
}
