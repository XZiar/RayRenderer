#include "SystemCommonPch.h"
#include "MiscIntrins.h"
#include "RuntimeFastPath.h"
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"
#include "3rdParty/digestpp/algorithm/sha2.hpp"

using namespace std::string_view_literals;
using common::CheckCPUFeature;
using common::MiscIntrins;
using common::DigestFuncs;

#define LeadZero32Args BOOST_PP_VARIADIC_TO_SEQ(num)
#define LeadZero64Args BOOST_PP_VARIADIC_TO_SEQ(num)
#define TailZero32Args BOOST_PP_VARIADIC_TO_SEQ(num)
#define TailZero64Args BOOST_PP_VARIADIC_TO_SEQ(num)
#define PopCount32Args BOOST_PP_VARIADIC_TO_SEQ(num)
#define PopCount64Args BOOST_PP_VARIADIC_TO_SEQ(num)
DEFINE_FASTPATH(MiscIntrins, LeadZero32);
DEFINE_FASTPATH(MiscIntrins, LeadZero64);
DEFINE_FASTPATH(MiscIntrins, TailZero32);
DEFINE_FASTPATH(MiscIntrins, TailZero64);
DEFINE_FASTPATH(MiscIntrins, PopCount32);
DEFINE_FASTPATH(MiscIntrins, PopCount64);
#define Sha256Args BOOST_PP_VARIADIC_TO_SEQ(data, size)
DEFINE_FASTPATH(DigestFuncs, Sha256);


namespace
{
using common::fastpath::FuncVarBase;

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
}

using namespace common::simd;


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


