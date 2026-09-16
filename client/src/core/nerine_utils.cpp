#include "nerine_utils.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>

pid_t GetPidByProcessName(const char *processName)
{
    DIR *procDirectory = opendir("/proc");
    if (!procDirectory)
    {
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(procDirectory)) != NULL)
    {
        if (!isdigit(entry->d_name[0]))
        {
            continue;
        }

        pid_t candidate = (pid_t) atoi(entry->d_name);

        char cmdlinePath[PATH_MAX];
        snprintf(cmdlinePath, sizeof(cmdlinePath), "/proc/%s/cmdline", entry->d_name);
        FILE *f = fopen(cmdlinePath, "r");
        if (!f)
        {
            continue;
        }

        char buffer[1024];
        size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, f);
        fclose(f);
        if (bytesRead == 0)
        {
            continue;
        }

        buffer[bytesRead] = '\0';

        bool cmdlineMatch = false;
        for (size_t i = 0; i < bytesRead; i += strlen(&buffer[i]) + 1)
        {
            if (strstr(&buffer[i], processName))
            {
                cmdlineMatch = true;
                break;
            }
        }

        if (!cmdlineMatch)
        {
            continue;
        }

        if (GetModuleBase(candidate, processName) != 0)
        {
            closedir(procDirectory);
            return candidate;
        }
    }

    closedir(procDirectory);
    return -1;
}

uint64_t GetModuleBase(pid_t pid, const char *moduleName)
{
    char mapsPath[PATH_MAX];
    FILE *mapsFile;
    char line[1024];
    char *pathStart;
    char *nameStart;
    uint64_t base;

    snprintf(mapsPath, sizeof(mapsPath), "/proc/%d/maps", pid);
    mapsFile = fopen(mapsPath, "r");
    if (mapsFile == NULL)
    {
        return 0;
    }

    while (fgets(line, sizeof(line), mapsFile) != NULL)
    {
        pathStart = strchr(line, '/');
        if (pathStart == NULL)
        {
            continue;
        }

        nameStart = strrchr(pathStart, '/');
        nameStart++;
        nameStart[strcspn(nameStart, "\n")] = '\0';

        if (strcmp(nameStart, moduleName) == 0)
        {
            base = strtoull(line, NULL, 16);
            fclose(mapsFile);
            return base;
        }
    }

    fclose(mapsFile);
    return 0;
}
