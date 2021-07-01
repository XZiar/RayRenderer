#pragma once
#include "SystemCommonRely.h"
#include "StringUtil/Convert.h"
#include "common/FileBase.hpp"
#include "common/MemoryStream.hpp"
#include "common/Stream.hpp"
#include "common/ContainerHelper.hpp"
#if COMMON_ARCH_X86
#   include "3rdParty/libcpuid/libcpuid/libcpuid.h"
#elif COMMON_OS_LINUX
#   include <sys/auxv.h>
#   include <asm/hwcap.h>
using cpu_id_t = std::pair<unsigned long, unsigned long>;
#else
using cpu_id_t = int;
#endif

#include <cassert>
#include <thread>
#include <variant>
#include <optional>
#include <algorithm>
#include <stdexcept>

#if COMMON_OS_WIN
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include <conio.h>
#   include <Windows.h>
#   include <ProcessThreadsApi.h>
#else
#   ifndef _GNU_SOURCE
#       define _GNU_SOURCE
#   endif
#   if COMMON_OS_ANDROID
#       define __USE_GNU 1
#   endif
#   include <cerrno>
#   include <errno.h>
#   include <fcntl.h>
#   include <pthread.h>
#   include <termios.h>
#   include <unistd.h>
#   include <sys/prctl.h>
#   include <sys/ioctl.h>
#   include <sys/mman.h>
#   include <sys/resource.h>
#   include <sys/stat.h>
#   include <sys/sysinfo.h>
#   include <sys/types.h>
#   include <sys/wait.h>
#   if COMMON_OS_MACOS
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