alignas(32) constexpr uint64_t SHA256RoundAdders[][2] =
{
    { 0x71374491428A2F98ULL, 0xE9B5DBA5B5C0FBCFULL },
    { 0x59F111F13956C25BULL, 0xAB1C5ED5923F82A4ULL },
    { 0x12835B01D807AA98ULL, 0x550C7DC3243185BEULL },
    { 0x80DEB1FE72BE5D74ULL, 0xC19BF1749BDC06A7ULL },
    { 0xEFBE4786E49B69C1ULL, 0x240CA1CC0FC19DC6ULL },
    { 0x4A7484AA2DE92C6FULL, 0x76F988DA5CB0A9DCULL },
    { 0xA831C66D983E5152ULL, 0xBF597FC7B00327C8ULL },
    { 0xD5A79147C6E00BF3ULL, 0x1429296706CA6351ULL },
    { 0x2E1B213827B70A85ULL, 0x53380D134D2C6DFCULL },
    { 0x766A0ABB650A7354ULL, 0x92722C8581C2C92EULL },
    { 0xA81A664BA2BFE8A1ULL, 0xC76C51A3C24B8B70ULL },
    { 0xD6990624D192E819ULL, 0x106AA070F40E3585ULL },
    { 0x1E376C0819A4C116ULL, 0x34B0BCB52748774CULL },
    { 0x4ED8AA4A391C0CB3ULL, 0x682E6FF35B9CCA4FULL },
    { 0x78A5636F748F82EEULL, 0x8CC7020884C87814ULL },
    { 0xA4506CEB90BEFFFAULL, 0xC67178F2BEF9A3F7ULL },
};
constexpr bool SHA256RoundProcControl[][2] =
{ // { ProcPrev, ProcNext }
    { false, false },
    {  true, false },
    {  true, false },
    {  true,  true },
    {  true,  true },
    {  true,  true },
    {  true,  true },
    {  true,  true },
    {  true,  true },
    {  true,  true },
    {  true,  true },
    {  true,  true },
    {  true,  true },
    { false,  true },
    { false,  true },
    { false, false },
};

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 41) || (COMMON_ARCH_ARM && COMMON_SIMD_LV >= 100)
struct Sha256Calc
{
    U32x4 State0, State1;
    /* Load initial values */
    //U32x4 state0(0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a); // ABCD
    //U32x4 state1(0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19); // EFGH
    Sha256Calc() noexcept : 
        State0(0x9b05688c, 0x510e527f, 0xbb67ae85, 0x6a09e667), // ABEF-rev
        State1(0x5be0cd19, 0x1f83d9ab, 0xa54ff53a, 0x3c6ef372)  // CDGH-rev 
    { }
    /* Save state */
    forceinline std::array<std::byte, 32> Output() const noexcept
    {
        // state0: feba
        // state1: hgdc
        std::array<std::byte, 32> output;
        const U8x16 ReverseMask(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        const auto abef = State0.As<U8x16>().Shuffle(ReverseMask);
        const auto cdgh = State1.As<U8x16>().Shuffle(ReverseMask);
        const auto abcd = abef.As<U64x2>().ZipLo(cdgh.As<U64x2>()).As<U8x16>();
        const auto efgh = abef.As<U64x2>().ZipHi(cdgh.As<U64x2>()).As<U8x16>();
        abcd.Save(reinterpret_cast<uint8_t*>(&output[0]));
        efgh.Save(reinterpret_cast<uint8_t*>(&output[16]));
        return output;
    }
};


forceinline U32x4 Load128With80BE(const uint32_t* data, const size_t len) noexcept
{
    // Expects len < 16
    if (len == 0)
        return U32x4::LoadLo(0x80000000);
        //return _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<char>(0x80), 0, 0, 0);
    U32x4 val(data);
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

// From http://software.intel.com/en-us/articles/intel-sha-extensions written by Sean Gulley.
template<typename Calc>
forceinline static void VECCALL Sha256Block128(Calc& calc, U32x4 msg0, U32x4 msg1, U32x4 msg2, U32x4 msg3) noexcept // msgs are BE
{
    /* Save current state */
    const auto abef_save = calc.State0;
    const auto cdgh_save = calc.State1;
    /* Rounds 0-3 */
    calc.template Calc< 0>(msg3, msg0, msg1);
    /* Rounds 4-7 */
    calc.template Calc< 1>(msg0, msg1, msg2);
    /* Rounds 8-11 */
    calc.template Calc< 2>(msg1, msg2, msg3);
    /* Rounds 12-15 */
    calc.template Calc< 3>(msg2, msg3, msg0);
    /* Rounds 16-19 */
    calc.template Calc< 4>(msg3, msg0, msg1);
    /* Rounds 20-23 */
    calc.template Calc< 5>(msg0, msg1, msg2);
    /* Rounds 24-27 */
    calc.template Calc< 6>(msg1, msg2, msg3);
    /* Rounds 28-31 */
    calc.template Calc< 7>(msg2, msg3, msg0);
    /* Rounds 32-35 */
    calc.template Calc< 8>(msg3, msg0, msg1);
    /* Rounds 36-39 */
    calc.template Calc< 9>(msg0, msg1, msg2);
    /* Rounds 40-43 */
    calc.template Calc<10>(msg1, msg2, msg3);
    /* Rounds 44-47 */
    calc.template Calc<11>(msg2, msg3, msg0);
    /* Rounds 48-51 */
    calc.template Calc<12>(msg3, msg0, msg1);
    /* Rounds 52-55 */
    calc.template Calc<13>(msg0, msg1, msg2);
    /* Rounds 56-59 */
    calc.template Calc<14>(msg1, msg2, msg3);
    /* Rounds 60-63 */
    calc.template Calc<15>(msg2, msg3, msg0);
    /* Combine state  */
    calc.State0 += abef_save;
    calc.State1 += cdgh_save;
}

template<typename Calc>
inline std::array<std::byte, 32> Sha256Main128(const std::byte* data, const size_t size) noexcept
{
    static_assert(std::is_base_of_v<Sha256Calc, Calc>);
    Calc calc;

    size_t len = size;
    const uint32_t* __restrict ptr = reinterpret_cast<const uint32_t*>(data);
    while (len >= 64)
    {
        const auto msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
        const auto msg1 = U32x4(ptr).SwapEndian(); ptr += 4;
        const auto msg2 = U32x4(ptr).SwapEndian(); ptr += 4;
        const auto msg3 = U32x4(ptr).SwapEndian(); ptr += 4;
        Sha256Block128(calc, msg0, msg1, msg2, msg3);
        len -= 64;
    }
    if (len >= 56)
    {
        const auto msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
        const auto msg1 = U32x4(ptr).SwapEndian(); ptr += 4;
        const auto msg2 = U32x4(ptr).SwapEndian(); ptr += 4;
        const auto msg3 = Load128With80BE(ptr, len % 16);
        Sha256Block128(calc, msg0, msg1, msg2, msg3);
        len = SIZE_MAX;
    }
    {
        U32x4 msg0, msg1, msg2, msg3;
        const auto bitsv = U64x2::LoadLo(static_cast<uint64_t>(size) * 8);
        const auto bitsvBE = bitsv.As<U32x4>().Shuffle<3, 3, 1, 0>();
        if (len == SIZE_MAX) // only need bits tail
        {
            msg0 = U32x4::AllZero();
            msg1 = U32x4::AllZero();
            msg2 = U32x4::AllZero();
            msg3 = bitsvBE;
        }
        else if (len < 16)
        {
            msg0 = Load128With80BE(ptr, len - 0); ptr += 4;
            msg1 = U32x4::AllZero();
            msg2 = U32x4::AllZero();
            msg3 = bitsvBE;
        }
        else if (len < 32)
        {
            msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
            msg1 = Load128With80BE(ptr, len - 16); ptr += 4;
            msg2 = U32x4::AllZero();
            msg3 = bitsvBE;
        }
        else if (len < 48)
        {
            msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
            msg1 = U32x4(ptr).SwapEndian(); ptr += 4;
            msg2 = Load128With80BE(ptr, len - 32); ptr += 4;
            msg3 = bitsvBE;
        }
        else // len < 56
        {
            msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
            msg1 = U32x4(ptr).SwapEndian(); ptr += 4;
            msg2 = U32x4(ptr).SwapEndian(); ptr += 4;
            msg3 = Load128With80BE(ptr, len - 48); ptr += 4;
            msg3 |= bitsvBE;
        }
        Sha256Block128(calc, msg0, msg1, msg2, msg3);
    }

    return calc.Output();
}


# if COMMON_ARCH_X86 && (COMMON_COMPILER_MSVC || defined(__SHA__))
# pragma message("Compiling DigestFuncs with SHA_NI")
struct Sha256Round_SHANI : public Sha256Calc
{
    // From http://software.intel.com/en-us/articles/intel-sha-extensions written by Sean Gulley.
    // Modifiled from code previously on https://github.com/mitls/hacl-star/tree/master/experimental/hash with BSD license.
    template<size_t Round>
    forceinline void VECCALL Calc([[maybe_unused]] U32x4& prev, U32x4& cur, [[maybe_unused]] U32x4& next) noexcept // msgs are BE
    {
        const auto adder = U64x2(SHA256RoundAdders[Round]).As<U32x4>();
        auto msg = cur + adder;
        State1 = _mm_sha256rnds2_epu32(State1, State0, msg);
        if constexpr (SHA256RoundProcControl[Round][1])
        {
            const U32x4 tmp = _mm_alignr_epi8(cur, prev, 4);
            next = _mm_sha256msg2_epu32(next + tmp, cur);
        }
        msg = msg.Shuffle<2, 3, 0, 0>(); // _mm_shuffle_epi32(msg, 0x0E);
        State0 = _mm_sha256rnds2_epu32(State0, State1, msg);
        if constexpr (SHA256RoundProcControl[Round][0])
            prev = _mm_sha256msg1_epu32(prev, cur);
    }
};
DEFINE_FASTPATH_METHOD(Sha256, SHANI)
{
    return Sha256Main128<Sha256Round_SHANI>(data, size);
}
# elif COMMON_ARCH_ARM
# pragma message("Compiling DigestFuncs with SHA2")
//struct Sha256Round_SHANI : public Sha256Calc
//{
//    template<size_t Round>
//    forceinline void VECCALL Calc([[maybe_unused]] U32x4& prev, U32x4& cur, [[maybe_unused]] U32x4& next) noexcept // msgs are BE
//    {
//        const auto adder = U64x2(SHA256RoundAdders[Round]).As<U32x4>();
//        auto msg = cur + adder;
//        State1 = _mm_sha256rnds2_epu32(State1, State0, msg);
//        if constexpr (SHA256RoundProcControl[Round][1])
//        {
//            const U32x4 tmp = _mm_alignr_epi8(cur, prev, 4);
//            next = _mm_sha256msg2_epu32(next + tmp, cur);
//        }
//        msg = msg.Shuffle<2, 3, 0, 0>(); // _mm_shuffle_epi32(msg, 0x0E);
//        State0 = _mm_sha256rnds2_epu32(State0, State1, msg);
//        if constexpr (SHA256RoundProcControl[Round][0])
//            prev = _mm_sha256msg1_epu32(prev, cur);
//    }
//};
//DEFINE_FASTPATH_METHOD(Sha256, SHA2)
//{
//    return Sha256Main128<Sha256Round_SHA2>(data, size);
//}
# endif

#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 200
// From http://software.intel.com/en-us/articles/intel-sha-extensions written by Sean Gulley.
template<typename Calc>
forceinline static void VECCALL Sha256Block256(Calc& calc, U32x8 msg01, U32x8 msg23) noexcept // msgs are BE
{
    /* Save current state */
    const auto abef_save = calc.State0;
    const auto cdgh_save = calc.State1;
    U32x4 msg0 = msg01.GetLoLane(), msg1 = msg01.GetHiLane(), msg2 = msg23.GetLoLane(), msg3 = msg23.GetHiLane(), msgLo;
    ///* Rounds 0-3 */ calc.template Calc< 0>(msg3, msg0, msg1);
    ///* Rounds 4-7 */ calc.template Calc< 1>(msg0, msg1, msg2);
    /* Rounds 0-7 */
    calc.template CalcFFTF< 0>(msg01, msg0);
    ///* Rounds 8-11 */  calc.template Calc< 2>(msg1, msg2, msg3);
    ///* Rounds 12-15 */ calc.template Calc< 3>(msg2, msg3, msg0);
    /* Rounds 8-15 */
    calc.template CalcTFTT< 2>(msg23, msg1, msg2, msg0);
    /* Rounds 16-19 */
    calc.template Calc< 4>(msg3, msg0, msg1);
    /* Rounds 20-23 */
    calc.template Calc< 5>(msg0, msg1, msg2);
    /* Rounds 24-27 */
    calc.template Calc< 6>(msg1, msg2, msg3);
    /* Rounds 28-31 */
    calc.template Calc< 7>(msg2, msg3, msg0);
    /* Rounds 32-35 */
    calc.template Calc< 8>(msg3, msg0, msg1);
    /* Rounds 36-39 */
    calc.template Calc< 9>(msg0, msg1, msg2);
    /* Rounds 40-43 */
    calc.template Calc<10>(msg1, msg2, msg3);
    /* Rounds 44-47 */
    calc.template Calc<11>(msg2, msg3, msg0);
    /* Rounds 48-51 */
    calc.template Calc<12>(msg3, msg0, msg1);
    /* Rounds 52-55 */
    calc.template Calc<13>(msg0, msg1, msg2);
    /* Rounds 56-59 */
    calc.template Calc<14>(msg1, msg2, msg3);
    /* Rounds 60-63 */
    calc.template Calc<15>(msg2, msg3, msg0);
    /* Combine state  */
    calc.State0 += abef_save;
    calc.State1 += cdgh_save;
}
template<typename Calc>
inline std::array<std::byte, 32> Sha256Main256(const std::byte* data, const size_t size) noexcept
{
    static_assert(std::is_base_of_v<Sha256Calc, Calc>);
    Calc calc;

    size_t len = size;
    const uint32_t* __restrict ptr = reinterpret_cast<const uint32_t*>(data);
    while (len >= 64)
    {
        const auto msg01 = U32x8(ptr).SwapEndian(); ptr += 8;
        const auto msg23 = U32x8(ptr).SwapEndian(); ptr += 8;
        Sha256Block256(calc, msg01, msg23);
        len -= 64;
    }
    if (len >= 56)
    {
        const auto msg01 = U32x8(ptr).SwapEndian(); ptr += 8;
        const auto msg2  = U32x4(ptr).SwapEndian(); ptr += 4;
        const auto msg3  = Load128With80BE(ptr, len % 16);
        const auto msg23 = U32x8(msg2, msg3);
        Sha256Block256(calc, msg01, msg23);
        len = SIZE_MAX;
    }
    {
        U32x8 msg01, msg23;
        const auto bitsv = U64x2::LoadLo(static_cast<uint64_t>(size) * 8);
        const auto bitsvBE = U32x8(U32x4::AllZero(), bitsv.As<U32x4>().Shuffle<3, 3, 1, 0>());
        //const auto bitsvBE = U32x8::LoadLoLane(bitsv.As<U32x4>().Shuffle<3, 3, 1, 0>());
        if (len == SIZE_MAX) // only need bits tail
        {
            msg01 = U32x8::AllZero();
            msg23 = bitsvBE;
        }
        else if (len < 16)
        {
            const auto msg0 = Load128With80BE(ptr, len - 0); ptr += 4;
            const auto msg1 = U32x4::AllZero();
            msg01 = U32x8(msg0, msg1);
            msg23 = bitsvBE;
        }
        else if (len < 32)
        {
            const auto msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
            const auto msg1 = Load128With80BE(ptr, len - 16); ptr += 4;
            msg01 = U32x8(msg0, msg1);
            msg23 = bitsvBE;
        }
        else if (len < 48)
        {
            msg01 = U32x8(ptr).SwapEndian(); ptr += 8;
            const auto msg2 = Load128With80BE(ptr, len - 32); ptr += 4;
            msg23 = U32x8::LoadLoLane(msg2).Or(bitsvBE);
        }
        else // len < 56
        {
            msg01 = U32x8(ptr).SwapEndian(); ptr += 8;
            const auto msg2 = U32x4(ptr).SwapEndian(); ptr += 4;
            const auto msg3 = Load128With80BE(ptr, len - 48); ptr += 4;
            msg23 = U32x8(msg2, msg3).Or(bitsvBE);
        }
        Sha256Block256(calc, msg01, msg23);
    }

    return calc.Output();
}
# if COMMON_ARCH_X86 && (COMMON_COMPILER_MSVC || defined(__SHA__))
struct Sha256Round_SHANIAVX2 : public Sha256Round_SHANI
{
    // From http://software.intel.com/en-us/articles/intel-sha-extensions written by Sean Gulley.
    // Modifiled from code previously on https://github.com/mitls/hacl-star/tree/master/experimental/hash with BSD license.
    template<size_t Round>
    forceinline void VECCALL CalcFFTF(const U32x8& msgLH, U32x4& msgL) noexcept // msgs are BE
    {
        static_assert(SHA256RoundProcControl[Round][0] == false && SHA256RoundProcControl[Round][1] == false &&
            SHA256RoundProcControl[Round + 1][0] == true && SHA256RoundProcControl[Round + 1][0 + 1] == false); // FF, TF
        const auto adderLH = U64x4(SHA256RoundAdders[Round]).As<U32x8>();
        auto msgAddLH = msgLH + adderLH;
        auto msgShufLH = msgAddLH.ShuffleLane<2, 3, 0, 0>(); // _mm256_shuffle_epi32(msgAddLH, 0x0E);
        State1 = _mm_sha256rnds2_epu32(State1, State0, msgAddLH.GetLoLane());
        State0 = _mm_sha256rnds2_epu32(State0, State1, msgShufLH.GetLoLane());
        State1 = _mm_sha256rnds2_epu32(State1, State0, msgAddLH.GetHiLane());
        State0 = _mm_sha256rnds2_epu32(State0, State1, msgShufLH.GetHiLane());
        msgL = _mm_sha256msg1_epu32(msgLH.GetLoLane(), msgLH.GetHiLane());
    }
    template<size_t Round>
    forceinline void VECCALL CalcTFTT(const U32x8& msgLH, U32x4& msgP, U32x4& msgL, U32x4& msgN) noexcept // msgs are BE
    {
        static_assert(SHA256RoundProcControl[Round][0] == true && SHA256RoundProcControl[Round][1] == false &&
            SHA256RoundProcControl[Round + 1][0] == true && SHA256RoundProcControl[Round + 1][0 + 1] == true); // TF, TT
        const auto adderLH = U64x4(SHA256RoundAdders[Round]).As<U32x8>();
        auto msgAddLH = msgLH + adderLH;
        auto msgShufLH = msgAddLH.ShuffleLane<2, 3, 0, 0>(); // _mm256_shuffle_epi32(msgAddLH, 0x0E);
        State1 = _mm_sha256rnds2_epu32(State1, State0, msgAddLH.GetLoLane());
        State0 = _mm_sha256rnds2_epu32(State0, State1, msgShufLH.GetLoLane());
        State1 = _mm_sha256rnds2_epu32(State1, State0, msgAddLH.GetHiLane());
        State0 = _mm_sha256rnds2_epu32(State0, State1, msgShufLH.GetHiLane());
        const U32x4 msgLo = msgLH.GetLoLane(), msgHi = msgLH.GetHiLane();
        const U32x4 tmp = _mm_alignr_epi8(msgHi, msgLo, 4);
        msgP = _mm_sha256msg1_epu32(msgP, msgLo);
        msgN = _mm_sha256msg2_epu32(msgN + tmp, msgHi);
        msgL = _mm_sha256msg1_epu32(msgLo, msgHi);
    }
};
DEFINE_FASTPATH_METHOD(Sha256, SHANIAVX2)
{
    return Sha256Main256<Sha256Round_SHANIAVX2>(data, size);
}
# endif
#endif



namespace common
{

common::span<const MiscIntrins::PathInfo> MiscIntrins::GetSupportMap() noexcept
{
    static auto list = []() 
    {
        std::vector<PathInfo> ret;
        RegistFuncVars(MiscIntrins, LeadZero32, LZCNT, COMPILER);
        RegistFuncVars(MiscIntrins, LeadZero64, LZCNT, COMPILER);
        RegistFuncVars(MiscIntrins, TailZero32, TZCNT, COMPILER);
        RegistFuncVars(MiscIntrins, TailZero64, TZCNT, COMPILER);
        RegistFuncVars(MiscIntrins, PopCount32, POPCNT, COMPILER, NAIVE);
        RegistFuncVars(MiscIntrins, PopCount64, POPCNT, COMPILER, NAIVE);
        return ret;
    }();
    return list;
}
MiscIntrins::MiscIntrins(common::span<const VarItem> requests) noexcept { Init(requests); }
MiscIntrins::~MiscIntrins() {}
bool MiscIntrins::IsComplete() const noexcept
{
    return LeadZero32 && LeadZero64 && TailZero32 && TailZero64 && PopCount32 && PopCount64;
}

const MiscIntrins MiscIntrin;


common::span<const DigestFuncs::PathInfo> DigestFuncs::GetSupportMap() noexcept
{
    static auto list = []()
    {
        std::vector<PathInfo> ret;
        RegistFuncVars(DigestFuncs, Sha256, SHANIAVX2, SHANI, NAIVE);
        return ret;
    }();
    return list;
}
DigestFuncs::DigestFuncs(common::span<const VarItem> requests) noexcept { Init(requests); }
DigestFuncs::~DigestFuncs() {}
bool DigestFuncs::IsComplete() const noexcept
{
    return Sha256;
}
const DigestFuncs DigestFunc;


}