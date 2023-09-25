#pragma once

#include "SystemCommonRely.h"
#include "RuntimeFastPath.h"

using namespace std::string_view_literals;
using common::CheckCPUFeature;


namespace
{
using common::fastpath::FuncVarBase;

struct NAIVE : FuncVarBase {};
struct COMPILER : FuncVarBase {};
struct OS : FuncVarBase {};
struct LOOP : FuncVarBase {};
struct SIMD128
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("sse2"sv);
#else
        return CheckCPUFeature("asimd"sv);
#endif
    }
};
struct SSE2
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("sse2"sv);
#else
        return false;
#endif
    }
};
struct SIMDSSSE3
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("ssse3"sv);
#else
        return false;
#endif
    }
};
struct SIMDSSE41
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("sse4_1"sv);
#else
        return false;
#endif
    }
};
struct SIMD256
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("avx"sv);
#else
        return false;
#endif
    }
};
struct SIMDAVX2
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("avx2"sv);
#else
        return false;
#endif
    }
};
struct AVX512F
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("avx512f"sv);
#else
        return false;
#endif
    }
};
struct F16C
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("f16c"sv);
#else
        return false;
#endif
    }
};
struct LZCNT
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("bmi1"sv) || CheckCPUFeature("lzcnt"sv);
#else
        return false;
#endif
    }
};
struct TZCNT
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("bmi1"sv);
#else
        return false;
#endif
    }
};
struct POPCNT
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("popcnt"sv);
#else
        return false;
#endif
    }
};
struct BMI1
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("bmi1"sv);
#else
        return false;
#endif
    }
};
struct SHANI
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("sha"sv);
#else
        return false;
#endif
    }
};
struct SHANIAVX2
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("sha"sv) && CheckCPUFeature("avx2"sv);
#else
        return false;
#endif
    }
};
struct SHA2
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return false;
#else
        return CheckCPUFeature("sha2"sv);
#endif
    }
};
struct WAITPKG
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("waitpkg"sv);
#else
        return false;
#endif
    }
};
}