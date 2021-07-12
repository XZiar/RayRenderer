#include "SystemCommonPch.h"
#include "MiscIntrins.h"
#include "common/simd/SIMD.hpp"
#include "3rdParty/digestpp/algorithm/sha2.hpp"

namespace common
{
using namespace std::string_view_literals;
namespace fastpath
{
DEFINE_FASTPATH(MiscIntrins, LeadZero32);
DEFINE_FASTPATH(MiscIntrins, LeadZero64);
DEFINE_FASTPATH(MiscIntrins, TailZero32);
DEFINE_FASTPATH(MiscIntrins, TailZero64);
DEFINE_FASTPATH(MiscIntrins, PopCount32);
DEFINE_FASTPATH(MiscIntrins, PopCount64);
#define LeadZero32Args BOOST_PP_VARIADIC_TO_SEQ(num)
#define LeadZero64Args BOOST_PP_VARIADIC_TO_SEQ(num)
#define TailZero32Args BOOST_PP_VARIADIC_TO_SEQ(num)
#define TailZero64Args BOOST_PP_VARIADIC_TO_SEQ(num)
#define PopCount32Args BOOST_PP_VARIADIC_TO_SEQ(num)
#define PopCount64Args BOOST_PP_VARIADIC_TO_SEQ(num)
DEFINE_FASTPATH(DigestFuncs, Sha256);
#define Sha256Args BOOST_PP_VARIADIC_TO_SEQ(data, size)


struct NAIVE : FuncVarBase {};
struct COMPILER : FuncVarBase {};
struct OS : FuncVarBase {};
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
}


#if COMMON_COMPILER_MSVC

DEFINE_FASTPATH_METHOD(LeadZero32, COMPILER)
{
    unsigned long idx = 0;
    return _BitScanReverse(&idx, num) ? 31 - idx : 32;
}
DEFINE_FASTPATH_METHOD(LeadZero64, COMPILER)
{
    unsigned long idx = 0;
#   if COMMON_OSBIT == 64
    return _BitScanReverse64(&idx, num) ? 63 - idx : 64;
#   else
    const auto lo = static_cast<uint32_t>(num), hi = static_cast<uint32_t>(num >> 32);
    if (!_BitScanReverse(&idx, hi))
        return _BitScanReverse(&idx, lo) ? 63 - idx : 64;
    else
        return 31 - idx;
#   endif
}

DEFINE_FASTPATH_METHOD(TailZero32, COMPILER)
{
    unsigned long idx = 0;
    return _BitScanForward(&idx, num) ? idx : 32;
}
DEFINE_FASTPATH_METHOD(TailZero64, COMPILER)
{
    unsigned long idx = 0;
#   if COMMON_OSBIT == 64
    return _BitScanForward64(&idx, num) ? idx : 64;
#   else
    const auto lo = static_cast<uint32_t>(num), hi = static_cast<uint32_t>(num >> 32);
    if (!_BitScanForward(&idx, lo))
        return _BitScanForward(&idx, hi) ? idx + 32 : 64;
    else
        return idx;
#endif
}

#elif COMMON_COMPILER_GCC || COMMON_COMPILER_CLANG

DEFINE_FASTPATH_METHOD(LeadZero32, COMPILER)
{
    return num == 0 ? 32 : __builtin_clz(num);
}
DEFINE_FASTPATH_METHOD(LeadZero64, COMPILER)
{
    return num == 0 ? 64 : __builtin_clzll(num);
}

DEFINE_FASTPATH_METHOD(TailZero32, COMPILER)
{
    return num == 0 ? 32 : __builtin_ctz(num);
}
DEFINE_FASTPATH_METHOD(TailZero64, COMPILER)
{
    return num == 0 ? 64 : __builtin_ctzll(num);
}

DEFINE_FASTPATH_METHOD(PopCount32, COMPILER)
{
    return __builtin_popcount(num);
}
DEFINE_FASTPATH_METHOD(PopCount64, COMPILER)
{
    return __builtin_popcountll(num);
}

#endif


//DEFINE_FASTPATH_METHOD(LeadZero32, NAIVE)
//{
//}
//DEFINE_FASTPATH_METHOD(LeadZero64, NAIVE)
//{
//}

//DEFINE_FASTPATH_METHOD(TailZero32, NAIVE)
//{
//}
//DEFINE_FASTPATH_METHOD(TailZero64, NAIVE)
//{
//}

DEFINE_FASTPATH_METHOD(PopCount32, NAIVE)
{
    auto tmp = num - ((num >> 1) & 0x55555555u);
    tmp = (tmp & 0x33333333u) + ((tmp >> 2) & 0x33333333u);
    tmp = (tmp + (tmp >> 4)) & 0x0f0f0f0fu;
    return (tmp * 0x01010101u) >> 24;
}
DEFINE_FASTPATH_METHOD(PopCount64, NAIVE)
{
    auto tmp = num - ((num >> 1) & 0x5555555555555555u);
    tmp = (tmp & 0x3333333333333333u) + ((tmp >> 2) & 0x3333333333333333u);
    tmp = (tmp + (tmp >> 4)) & 0x0f0f0f0f0f0f0f0fu;
    return (tmp * 0x0101010101010101u) >> 56;
}

DEFINE_FASTPATH_METHOD(Sha256, NAIVE)
{
    std::array<std::byte, 32> output;
    digestpp::sha256().absorb(data, size)
        .digest(reinterpret_cast<unsigned char*>(output.data()), 32);
    return output;
}


#if (COMMON_COMPILER_MSVC && COMMON_ARCH_X86/* && COMMON_SIMD_LV >= 200*/) || (!COMMON_COMPILER_MSVC && (defined(__LZCNT__) || defined(__BMI__)))
#pragma message("Compiling MiscIntrins with LZCNT")

DEFINE_FASTPATH_METHOD(LeadZero32, LZCNT)
{
    return _lzcnt_u32(num);
}
DEFINE_FASTPATH_METHOD(LeadZero64, LZCNT)
{
#   if COMMON_OSBIT == 64
    return static_cast<uint32_t>(_lzcnt_u64(num));
#   else
    const auto lo = static_cast<uint32_t>(num), hi = static_cast<uint32_t>(num >> 32);
    const auto hiCnt = _lzcnt_u32(hi);
    return hiCnt == 32 ? _lzcnt_u32(lo) + 32 : hiCnt;
#   endif
}

#endif


#if (COMMON_COMPILER_MSVC && COMMON_ARCH_X86/* && COMMON_SIMD_LV >= 200*/) || (!COMMON_COMPILER_MSVC && defined(__BMI__))
#pragma message("Compiling MiscIntrins with TZCNT")

DEFINE_FASTPATH_METHOD(TailZero32, TZCNT)
{
    return _tzcnt_u32(num);
}
DEFINE_FASTPATH_METHOD(TailZero64, TZCNT)
{
#   if COMMON_OSBIT == 64
    return static_cast<uint32_t>(_tzcnt_u64(num));
#   else
    const auto lo = static_cast<uint32_t>(num), hi = static_cast<uint32_t>(num >> 32);
    const auto loCnt = _tzcnt_u32(lo);
    return loCnt == 32 ? _tzcnt_u32(hi) + 32 : loCnt;
#   endif
}

#endif


#if (COMMON_COMPILER_MSVC && COMMON_ARCH_X86/* && COMMON_SIMD_LV >= 42*/) || (!COMMON_COMPILER_MSVC && defined(__POPCNT__))
#pragma message("Compiling MiscIntrins with POPCNT")

DEFINE_FASTPATH_METHOD(PopCount32, POPCNT)
{
    return _mm_popcnt_u32(num);
}
DEFINE_FASTPATH_METHOD(PopCount64, POPCNT)
{
#   if COMMON_OSBIT == 64
    return static_cast<uint32_t>(_mm_popcnt_u64(num));
#   else
    const auto lo = static_cast<uint32_t>(num), hi = static_cast<uint32_t>(num >> 32);
    return _mm_popcnt_u32(lo) + _mm_popcnt_u32(hi);
#   endif
}

#endif


#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 41

forceinline __m128i Load128With80BE(const __m128i* data, const size_t len) noexcept
{
    // Expects len < 16
    if (len == 0)
        return _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<char>(0x80), 0, 0, 0);
    auto val = _mm_loadu_si128(data);
    const __m128i IdxConst  = _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);
    const __m128i SignConst = _mm_set1_epi8(static_cast<char>(0x80));   // 80,80,80,80
    const __m128i SizeMask  = _mm_set1_epi8(static_cast<char>(len));    //  x, x, x, x
    const __m128i EndMask   = _mm_cmpeq_epi8(IdxConst, SizeMask);       // 00,00,ff,00
    const __m128i SuffixBit = _mm_and_si128(SignConst, EndMask);        // 00,00,80,00
    const __m128i Size1Mask = _mm_sub_epi8(SizeMask, _mm_set1_epi8(1)); //  y, y, y, y
    const __m128i KeepMask  = _mm_cmpgt_epi8(IdxConst, Size1Mask);      // 00,00,ff,ff
    const __m128i KeepBit   = _mm_and_si128(SignConst, KeepMask);       // 00,00,80,80
    const __m128i ShufMask  = _mm_or_si128(KeepBit, IdxConst);          //  4, 5,86,87
    val = _mm_shuffle_epi8(val, ShufMask);                              // z4,z5, 0, 0
    val = _mm_or_si128(val, SuffixBit);                                 // z4,z5,80, 0
    return val;
}

