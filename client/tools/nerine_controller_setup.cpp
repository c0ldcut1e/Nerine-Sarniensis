#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <pwd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

constexpr const char *kGatePath    = "/usr/local/libexec/nerine_hidraw_gate";
constexpr const char *kRulePath    = "/etc/udev/rules.d/99-nerine_hidraw_gate.rules";
constexpr const char *kSudoersPath = "/etc/sudoers.d/nerine_hidraw_gate";

static bool IsValidUserName(const std::string &userName)
{
    if (userName.empty())
    {
        return false;
    }

    for (char character : userName)
    {
        if (!isalnum(character) && character != '_' && character != '-')
        {
            return false;
        }
    }

    return true;
}

static bool ReloadUdevRules()
{
    pid_t process = fork();
    if (process < 0)
    {
        return false;
    }

    if (process == 0)
    {
        execl("/usr/bin/udevadm", "udevadm", "control", "--reload-rules", NULL);
        _exit(1);
    }

    int status;
    return waitpid(process, &status, 0) == process && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

int main(int argumentCount, char **arguments)
{
    if (argumentCount != 3 || geteuid() != 0)
    {
        return 1;
    }

    std::filesystem::path gateSource = arguments[1];
    std::filesystem::path ruleSource = arguments[2];
    if (!std::filesystem::is_regular_file(gateSource) || !std::filesystem::is_regular_file(ruleSource))
    {
        return 1;
    }

    char *userId = getenv("PKEXEC_UID");
    if (userId == NULL)
    {
        return 1;
    }

    char *end;
    unsigned long value = strtoul(userId, &end, 10);
    if (*userId == '\0' || *end != '\0')
    {
        return 1;
    }

    passwd *user = getpwuid((uid_t) value);
    if (user == NULL || !IsValidUserName(user->pw_name))
    {
        return 1;
    }

    try
    {
        std::filesystem::create_directories("/usr/local/libexec");
        std::filesystem::copy_file(gateSource, kGatePath, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy_file(ruleSource, kRulePath, std::filesystem::copy_options::overwrite_existing);
    }
    catch (const std::filesystem::filesystem_error &)
    {
        return 1;
    }

    if (chmod(kGatePath, 0755) != 0 || chmod(kRulePath, 0644) != 0)
    {
        return 1;
    }

    std::ofstream sudoers(kSudoersPath);
    sudoers << user->pw_name << " ALL=(root) NOPASSWD: " << kGatePath << " block *, " << kGatePath << " restore *, " << kGatePath << " restore-all\n";
    sudoers.close();
    if (!sudoers || chmod(kSudoersPath, 0440) != 0)
    {
        return 1;
    }

    return ReloadUdevRules() ? 0 : 1;
}
