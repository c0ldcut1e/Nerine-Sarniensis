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
    uint64_t camera = ReadMemory<uint64_t>(m_address + g_playerCameraOffset);
    return ReadMemory<FTPOV>(camera + g_cameraCacheOffset + g_cameraCachePovOffset);
}

Car LocalPlayer::GetCar() const
{
    return Car(ReadMemory<uint64_t>(m_address + g_playerCarOffset));
}

LocalPlayer LocalPlayer::Find()
{
    return LocalPlayer(WorldScanner::GetLocalPlayerAddress());
}
