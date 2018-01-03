#pragma once

#include "common/CommonMacro.hpp"
#include "common/CommonRely.hpp"
#include <cstdint>
#include <type_traits>
#include <cmath>


#if defined(__GNUC__)
#   include <x86intrin.h>
#   define _mm256_set_m128(/* __m128 */ hi, /* __m128 */ lo)  _mm256_insertf128_ps(_mm256_castps128_ps256(lo), (hi), 0x1)
#   define _mm256_set_m128d(/* __m128d */ hi, /* __m128d */ lo)  _mm256_insertf128_pd(_mm256_castpd128_pd256(lo), (hi), 0x1)
#   define _mm256_set_m128i(/* __m128i */ hi, /* __m128i */ lo)  _mm256_insertf128_si256(_mm256_castsi128_si256(lo), (hi), 0x1)
#else
#   include <intrin.h>
#   if (_M_IX86_FP == 2)
#      define __SSE2__ 1
#   endif
#endif

#ifdef __AVX2__
#   define __AVX__ 1
#endif

#ifdef __AVX__
#   define __SSE4_2__ 1
#endif

#ifdef __SSE4_2__
#   define __SSE2__ 1
#endif

#if defined(_WIN32) && defined(__SSE2__) && !defined(_MANAGED) && !defined(_M_CEE) && !defined(__cplusplus_cli) && !defined(BAK_M_CEE)
#   define VECCALL __vectorcall
#else
#   define VECCALL 
#endif

#ifdef __AVX2__
#   define MINIBLAS_INTRIN AVX2
#elif defined(__AVX__)
#   define MINIBLAS_INTRIN AVX
#elif defined(__SSE4_2__)
#   define MINIBLAS_INTRIN SSE4.2
#elif defined(__SSE2__)
#   define MINIBLAS_INTRIN SSE2
#else
#   define MINIBLAS_INTRIN NONE
#endif
#pragma message("Compiling miniBLAS with " STRINGIZE(MINIBLAS_INTRIN) )

namespace miniBLAS
{


inline constexpr auto miniBLAS_intrin()
{
	return STRINGIZE(MINIBLAS_INTRIN);
}


}