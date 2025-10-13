#pragma once
#include "SIMD.hpp"
#include "SIMDVec.hpp"
#include <type_traits>

#if COMMON_SIMD_LV < 300
#   error require at least AVX512
#endif
#if !COMMON_OVER_ALIGNED
#   error require c++17 + aligned_new to support memory management for over-aligned SIMD type.
#endif


namespace COMMON_SIMD_NAMESPACE
{
#define mm512_set_lane(n, ...) _mm512_set_##n(__VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__)

template<typename Op>
inline constexpr uint8_t GetTernaryLogicOp(const Op& op) noexcept
{
    static_assert(std::is_invocable_r_v<bool, Op, bool, bool, bool>);
    uint32_t ret = 0;
    for (uint8_t i = 0; i < 8; ++i)
    {
        const bool v0 = i & 0b100, v1 = i & 0b10, v2 = i & 0b1;
        const bool ans = op(v0, v1, v2);
        if (ans) ret += (1u << i);
    }
    return static_cast<uint8_t>(ret);
}

forceinline __m512i MaskedBlendBit(const __m512i& a, const __m512i& b, const __m512i& mask) noexcept
{
    constexpr auto opcode = GetTernaryLogicOp([](bool a, bool b, bool c) { return c ? b : a; });
    // 000=0 001=0 010=0 011=1 100=1 101=0 110=1 111=1
    static_assert(opcode == 0b11011000);
    return _mm512_ternarylogic_epi32(a, b, mask, opcode);
}


}
