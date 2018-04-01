#pragma once
#include "CommonRely.hpp"

#if defined(__GNUC__)
# if __GNUC__ <= 4 && __GNUC_MINOR__ <= 5
#   error GCC Version too low to use this header, at least gcc 4.5.0
# endif
# if defined(__INTEL_COMPILER)
#   if __INTEL_COMPILER < 1500
#     error ICC Version too low to use this header, at least icc 15.0
#   endif
# endif
# include <x86intrin.h>
# define _mm256_set_m128(/* __m128 */ hi, /* __m128 */ lo)  _mm256_insertf128_ps(_mm256_castps128_ps256(lo), (hi), 0x1)
# define _mm256_set_m128d(/* __m128d */ hi, /* __m128d */ lo)  _mm256_insertf128_pd(_mm256_castpd128_pd256(lo), (hi), 0x1)
# define _mm256_set_m128i(/* __m128i */ hi, /* __m128i */ lo)  _mm256_insertf128_si256(_mm256_castsi128_si256(lo), (hi), 0x1)
#elif defined(_MSC_VER)
# if _MSC_VER < 1200
#   error MSVC Version too low to use this header, at least msvc 12.0
# endif
# include <intrin.h>
# if (_M_IX86_FP == 2) && !defined(__SSE2__)
#   define __SSE2__ 1
# endif
#else
#error Unknown compiler, not supported by this header
#endif

#if defined(__AVX2__)
# define COMMON_SIMD_INTRIN AVX2
# define COMMON_SIMD_LV 200
#elif defined(__FMA__)
# define COMMON_SIMD_INTRIN FMA
# define COMMON_SIMD_LV 150
#elif defined(__AVX__)
# define COMMON_SIMD_INTRIN AVX
# define COMMON_SIMD_LV 100
#elif defined(__SSE4_2__)
# define COMMON_SIMD_INTRIN SSE4.2
# define COMMON_SIMD_LV 42
#elif defined(__SSE4_1__)
# define COMMON_SIMD_INTRIN SSE4.1
# define COMMON_SIMD_LV 41
#elif defined(__SSSE3__)
# define COMMON_SIMD_INTRIN SSSE3
# define COMMON_SIMD_LV 31
#elif defined(__SSE3__)
# define COMMON_SIMD_INTRIN SSE3
# define COMMON_SIMD_LV 30
#elif defined(__SSE2__)
# define COMMON_SIMD_INTRIN SSE2
# define COMMON_SIMD_LV 20
#elif defined(__SSE__)
# define COMMON_SIMD_INTRIN SSE
# define COMMON_SIMD_LV 10
#else
# define COMMON_SIMD_INTRIN NONE
# define COMMON_SIMD_LV 0
#endif

