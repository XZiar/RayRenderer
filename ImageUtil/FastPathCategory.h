#pragma once

#include "SystemCommon/SystemCommonRely.h"
#include "SystemCommon/RuntimeFastPath.h"

using ::common::CheckCPUFeature;


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
struct BMI2
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("bmi2");
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
struct AVX22
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
struct AVX512BW
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("avx512f") && CheckCPUFeature("avx512bw");
#else
        return false;
#endif
    }
};
struct AVX512BW2
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("avx512f") && CheckCPUFeature("avx512bw");
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
        return CheckCPUFeature("avx512f") && CheckCPUFeature("avx512bw") && CheckCPUFeature("avx512vbmi2");
#else
        return false;
#endif
    }
};

