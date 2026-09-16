#include "virtual_gamepad.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits.h>
#include <mutex>
#include <thread>

#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fmt/format.h>

constexpr const char *kUinputPath        = "/dev/uinput";
constexpr const char *kVirtualDeviceName = "Nerine Virtual Gamepad";
constexpr uint16_t kVirtualVendor        = 0x045E;
constexpr uint16_t kVirtualProduct       = 0x028E;
constexpr uint16_t kVirtualVersion       = 1;

struct VirtualAxis
{
    uint16_t code;
    int minimum;
    int maximum;
};

constexpr VirtualAxis kVirtualAxes[] = {
        {ABS_X, -32767, 32767}, {ABS_Y, -32767, 32767}, {ABS_RX, -32767, 32767}, {ABS_RY, -32767, 32767},
        {ABS_Z, 0, 255},        {ABS_RZ, 0, 255},       {ABS_HAT0X, -1, 1},      {ABS_HAT0Y, -1, 1},
};

constexpr uint16_t kVirtualButtons[] = {
        BTN_SOUTH, BTN_EAST, BTN_NORTH, BTN_WEST, BTN_TL, BTN_TR, BTN_SELECT, BTN_START, BTN_THUMBL, BTN_THUMBR, BTN_MODE,
};

static uint16_t g_sourceVendor;

static const VirtualAxis *FindVirtualAxis(uint16_t code)
{
    for (const VirtualAxis &axis : kVirtualAxes)
    {
        if (axis.code == code)
        {
            return &axis;
        }
    }

    return NULL;
}

static bool IsVirtualButton(uint16_t code)
{
    for (uint16_t button : kVirtualButtons)
    {
        if (button == code)
        {
            return true;
        }
    }

    return false;
}

static uint16_t TranslateSourceButton(uint16_t code)
{
    if (g_sourceVendor != 0x054C)
    {
        return code;
    }

    if (code == BTN_WEST)
    {
        return BTN_X;
    }

    if (code == BTN_NORTH)
    {
        return BTN_Y;
    }

    return code;
}

static int g_sourceDescriptor = -1;
static int g_uinputDescriptor = -1;
static std::atomic<bool> g_running;
static std::thread g_thread;
static std::string g_status;
static std::mutex g_mutex;
static std::mutex g_writeMutex;
static std::string g_virtualDevicePath;
static std::atomic<bool> g_botInputActive;
static input_absinfo g_sourceAxes[ABS_MAX + 1];
static bool g_sourceAxisSupported[ABS_MAX + 1];

static void SetStatus(const std::string &status)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_status = status;
    fmt::println("[VirtualGamepad] {}", status);
}

static std::string EncodeDeviceIdentity(const char *identity)
{
    constexpr char hexDigits[] = "0123456789abcdef";
    std::string encodedIdentity;
    for (size_t i = 0; identity[i] != '\0'; i++)
    {
        encodedIdentity += hexDigits[((uint8_t) identity[i]) >> 4];
        encodedIdentity += hexDigits[((uint8_t) identity[i]) & 0xF];
    }

    return encodedIdentity;
}

