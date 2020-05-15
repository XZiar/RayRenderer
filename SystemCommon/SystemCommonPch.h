#pragma once
#include "SystemCommonRely.h"
#include "StringCharset/Convert.h"
#include "common/FileBase.hpp"
#include "common/MemoryStream.hpp"
#include "common/Stream.hpp"
#include "common/ContainerHelper.hpp"
#include "gsl/gsl_assert"
#include "cpuid/libcpuid.h"

#include <cassert>
#include <thread>
#include <variant>
#include <optional>
#include <algorithm>
#include <stdexcept>

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include <conio.h>
#   include <Windows.h>
#   include <ProcessThreadsApi.h>
#else
//# define _GNU_SOURCE
#   include <cerrno>
#   include <errno.h>
#   include <fcntl.h>
#   include <pthread.h>
#   include <termios.h>
#   include <unistd.h>
#   include <sys/ioctl.h>
#   include <sys/mman.h>
#   include <sys/resource.h>
#   include <sys/stat.h>
#   include <sys/sysinfo.h>
#   include <sys/types.h>
#   include <sys/wait.h>
#   if defined(__APPLE__)
#       include <sys/sysctl.h>
#       include <mach/mach_types.h>
#       include <mach/thread_act.h>
#   else
#       include <sched.h>
#       include <sys/syscall.h>
#   endif
#endif


namespace common
{

#if COMMON_OS_WIN
[[nodiscard]] uint32_t GetWinBuildNumber() noexcept;
#endif

[[nodiscard]] const std::optional<cpu_id_t>& GetCPUInfo() noexcept;
// follow initializer may not work
// uint32_t RegisterInitializer(void(*func)() noexcept) noexcept;

}