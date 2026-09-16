#ifndef VIRTUAL_GAMEPAD_HPP
#define VIRTUAL_GAMEPAD_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <toml.hpp>

struct GamepadDevice
{
    std::string path;
    std::string name;
    std::string identity;
    uint16_t vendor;
};

class VirtualGamepad
{
public:
    VirtualGamepad();

    bool AutoStart();
    bool SetBotInputEnabled(bool enabled);
    bool UpdateBotInput(int type, int code, int value);

private:
    void RefreshDevices();
    bool Start();
    void Stop();

    std::vector<GamepadDevice> m_devices;
    int m_selectedDevice;
    std::string m_deviceName;
    std::string m_deviceIdentity;
};

#endif
