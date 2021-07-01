#pragma once
#include "CommonRely.hpp"

#if COMMON_ARCH_X86
# if COMMON_COMPILER_CLANG
#   if COMMON_CLANG_VER < 30000
#     error clang version too low to use this header, at least clang 3.0.0
#   endif
#   include <x86intrin.h>
# elif COMMON_COMPILER_GCC
#   if COMMON_GCC_VER < 40500
#     error GCC version too low to use this header, at least gcc 4.5.0
#   endif
#   if defined(__INTEL_COMPILER)
#     if __INTEL_COMPILER < 1500
#       error ICC version too low to use this header, at least icc 15.0
#     endif
#   endif
#   include <x86intrin.h>
#   define _mm256_set_m128(/* __m128 */ hi, /* __m128 */ lo)  _mm256_insertf128_ps(_mm256_castps128_ps256(lo), (hi), 0x1)
#   define _mm256_set_m128d(/* __m128d */ hi, /* __m128d */ lo)  _mm256_insertf128_pd(_mm256_castpd128_pd256(lo), (hi), 0x1)
#   define _mm256_set_m128i(/* __m128i */ hi, /* __m128i */ lo)  _mm256_insertf128_si256(_mm256_castsi128_si256(lo), (hi), 0x1)
#   define _mm256_loadu2_m128(/* __m128i const* */ hi, /* __m128i const* */ lo) _mm256_set_m128(_mm_loadu_ps(hi), _mm_loadu_ps(lo))
#   define _mm256_loadu2_m128d(/* __m128i const* */ hi, /* __m128i const* */ lo) _mm256_set_m128d(_mm_loadu_pd(hi), _mm_loadu_pd(lo))
#   define _mm256_loadu2_m128i(/* __m128i const* */ hi, /* __m128i const* */ lo) _mm256_set_m128i(_mm_loadu_si128(hi), _mm_loadu_si128(lo))
# elif COMMON_COMPILER_MSVC
#   if COMMON_MSVC_VER < 120000
#     error MSVC version too low to use this header, at least msvc 12.0 (VS6.0)
#   endif
#   include <intrin.h>
#   if (_M_IX86_FP == 1) && !defined(__SSE__)
#     define __SSE__ 1
#   elif (_M_IX86_FP == 2) && !defined(__SSE2__)
#     define __SSE2__ 1
#   endif
# else
#  error Unknown compiler, not supported by this header
# endif
#elif COMMON_ARCH_ARM
# if COMMON_COMPILER_CLANG
#   if COMMON_CLANG_VER < 30000
#     error clang version too low to use this header, at least clang 3.0.0
#   endif
#   include <arm_neon.h>
# elif COMMON_COMPILER_GCC
#   if COMMON_GCC_VER < 40300
#     error GCC version too low to use this header, at least gcc 4.3.0
#   endif
#   include <arm_neon.h>
# elif COMMON_COMPILER_MSVC
#   if COMMON_MSVC_VER < 191600
#     error MSVC version too low to use this header, at least msvc 19.16 (VS15.9.0)
#   endif
#   include <arm_neon.h>
# else
#  error Unknown compiler, not supported by this header
# endif
# if !defined(__ARM_NEON) && (defined(_M_ARM64) || defined(__aarch64__))
// The ARMv8-A architecture has made many ARMv7-A optional features mandatory, including advanced SIMD (also called NEON).
#   define __ARM_NEON 1
# endif
#else
#  error Unknown architecture, not supported by this header
#endif

#if COMMON_ARCH_X86

