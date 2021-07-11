#pragma once
#include "SystemCommonRely.h"
#include "StringUtil/Convert.h"
#include "common/FileBase.hpp"
#include "common/MemoryStream.hpp"
#include "common/Stream.hpp"
#include "common/ContainerHelper.hpp"
#include "common/StrParsePack.hpp"
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>

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

// follow initializer may not work
// uint32_t RegisterInitializer(void(*func)() noexcept) noexcept;


namespace fastpath
{
struct FuncVarBase
{
    static bool RuntimeCheck() noexcept { return true; }
};
}

template<typename Func, typename Method>
constexpr bool CheckExists() noexcept
{
    return false;
}
template<typename Func, typename Method, typename F>
constexpr void InjectFunc(F&) noexcept
{
    static_assert(AlwaysTrue<F>, "Should not pass CheckExists");
}


#define DEFINE_FASTPATH_METHOD(func, var, ...)                                                  \
template<>                                                                                      \
constexpr bool CheckExists<fastpath::func, fastpath::var>() noexcept                            \
{ return true; }                                                                                \
static typename fastpath::func::RetType func##_##var(__VA_ARGS__) noexcept;                     \
template<>                                                                                      \
constexpr void InjectFunc<fastpath::func, fastpath::var>(decltype(&func##_##var)& f) noexcept   \
{ f = func##_##var; }                                                                           \
static typename fastpath::func::RetType func##_##var(__VA_ARGS__) noexcept                      \

#define CALL_FASTPATH_METHOD(func, var, ...) func##_##var(__VA_ARGS__)

#define RegistFuncVar(r, func, var)                         \
if constexpr (CheckExists<fastpath::func, fastpath::var>()) \
{                                                           \
    if (fastpath::var::RuntimeCheck())                      \
        ret.emplace_back(STRINGIZE(func), STRINGIZE(var));  \
}                                                           \

#define RegistFuncVars(func, ...) do                                                \
{                                                                                   \
BOOST_PP_SEQ_FOR_EACH(RegistFuncVar, func, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))   \
} while(0)                                                                          \

#define SWITCH_VAR_CASE(v, func, var)                               \
HashCase(v, STRINGIZE(var))                                         \
if constexpr (CheckExists<fastpath::func, fastpath::var>())         \
{                                                                   \
    if (func == nullptr)                                            \
    {                                                               \
        InjectFunc<fastpath::func, fastpath::var>(func);            \
        VariantMap.emplace_back(STRINGIZE(func), STRINGIZE(var));   \
    }                                                               \
}                                                                   \
break;                                                              \

#define SWITCH_VAR_CASE_(r, vf, var) SWITCH_VAR_CASE(BOOST_PP_TUPLE_ELEM(0, vf), BOOST_PP_TUPLE_ELEM(1, vf), var)
#define SWITCH_VAR_CASES(vf, vars) BOOST_PP_SEQ_FOR_EACH(SWITCH_VAR_CASE_, vf, vars)
#define CHECK_FUNC_VARS(f, v, func, ...)                            \
HashCase(f, STRINGIZE(func))                                        \
switch (DJBHash::HashC(v))                                          \
{                                                                   \
SWITCH_VAR_CASES((v, func), BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))  \
} break                                                             \



}