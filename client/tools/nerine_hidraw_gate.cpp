#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

constexpr const char *kMarkerDirectory = "/run/nerine_hidraw_gate";

static bool IsValidIdentity(const std::string &identity)
{
    if (identity.empty() || identity.length() > 256 || identity.length() % 2 != 0)
    {
        return false;
    }

    for (char character : identity)
    {
        if ((character < '0' || character > '9') && (character < 'a' || character > 'f'))
        {
            return false;
        }
    }

    return true;
}

static std::string EncodeIdentity(const std::string &identity)
{
    constexpr char hexDigits[] = "0123456789abcdef";
    std::string encodedIdentity;
    for (char character : identity)
    {
        encodedIdentity += hexDigits[((uint8_t) character) >> 4];
        encodedIdentity += hexDigits[((uint8_t) character) & 0xF];
    }

    return encodedIdentity;
}

static std::string ReadFile(const std::filesystem::path &path)
{
    std::ifstream file(path);
    std::string value;
    getline(file, value);
    value.erase(std::remove_if(value.begin(), value.end(),
                               [](char character)
                               {
                                   return isspace(character);
                               }),
                value.end());
    return value;
}

static std::filesystem::path FindHidDevice(std::filesystem::path path)
{
    path = std::filesystem::canonical(path);
    while (path != "/")
    {
        if (std::filesystem::exists(path / "driver") && std::filesystem::exists(path / "uevent") && std::filesystem::exists(path / "hidraw"))
        {
            return path;
        }

        path = path.parent_path();
    }

    return "";
}

static std::string GetIdentity(const std::filesystem::path &path)
{
    std::filesystem::path hidDevice = FindHidDevice(path);
    if (hidDevice.empty())
    {
        return "";
    }

    std::string identity = ReadFile(hidDevice / "uniq");
    if (!identity.empty())
    {
        return EncodeIdentity(identity);
    }

    return EncodeIdentity(ReadFile(hidDevice / "phys"));
}

static std::filesystem::path FindHidrawDevice(const std::string &identity)
{
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator("/sys/class/hidraw"))
    {
        if (GetIdentity(entry.path()) == identity)
        {
            return entry.path();
        }
    }

    return "";
}

static bool Rebind(const std::filesystem::path &hidrawDevice)
{
    std::filesystem::path hidDevice = FindHidDevice(hidrawDevice);
    if (hidDevice.empty())
    {
        return false;
    }

    std::filesystem::path driver = std::filesystem::canonical(hidDevice / "driver");
    std::ofstream unbind(driver / "unbind");
    unbind << hidDevice.filename().string();
    if (!unbind)
    {
        return false;
    }

    std::ofstream bind(driver / "bind");
    bind << hidDevice.filename().string();
    return (bool) bind;
}

static bool IsBlocked(const std::string &identity)
{
    return std::filesystem::exists(std::filesystem::path(kMarkerDirectory) / identity);
}

static bool SetBlocked(const std::string &identity, bool blocked)
{
    std::filesystem::path marker = std::filesystem::path(kMarkerDirectory) / identity;
    if (blocked)
    {
        std::filesystem::create_directories(kMarkerDirectory);
        std::ofstream file(marker);
        if (!file)
        {
            return false;
        }
    }
    else
    {
        std::filesystem::remove(marker);
    }

    std::filesystem::path hidrawDevice = FindHidrawDevice(identity);
    if (hidrawDevice.empty())
    {
        return true;
    }

    if (Rebind(hidrawDevice))
    {
        return true;
    }

    if (blocked)
    {
        std::filesystem::remove(marker);
    }

    return false;
}

int main(int argumentCount, char **arguments)
{
    if (argumentCount < 2)
    {
        return 1;
    }

    std::string command = arguments[1];
    try
    {
        if (command == "verify" && argumentCount == 2)
        {
            return geteuid() == 0 && std::filesystem::is_regular_file("/etc/udev/rules.d/99-nerine_hidraw_gate.rules") ? 0 : 1;
        }

        if (command == "restore-all" && argumentCount == 2)
        {
            if (!std::filesystem::exists(kMarkerDirectory))
            {
                return 0;
            }

            bool restored = true;
            for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(kMarkerDirectory))
            {
                std::string identity = entry.path().filename().string();
                if (IsValidIdentity(identity) && !SetBlocked(identity, false))
                {
                    restored = false;
                }
            }

            return restored ? 0 : 1;
        }

        if (argumentCount != 3)
        {
            return 1;
        }

        std::string argument = arguments[2];
        if (command == "udev-check")
        {
            return IsBlocked(GetIdentity(std::filesystem::path("/sys" + argument))) ? 0 : 1;
        }

        if (!IsValidIdentity(argument))
        {
            return 1;
        }

        if (command == "block")
        {
            return SetBlocked(argument, true) ? 0 : 1;
        }

        if (command == "restore")
        {
            return SetBlocked(argument, false) ? 0 : 1;
        }
    }
    catch (const std::filesystem::filesystem_error &)
    {
        return 1;
    }

    return 1;
}
