#pragma once

#include "StrBase.hpp"
#include "SIMD.hpp"

namespace common::str
{

template<typename T, size_t N>
forceinline bool ShortCompare(const std::basic_string_view<T> src, const T(&target)[N]) noexcept
{
    constexpr size_t len = N - 1;
    if (src.size() != len)
        return false;
    [[maybe_unused]] constexpr size_t targetSBytes = len * sizeof(T);
    if constexpr (targetSBytes <= (8 * 8u) && len <= 4)
    {
        for (size_t i = 0; i < len; ++i)
            if (src[i] != target[i])
                return false;
        return true;
    }
    else if constexpr (targetSBytes <= 128u / 8u)
    {
#if COMMON_SIMD_LV >= 20
        const auto mask = (1u << targetSBytes) - 1u;
        const auto vecT = _mm_loadu_si128(reinterpret_cast<const __m128i*>(target));
        const auto vecS = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src.data()));
        const auto cmpbits = (uint32_t)_mm_movemask_epi8(_mm_cmpeq_epi8(vecS, vecT));
        return (cmpbits & mask) == mask;
#else
        return src == target;
#endif
    }
    else if constexpr (targetSBytes <= 256u / 8u)
    {
#if COMMON_SIMD_LV >= 200
        const auto mask = static_cast<uint32_t>((uint64_t(1) << targetSBytes) - 1u);
        const auto vecT = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(target));
        const auto vecS = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src.data()));
        const auto cmpbits = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(vecS, vecT)));
        return (cmpbits & mask) == mask;
#elif COMMON_SIMD_LV >= 20
        const auto mask = static_cast<uint32_t>((uint64_t(1) << targetSBytes) - 1u);
        const auto vecT0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(target) + 0);
        const auto vecT1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(target) + 1);
        const auto vecS0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src.data()) + 0);
        const auto vecS1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src.data()) + 1);
        const auto cmpbits0 = static_cast<uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(vecS0, vecT0)));
        const auto cmpbits1 = static_cast<uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(vecS1, vecT1)));
        return cmpbits0 == 0xffffffffu && (cmpbits1 & mask) == mask;
#else
        return src == target;
#endif
    }
    else
        return src == target;
}

#define HashCase(target, cstr) case hash_(cstr): if (!::common::str::ShortCompare(target, cstr)) break;


}