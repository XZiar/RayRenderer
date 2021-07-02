#pragma once

#include "common/CommonRely.hpp"
#include "common/AlignedBase.hpp"
#include "common/simd/SIMD.hpp"
#include <cstdint>
#include <type_traits>
#include <cmath>

#if !COMMON_OVER_ALIGNED
#   error require c++17 + aligned_new to support memory management for over-aligned SIMD type.
#endif


//#pragma message("Compiling miniBLAS with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace miniBLAS
{


inline constexpr auto miniBLAS_intrin()
{
    return STRINGIZE(COMMON_SIMD_INTRIN);
}

namespace detail
{

template<uint8_t Imm>
forceinline __m128 PermutePS(const __m128& dat)
{
#if COMMON_SIMD_LV >= 100
    return _mm_permute_ps(dat, Imm);
#else
    return _mm_shuffle_ps(dat, dat, Imm);
#endif
}

}


}