template<typename T>
inline std::array<std::byte, 32> Sha256SSE(const std::byte* data, const size_t size, T&& calcBlock) noexcept
{
    /* Load initial values */
    __m128i state0 = _mm_set_epi32(0xa54ff53a, 0x3c6ef372, 0xbb67ae85, 0x6a09e667);
    __m128i state1 = _mm_set_epi32(0x5be0cd19, 0x1f83d9ab, 0x9b05688c, 0x510e527f);

    {
        __m128i tmp = _mm_shuffle_epi32(state0, 0xB1);  /* CDAB */
        state1 = _mm_shuffle_epi32(state1, 0x1B);       /* EFGH */
        state0 = _mm_alignr_epi8(tmp, state1, 8);       /* ABEF */
        state1 = _mm_blend_epi16(state1, tmp, 0xF0);    /* CDGH */
    }

    const __m128i mask = _mm_set_epi64x(0x0c0d0e0f08090a0bULL, 0x0405060700010203ULL);

    size_t len = size;
    const __m128i* __restrict ptr = reinterpret_cast<const __m128i*>(data);
    while (len >= 64)
    {
        const auto msg0 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
        const auto msg1 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
        const auto msg2 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
        const auto msg3 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
        calcBlock(state0, state1, msg0, msg1, msg2, msg3);
        len -= 64;
    }
    if (len >= 56)
    {
        const auto msg0 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
        const auto msg1 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
        const auto msg2 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
        const auto msg3 = Load128With80BE(ptr++, len % 16);
        calcBlock(state0, state1, msg0, msg1, msg2, msg3);
        len = SIZE_MAX;
    }
    {
        __m128i msg0, msg1, msg2, msg3;
        const auto bits = static_cast<uint64_t>(size) * 8;
        const auto bitsv = _mm_set1_epi64x(static_cast<int64_t>(bits));
        const auto bitsvBE = _mm_shuffle_epi8(bitsv, _mm_set_epi8(3, 2, 1, 0, 7, 6, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1));
        if (len == SIZE_MAX) // only need bits tail
        {
            msg0 = _mm_setzero_si128();
            msg1 = _mm_setzero_si128();
            msg2 = _mm_setzero_si128();
            msg3 = bitsvBE;
        }
        else if (len < 16)
        {
            msg0 = Load128With80BE(ptr++, len - 0);
            msg1 = _mm_setzero_si128();
            msg2 = _mm_setzero_si128();
            msg3 = bitsvBE;
        }
        else if (len < 32)
        {
            msg0 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
            msg1 = Load128With80BE(ptr++, len - 16);
            msg2 = _mm_setzero_si128();
            msg3 = bitsvBE;
        }
        else if (len < 48)
        {
            msg0 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
            msg1 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
            msg2 = Load128With80BE(ptr++, len - 32);
            msg3 = bitsvBE;
        }
        else // len < 56
        {
            msg0 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
            msg1 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
            msg2 = _mm_shuffle_epi8(_mm_loadu_si128(ptr++), mask);
            msg3 = Load128With80BE(ptr++, len - 48);
            msg3 = _mm_or_si128(bitsvBE, msg3);
        }
        calcBlock(state0, state1, msg0, msg1, msg2, msg3);
    }

    // state0: feba
    // state1: hgdc
    /* Save state */
    std::array<std::byte, 32> output;
    const __m128i ShuffleMask = _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    const __m128i abef = _mm_shuffle_epi8(state0, ShuffleMask);
    const __m128i cdgh = _mm_shuffle_epi8(state1, ShuffleMask);
    const __m128i abcd = _mm_unpacklo_epi64(abef, cdgh);
    const __m128i efgh = _mm_unpackhi_epi64(abef, cdgh);

    _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[0]), abcd);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[16]), efgh);
    return output;
}