# if defined(__AVX2__)
#    define COMMON_SIMD_LV_ 200
# elif defined(__FMA__)
#    define COMMON_SIMD_LV_ 150
# elif defined(__AVX__)
#    define COMMON_SIMD_LV_ 100
# elif defined(__SSE4_2__)
#    define COMMON_SIMD_LV_ 42
# elif defined(__SSE4_1__)
#    define COMMON_SIMD_LV_ 41
# elif defined(__SSSE3__)
#    define COMMON_SIMD_LV_ 31
# elif defined(__SSE3__)
#    define COMMON_SIMD_LV_ 30
# elif defined(__SSE2__)
#    define COMMON_SIMD_LV_ 20
# elif defined(__SSE__)
#    define COMMON_SIMD_LV_ 10
# else
#    define COMMON_SIMD_LV_ 0
# endif
# ifndef COMMON_SIMD_LV
#    define COMMON_SIMD_LV COMMON_SIMD_LV_
# endif

# if COMMON_SIMD_LV >= 200
#   define COMMON_SIMD_INTRIN AVX2
# elif COMMON_SIMD_LV >= 150
#   define COMMON_SIMD_INTRIN FMA
# elif COMMON_SIMD_LV >= 100
#   define COMMON_SIMD_INTRIN AVX
# elif COMMON_SIMD_LV >= 42
#   define COMMON_SIMD_INTRIN SSE4_2
# elif COMMON_SIMD_LV >= 41
#   define COMMON_SIMD_INTRIN SSE4_1
# elif COMMON_SIMD_LV >= 31
#   define COMMON_SIMD_INTRIN SSSE3
# elif COMMON_SIMD_LV >= 30
#   define COMMON_SIMD_INTRIN SSE3
# elif COMMON_SIMD_LV >= 20
#   define COMMON_SIMD_INTRIN SSE2
# elif COMMON_SIMD_LV >= 10
#   define COMMON_SIMD_INTRIN SSE
# else
#   define COMMON_SIMD_INTRIN NONE
# endif

#endif


#if COMMON_ARCH_ARM

# if defined(__ARM_NEON)
#    define COMMON_SIMD_LV_ 200
# elif defined(__ARM_NEON)
#    define COMMON_SIMD_LV_ 100
# else
#    define COMMON_SIMD_LV_ 0
# endif
# ifndef COMMON_SIMD_LV
#    define COMMON_SIMD_LV COMMON_SIMD_LV_
# endif

# if COMMON_SIMD_LV >= 200
#   define COMMON_SIMD_INTRIN NEONv2
# elif COMMON_SIMD_LV >= 100
#   define COMMON_SIMD_INTRIN NEON
# else
#   define COMMON_SIMD_INTRIN NONE
# endif

#endif


namespace common::simd
{
inline constexpr auto GetSIMDIntrin() noexcept
{
    return STRINGIZE(COMMON_SIMD_INTRIN);
}

struct VecDataInfo
{
    enum class DataTypes : uint8_t { Empty = 0, Custom, Unsigned, Signed, Float, BFloat };
    DataTypes Type;
    uint8_t Bit;
    uint8_t Dim0;
    uint8_t Dim1;

    constexpr operator uint32_t() const noexcept
    {
        return (static_cast<uint32_t>(Type) << 0)  | (static_cast<uint32_t>(Bit)  << 8) 
            |  (static_cast<uint32_t>(Dim0) << 16) | (static_cast<uint32_t>(Dim1) << 24);
    }
    constexpr VecDataInfo Scalar(uint8_t dim = 0) const noexcept
    {
        return { Type, Bit, dim, 0 };
    }

    template<typename T, uint8_t N>
    static constexpr VecDataInfo GetVectorInfo() noexcept
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            const auto bit = gsl::narrow_cast<uint8_t>(sizeof(T));
            return { DataTypes::Float, bit, N, (uint8_t)0 };
        }
        else if constexpr (std::is_integral_v<T>)
        {
            const auto type = std::is_unsigned_v<T> ? DataTypes::Unsigned : DataTypes::Signed;
            const auto bit = gsl::narrow_cast<uint8_t>(sizeof(T));
            return { type, bit, N, (uint8_t)0 };
        }
        else
        {
            static_assert(!AlwaysTrue<T>, "only numeraic type allowed");
        }
    }
};
}