static bool SetRawHidGate(const char *command, const std::string &identity)
{
    pid_t process = fork();
    if (process < 0)
    {
        return false;
    }

    if (process == 0)
    {
        if (identity.empty())
        {
            execl("/usr/bin/sudo", "sudo", "-n", "/usr/local/libexec/nerine_hidraw_gate", command, NULL);
        }
        else
        {
            execl("/usr/bin/sudo", "sudo", "-n", "/usr/local/libexec/nerine_hidraw_gate", command, identity.c_str(), NULL);
        }
        _exit(1);
    }

    int status;
    return waitpid(process, &status, 0) == process && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static bool InstallRawHidGate()
{
    char executablePath[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", executablePath, sizeof(executablePath) - 1);
    if (length <= 0)
    {
        return false;
    }

    executablePath[length] = '\0';
    std::filesystem::path directory(executablePath);
    directory                       = directory.parent_path();
    std::filesystem::path setupPath = directory / "nerine_controller_setup";
    std::filesystem::path gatePath  = directory / "nerine_hidraw_gate";
    std::filesystem::path rulePath  = directory / "99-nerine_hidraw_gate.rules";
    if (!std::filesystem::is_regular_file(setupPath) || !std::filesystem::is_regular_file(gatePath) || !std::filesystem::is_regular_file(rulePath))
    {
        return false;
    }

    SetStatus("Installing controller support; approve the system password prompt");
    pid_t process = fork();
    if (process < 0)
    {
        return false;
    }

    if (process == 0)
    {
        execl("/usr/bin/pkexec", "pkexec", setupPath.c_str(), gatePath.c_str(), rulePath.c_str(), NULL);
        _exit(1);
    }

    int status;
    return waitpid(process, &status, 0) == process && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static bool HasBit(const uint8_t *bits, int bit)
{
    return (bits[bit / 8] & (1 << (bit % 8))) != 0;
}

static bool IsGamepad(int descriptor)
{
    uint8_t keyBits[KEY_MAX / 8 + 1];
    uint8_t absBits[ABS_MAX / 8 + 1];

    memset(keyBits, 0, sizeof(keyBits));
    memset(absBits, 0, sizeof(absBits));
    ioctl(descriptor, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits);
    ioctl(descriptor, EVIOCGBIT(EV_ABS, sizeof(absBits)), absBits);

    return (HasBit(keyBits, BTN_GAMEPAD) || HasBit(keyBits, BTN_JOYSTICK)) && HasBit(absBits, ABS_X);
}

static void WriteInput(uint16_t type, uint16_t code, int value)
{
    input_event events[2];
    memset(events, 0, sizeof(events));
    events[0].type  = type;
    events[0].code  = code;
    events[0].value = value;
    events[1].type  = EV_SYN;
    events[1].code  = SYN_REPORT;

    std::lock_guard<std::mutex> lock(g_writeMutex);
    write(g_uinputDescriptor, events, sizeof(events));
}

static bool CreateVirtualDevice()
{
    g_uinputDescriptor = open(kUinputPath, O_WRONLY | O_NONBLOCK);
    if (g_uinputDescriptor < 0)
    {
        SetStatus(fmt::format("Cannot open {}: {}", kUinputPath, strerror(errno)));
        return false;
    }

    ioctl(g_uinputDescriptor, UI_SET_EVBIT, EV_KEY);
    ioctl(g_uinputDescriptor, UI_SET_EVBIT, EV_ABS);
    ioctl(g_uinputDescriptor, UI_SET_EVBIT, EV_SYN);

    for (uint16_t button : kVirtualButtons)
    {
        ioctl(g_uinputDescriptor, UI_SET_KEYBIT, button);
    }

    for (const VirtualAxis &axis : kVirtualAxes)
    {
        uinput_abs_setup absSetup;
        memset(&absSetup, 0, sizeof(absSetup));
        absSetup.code            = axis.code;
        absSetup.absinfo.minimum = axis.minimum;
        absSetup.absinfo.maximum = axis.maximum;
        ioctl(g_uinputDescriptor, UI_ABS_SETUP, &absSetup);
    }

    uinput_setup setup;
    memset(&setup, 0, sizeof(setup));
    setup.id.bustype = BUS_USB;
    setup.id.vendor  = kVirtualVendor;
    setup.id.product = kVirtualProduct;
    setup.id.version = kVirtualVersion;
    snprintf(setup.name, sizeof(setup.name), "%s", kVirtualDeviceName);

    if (ioctl(g_uinputDescriptor, UI_DEV_SETUP, &setup) < 0 || ioctl(g_uinputDescriptor, UI_DEV_CREATE) < 0)
    {
        SetStatus(fmt::format("Failed to create virtual device: {}", strerror(errno)));
        close(g_uinputDescriptor);
        g_uinputDescriptor = -1;
        return false;
    }

    char sysName[64];
    memset(sysName, 0, sizeof(sysName));
    ioctl(g_uinputDescriptor, UI_GET_SYSNAME(sizeof(sysName)), sysName);

    g_virtualDevicePath.clear();
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(std::string("/sys/class/input/") + sysName))
    {
        if (entry.path().filename().string().rfind("event", 0) == 0)
        {
            g_virtualDevicePath = "/dev/input/" + entry.path().filename().string();
        }
    }

    fmt::println("[VirtualGamepad] Created '{}' {:04x}:{:04x} as {} ({})", kVirtualDeviceName, kVirtualVendor, kVirtualProduct, sysName,
                 g_virtualDevicePath);
    return true;
}

static int ScaleAxisValue(uint16_t code, int value)
{
    const VirtualAxis *axis = FindVirtualAxis(code);
    if (!g_sourceAxisSupported[code] || g_sourceAxes[code].maximum == g_sourceAxes[code].minimum)
    {
        return value;
    }

    return axis->minimum + (int) (((int64_t) (value - g_sourceAxes[code].minimum) * (axis->maximum - axis->minimum)) /
                                  (g_sourceAxes[code].maximum - g_sourceAxes[code].minimum));
}

static void ForwardLoop()
{
    pollfd sourcePoll;
    sourcePoll.fd     = g_sourceDescriptor;
    sourcePoll.events = POLLIN;

    while (g_running)
    {
        if (poll(&sourcePoll, 1, 20) <= 0 || (sourcePoll.revents & POLLIN) == 0)
        {
            continue;
        }

        input_event event;
        while (read(g_sourceDescriptor, &event, sizeof(event)) == sizeof(event))
        {
            if (event.type == EV_KEY && event.code <= KEY_MAX && IsVirtualButton(TranslateSourceButton(event.code)))
            {
                event.code = TranslateSourceButton(event.code);
            }
            else if (event.type != EV_ABS || event.code > ABS_MAX || !FindVirtualAxis(event.code))
            {
                continue;
            }

            WriteInput(event.type, event.code, event.type == EV_ABS ? ScaleAxisValue(event.code, event.value) : event.value);
        }
    }
}

VirtualGamepad::VirtualGamepad()
{
    m_selectedDevice = 0;
    g_status         = "Stopped";
    RefreshDevices();
}

void VirtualGamepad::RefreshDevices()
{
    m_devices.clear();

    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator("/dev/input"))
    {
        if (entry.path().filename().string().rfind("event", 0) != 0)
        {
            continue;
        }

        int descriptor = open(entry.path().c_str(), O_RDONLY | O_NONBLOCK);
        if (descriptor < 0)
        {
            continue;
        }

        char name[256];
        char identity[256];
        input_id inputId;
        memset(name, 0, sizeof(name));
        memset(identity, 0, sizeof(identity));
        memset(&inputId, 0, sizeof(inputId));
        ioctl(descriptor, EVIOCGNAME(sizeof(name)), name);
        ioctl(descriptor, EVIOCGUNIQ(sizeof(identity)), identity);
        if (identity[0] == '\0')
        {
            ioctl(descriptor, EVIOCGPHYS(sizeof(identity)), identity);
        }
        ioctl(descriptor, EVIOCGID, &inputId);

        if (IsGamepad(descriptor) && entry.path().string() != g_virtualDevicePath)
        {
            GamepadDevice device;
            device.path     = entry.path().string();
            device.name     = name;
            device.identity = EncodeDeviceIdentity(identity);
            device.vendor   = inputId.vendor;
            m_devices.push_back(device);
        }

        close(descriptor);
    }

    std::sort(m_devices.begin(), m_devices.end(),
              [](const GamepadDevice &first, const GamepadDevice &second)
              {
                  return first.path < second.path;
              });

    for (size_t i = 0; i < m_devices.size(); i++)
    {
        if (m_devices[i].name == m_deviceName)
        {
            m_selectedDevice = (int) i;
        }
    }

    m_selectedDevice = m_devices.empty() ? 0 : std::clamp(m_selectedDevice, 0, (int) m_devices.size() - 1);
}

bool VirtualGamepad::Start()
{
    if (g_running)
    {
        return true;
    }

    if (m_devices.empty())
    {
        SetStatus("No controller was found");
        return false;
    }

    m_deviceIdentity = m_devices[m_selectedDevice].identity;
    if (m_deviceIdentity.empty())
    {
        SetStatus("Selected controller has no stable identity");
        return false;
    }

    if (!SetRawHidGate("verify", "") && !InstallRawHidGate())
    {
        SetStatus("Controller support setup was cancelled or failed");
        return false;
    }

    if (!SetRawHidGate("block", m_deviceIdentity))
    {
        SetStatus("Cannot hide the controller raw-input device");
        return false;
    }

    usleep(250000);
    RefreshDevices();
    bool found = false;
    for (size_t i = 0; i < m_devices.size(); i++)
    {
        if (m_devices[i].identity == m_deviceIdentity)
        {
            m_selectedDevice = (int) i;
            found            = true;
            break;
        }
    }

    if (!found)
    {
        SetRawHidGate("restore", m_deviceIdentity);
        SetStatus("Controller did not return after raw-input interception");
        return false;
    }

    g_sourceDescriptor = open(m_devices[m_selectedDevice].path.c_str(), O_RDONLY | O_NONBLOCK);
    if (g_sourceDescriptor < 0)
    {
        SetStatus(fmt::format("Cannot open {}: {}", m_devices[m_selectedDevice].path, strerror(errno)));
        SetRawHidGate("restore", m_deviceIdentity);
        return false;
    }

    if (ioctl(g_sourceDescriptor, EVIOCGRAB, 1) < 0)
    {
        SetStatus(fmt::format("Cannot intercept {}: {}", m_devices[m_selectedDevice].path, strerror(errno)));
        close(g_sourceDescriptor);
        g_sourceDescriptor = -1;
        SetRawHidGate("restore", m_deviceIdentity);
        return false;
    }

    memset(g_sourceAxisSupported, 0, sizeof(g_sourceAxisSupported));
    g_botInputActive = false;
    for (const VirtualAxis &axis : kVirtualAxes)
    {
        if (ioctl(g_sourceDescriptor, EVIOCGABS(axis.code), &g_sourceAxes[axis.code]) == 0)
        {
            g_sourceAxisSupported[axis.code] = true;
        }
    }

    if (!CreateVirtualDevice())
    {
        ioctl(g_sourceDescriptor, EVIOCGRAB, 0);
        close(g_sourceDescriptor);
        g_sourceDescriptor = -1;
        SetRawHidGate("restore", m_deviceIdentity);
        return false;
    }

    m_deviceName   = m_devices[m_selectedDevice].name;
    g_sourceVendor = m_devices[m_selectedDevice].vendor;
    g_running      = true;
    g_thread       = std::thread(ForwardLoop);
    SetStatus(fmt::format("Intercepting '{}' through the virtual controller", m_deviceName));
    return true;
}

void VirtualGamepad::Stop()
{
    if (!g_running)
    {
        return;
    }

    g_running        = false;
    g_botInputActive = false;
    g_thread.join();

    ioctl(g_sourceDescriptor, EVIOCGRAB, 0);
    close(g_sourceDescriptor);
    g_sourceDescriptor = -1;

    ioctl(g_uinputDescriptor, UI_DEV_DESTROY);
    close(g_uinputDescriptor);
    g_uinputDescriptor = -1;

    if (SetRawHidGate("restore", m_deviceIdentity))
    {
        SetStatus("Stopped");
    }
    else
    {
        SetStatus("Stopped, but failed to restore the controller raw-input device");
    }
}

bool VirtualGamepad::AutoStart()
{
    return Start();
}

bool VirtualGamepad::SetBotInputEnabled(bool enabled)
{
    if (!g_running)
    {
        return false;
    }

    g_botInputActive = enabled;
    if (!enabled)
    {
        for (const VirtualAxis &axis : kVirtualAxes)
        {
            WriteInput(EV_ABS, axis.code, axis.minimum < 0 ? 0 : axis.minimum);
        }

        for (uint16_t button : kVirtualButtons)
        {
            WriteInput(EV_KEY, button, 0);
        }
    }

    return true;
}

bool VirtualGamepad::UpdateBotInput(int type, int code, int value)
{
    if (!g_running || !g_botInputActive || (type == EV_KEY && !IsVirtualButton((uint16_t) code)) ||
        (type == EV_ABS && FindVirtualAxis((uint16_t) code) == NULL))
    {
        return false;
    }

    WriteInput((uint16_t) type, (uint16_t) code, value);
    return true;
}