#   if (COMMON_COMPILER_MSVC) || (!COMMON_COMPILER_MSVC && defined(__SHA__))
#   pragma message("Compiling DigestFuncs with SHA_NI")

// From http://software.intel.com/en-us/articles/intel-sha-extensions written by Sean Gulley.
// Modifiled from code previously on https://github.com/mitls/hacl-star/tree/master/experimental/hash with BSD license.
forceinline static void VECCALL Sha256Block_SHANI(__m128i& state0, __m128i& state1, 
    __m128i msg0, __m128i msg1, __m128i msg2, __m128i msg3) noexcept // msgs are BE
{
    /* Save current state */
    const __m128i abef_save = state0;
    const __m128i cdgh_save = state1;
    __m128i msg;

    /* Rounds 0-3 */
    msg = _mm_add_epi32(msg0, _mm_set_epi64x(0xE9B5DBA5B5C0FBCFULL, 0x71374491428A2F98ULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);

    /* Rounds 4-7 */
    msg = _mm_add_epi32(msg1, _mm_set_epi64x(0xAB1C5ED5923F82A4ULL, 0x59F111F13956C25BULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg0 = _mm_sha256msg1_epu32(msg0, msg1);

    /* Rounds 8-11 */
    msg = _mm_add_epi32(msg2, _mm_set_epi64x(0x550C7DC3243185BEULL, 0x12835B01D807AA98ULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg1 = _mm_sha256msg1_epu32(msg1, msg2);

    /* Rounds 12-15 */
    msg = _mm_add_epi32(msg3, _mm_set_epi64x(0xC19BF1749BDC06A7ULL, 0x80DEB1FE72BE5D74ULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    __m128i tmp = _mm_alignr_epi8(msg3, msg2, 4);
    msg0 = _mm_add_epi32(msg0, tmp);
    msg0 = _mm_sha256msg2_epu32(msg0, msg3);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg2 = _mm_sha256msg1_epu32(msg2, msg3);

    /* Rounds 16-19 */
    msg = _mm_add_epi32(msg0, _mm_set_epi64x(0x240CA1CC0FC19DC6ULL, 0xEFBE4786E49B69C1ULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    tmp = _mm_alignr_epi8(msg0, msg3, 4);
    msg1 = _mm_add_epi32(msg1, tmp);
    msg1 = _mm_sha256msg2_epu32(msg1, msg0);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg3 = _mm_sha256msg1_epu32(msg3, msg0);

    /* Rounds 20-23 */
    msg = _mm_add_epi32(msg1, _mm_set_epi64x(0x76F988DA5CB0A9DCULL, 0x4A7484AA2DE92C6FULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    tmp = _mm_alignr_epi8(msg1, msg0, 4);
    msg2 = _mm_add_epi32(msg2, tmp);
    msg2 = _mm_sha256msg2_epu32(msg2, msg1);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg0 = _mm_sha256msg1_epu32(msg0, msg1);

    /* Rounds 24-27 */
    msg = _mm_add_epi32(msg2, _mm_set_epi64x(0xBF597FC7B00327C8ULL, 0xA831C66D983E5152ULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    tmp = _mm_alignr_epi8(msg2, msg1, 4);
    msg3 = _mm_add_epi32(msg3, tmp);
    msg3 = _mm_sha256msg2_epu32(msg3, msg2);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg1 = _mm_sha256msg1_epu32(msg1, msg2);

    /* Rounds 28-31 */
    msg = _mm_add_epi32(msg3, _mm_set_epi64x(0x1429296706CA6351ULL, 0xD5A79147C6E00BF3ULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    tmp = _mm_alignr_epi8(msg3, msg2, 4);
    msg0 = _mm_add_epi32(msg0, tmp);
    msg0 = _mm_sha256msg2_epu32(msg0, msg3);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg2 = _mm_sha256msg1_epu32(msg2, msg3);

    /* Rounds 32-35 */
    msg = _mm_add_epi32(msg0, _mm_set_epi64x(0x53380D134D2C6DFCULL, 0x2E1B213827B70A85ULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    tmp = _mm_alignr_epi8(msg0, msg3, 4);
    msg1 = _mm_add_epi32(msg1, tmp);
    msg1 = _mm_sha256msg2_epu32(msg1, msg0);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg3 = _mm_sha256msg1_epu32(msg3, msg0);

    /* Rounds 36-39 */
    msg = _mm_add_epi32(msg1, _mm_set_epi64x(0x92722C8581C2C92EULL, 0x766A0ABB650A7354ULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    tmp = _mm_alignr_epi8(msg1, msg0, 4);
    msg2 = _mm_add_epi32(msg2, tmp);
    msg2 = _mm_sha256msg2_epu32(msg2, msg1);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg0 = _mm_sha256msg1_epu32(msg0, msg1);

    /* Rounds 40-43 */
    msg = _mm_add_epi32(msg2, _mm_set_epi64x(0xC76C51A3C24B8B70ULL, 0xA81A664BA2BFE8A1ULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    tmp = _mm_alignr_epi8(msg2, msg1, 4);
    msg3 = _mm_add_epi32(msg3, tmp);
    msg3 = _mm_sha256msg2_epu32(msg3, msg2);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg1 = _mm_sha256msg1_epu32(msg1, msg2);

    /* Rounds 44-47 */
    msg = _mm_add_epi32(msg3, _mm_set_epi64x(0x106AA070F40E3585ULL, 0xD6990624D192E819ULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    tmp = _mm_alignr_epi8(msg3, msg2, 4);
    msg0 = _mm_add_epi32(msg0, tmp);
    msg0 = _mm_sha256msg2_epu32(msg0, msg3);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg2 = _mm_sha256msg1_epu32(msg2, msg3);

    /* Rounds 48-51 */
    msg = _mm_add_epi32(msg0, _mm_set_epi64x(0x34B0BCB52748774CULL, 0x1E376C0819A4C116ULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    tmp = _mm_alignr_epi8(msg0, msg3, 4);
    msg1 = _mm_add_epi32(msg1, tmp);
    msg1 = _mm_sha256msg2_epu32(msg1, msg0);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg3 = _mm_sha256msg1_epu32(msg3, msg0);

    /* Rounds 52-55 */
    msg = _mm_add_epi32(msg1, _mm_set_epi64x(0x682E6FF35B9CCA4FULL, 0x4ED8AA4A391C0CB3ULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    tmp = _mm_alignr_epi8(msg1, msg0, 4);
    msg2 = _mm_add_epi32(msg2, tmp);
    msg2 = _mm_sha256msg2_epu32(msg2, msg1);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);

    /* Rounds 56-59 */
    msg = _mm_add_epi32(msg2, _mm_set_epi64x(0x8CC7020884C87814ULL, 0x78A5636F748F82EEULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    tmp = _mm_alignr_epi8(msg2, msg1, 4);
    msg3 = _mm_add_epi32(msg3, tmp);
    msg3 = _mm_sha256msg2_epu32(msg3, msg2);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);

    /* Rounds 60-63 */
    msg = _mm_add_epi32(msg3, _mm_set_epi64x(0xC67178F2BEF9A3F7ULL, 0xA4506CEB90BEFFFAULL));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);

    /* Combine state  */
    state0 = _mm_add_epi32(state0, abef_save);
    state1 = _mm_add_epi32(state1, cdgh_save);
}

DEFINE_FASTPATH_METHOD(Sha256, SHANI)
{
    return Sha256SSE(data, size, Sha256Block_SHANI);
}

#   endif

#endif


common::span<const MiscIntrins::VarItem> MiscIntrins::GetSupportMap() noexcept
{
    static auto list = []() 
    {
        std::vector<VarItem> ret;
        RegistFuncVars(LeadZero32, LZCNT, COMPILER);
        RegistFuncVars(LeadZero64, LZCNT, COMPILER);
        RegistFuncVars(TailZero32, TZCNT, COMPILER);
        RegistFuncVars(TailZero64, TZCNT, COMPILER);
        RegistFuncVars(PopCount32, POPCNT, COMPILER, NAIVE);
        RegistFuncVars(PopCount64, POPCNT, COMPILER, NAIVE);
        return ret;
    }();
    return list;
}
MiscIntrins::MiscIntrins(common::span<const MiscIntrins::VarItem> requests) noexcept
{
    for (const auto& [func, var] : requests)
    {
        switch (DJBHash::HashC(func))
        {
        CHECK_FUNC_VARS(func, var, LeadZero32, LZCNT, COMPILER);
        CHECK_FUNC_VARS(func, var, LeadZero64, LZCNT, COMPILER);
        CHECK_FUNC_VARS(func, var, TailZero32, TZCNT, COMPILER);
        CHECK_FUNC_VARS(func, var, TailZero64, TZCNT, COMPILER);
        CHECK_FUNC_VARS(func, var, PopCount32, POPCNT, COMPILER, NAIVE);
        CHECK_FUNC_VARS(func, var, PopCount64, POPCNT, COMPILER, NAIVE);
        }
    }
}
const MiscIntrins MiscIntrin;


common::span<const DigestFuncs::VarItem> DigestFuncs::GetSupportMap() noexcept
{
    static auto list = []()
    {
        std::vector<VarItem> ret;
        RegistFuncVars(Sha256, SHANI, NAIVE);
        return ret;
    }();
    return list;
}
DigestFuncs::DigestFuncs(common::span<const DigestFuncs::VarItem> requests) noexcept
{
    for (const auto& [func, var] : requests)
    {
        switch (DJBHash::HashC(func))
        {
        CHECK_FUNC_VARS(func, var, Sha256, SHANI, NAIVE);
        }
    }
}
const DigestFuncs DigestFunc;


}