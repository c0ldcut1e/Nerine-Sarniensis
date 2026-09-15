#include "local_player.hpp"

#include "nerine_memory.hpp"
#include "offsets.hpp"
#include "world_scanner.hpp"

LocalPlayer::LocalPlayer()
{}

LocalPlayer::LocalPlayer(uint64_t address)
{
    m_address = address;
}

FTPOV LocalPlayer::GetCameraPOV() const
{
    uint64_t camera = ReadMemory<uint64_t>(m_address + kPlayerCameraOffset);
    return ReadMemory<FTPOV>(camera + kCameraCacheOffset + kCameraCachePovOffset);
}

Car LocalPlayer::GetCar() const
{
    return Car(ReadMemory<uint64_t>(m_address + kPlayerCarOffset));
}

LocalPlayer LocalPlayer::Find()
{
    return LocalPlayer(WorldScanner::GetLocalPlayerAddress());
}
