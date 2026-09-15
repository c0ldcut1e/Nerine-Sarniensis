#ifndef NERINE_UTILS_HPP
#define NERINE_UTILS_HPP

#include <cstdint>
#include <sys/types.h>

pid_t GetPidByProcessName(const char *processName);
uint64_t GetModuleBase(pid_t pid, const char *moduleName);

#endif
