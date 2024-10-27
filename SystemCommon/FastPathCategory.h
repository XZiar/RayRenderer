#pragma once

#include "SystemCommonRely.h"
#include "RuntimeFastPath.h"

using ::common::CheckCPUFeature;


struct NAIVE : ::common::fastpath::FuncVarBase {};
struct COMPILER : ::common::fastpath::FuncVarBase {};
struct OS : ::common::fastpath::FuncVarBase {};
struct LOOP : ::common::fastpath::FuncVarBase {};
struct SIMD128
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("sse2");
#else
        return CheckCPUFeature("asimd");
#endif
    }
};
struct SSE2
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("sse2");
#else
        return false;
#endif
    }
};
struct SSSE3
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("ssse3");
#else
        return false;
#endif
    }
};
struct SSE41
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("sse4_1");
#else
        return false;
#endif
    }
};
struct AVX
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("avx");
#else
        return false;
#endif
    }
};
struct AVX2
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("avx2");
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
        return CheckCPUFeature("avx512f");
#else
        return false;
#endif
    }
};
struct AVX512BW
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return AVX512F::RuntimeCheck() && CheckCPUFeature("avx512bw") && CheckCPUFeature("avx512vl");
#else
        return false;
#endif
    }
};
struct AVX512VBMI
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return AVX512BW::RuntimeCheck() && CheckCPUFeature("avx512vbmi");
#else
        return false;
#endif
    }
};
struct AVX512VBMI2
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return AVX512BW::RuntimeCheck() && CheckCPUFeature("avx512vbmi2");
#else
        return false;
#endif
    }
};
struct AVX512VPOPCNT
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return AVX512BW::RuntimeCheck() && CheckCPUFeature("avx512vpopcntdq");
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
        return CheckCPUFeature("f16c");
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
        return CheckCPUFeature("bmi1") || CheckCPUFeature("lzcnt");
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
        return CheckCPUFeature("bmi1");
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
        return CheckCPUFeature("popcnt");
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
        return CheckCPUFeature("bmi1");
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
        return CheckCPUFeature("sha");
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
        return SHANI::RuntimeCheck() && AVX2::RuntimeCheck() && CheckCPUFeature("movbe");
#else
        return false;
#endif
    }
};
struct SHANIAVX512BW
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return SHANI::RuntimeCheck() && AVX512BW::RuntimeCheck();
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
        return CheckCPUFeature("sha2");
#endif
    }
};
struct WAITPKG
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("waitpkg");
#else
        return false;
#endif
    }
};
