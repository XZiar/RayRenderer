#include "SystemCommonPch.h"
#include "MiscIntrins.h"
#include "RuntimeFastPath.h"
#include "SpinLock.h"
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"
#include "common/SpinLock.hpp"
#include "3rdParty/digestpp/algorithm/sha2.hpp"

using namespace std::string_view_literals;
using common::CheckCPUFeature;
using common::MiscIntrins;
using common::DigestFuncs;

#define LeadZero32Args  BOOST_PP_VARIADIC_TO_SEQ(num)
#define LeadZero64Args  BOOST_PP_VARIADIC_TO_SEQ(num)
#define TailZero32Args  BOOST_PP_VARIADIC_TO_SEQ(num)
#define TailZero64Args  BOOST_PP_VARIADIC_TO_SEQ(num)
#define PopCount32Args  BOOST_PP_VARIADIC_TO_SEQ(num)
#define PopCount64Args  BOOST_PP_VARIADIC_TO_SEQ(num)
#define Hex2StrArgs     BOOST_PP_VARIADIC_TO_SEQ(data, size, isCapital)
#define PauseCyclesArgs BOOST_PP_VARIADIC_TO_SEQ(cycles)
DEFINE_FASTPATH(MiscIntrins, LeadZero32);
DEFINE_FASTPATH(MiscIntrins, LeadZero64);
DEFINE_FASTPATH(MiscIntrins, TailZero32);
DEFINE_FASTPATH(MiscIntrins, TailZero64);
DEFINE_FASTPATH(MiscIntrins, PopCount32);
DEFINE_FASTPATH(MiscIntrins, PopCount64);
DEFINE_FASTPATH(MiscIntrins, Hex2Str);
DEFINE_FASTPATH(MiscIntrins, PauseCycles);
#define Sha256Args BOOST_PP_VARIADIC_TO_SEQ(data, size)
DEFINE_FASTPATH(DigestFuncs, Sha256);


namespace
{
using common::fastpath::FuncVarBase;

struct NAIVE : FuncVarBase {};
struct COMPILER : FuncVarBase {};
struct OS : FuncVarBase {};
struct SIMD128
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("sse2"sv);
#else
        return CheckCPUFeature("asimd"sv);
#endif
    }
};
struct SIMDSSSE3
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("ssse3"sv);
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
struct SSE2
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("sse2"sv);
#else
        return false;
#endif
    }
};
struct WAITPKG
{
    static bool RuntimeCheck() noexcept
    {
#if COMMON_ARCH_X86
        return CheckCPUFeature("waitpkg"sv);
#else
        return false;
#endif
    }
};
}

using namespace common::simd;

#if COMMON_ARCH_X86
#    define PauseCycleLoop(...) \
const auto tsc = __rdtsc();     \
do                              \
{                               \
    __VA_ARGS__                 \
} while (__rdtsc() - tsc < cycles);
#else
#    define PauseCycleLoop(...)             \
cycles &= 0xffffff80u;                      \
for (uint32_t i = 0; i <= cycles; i += 32)  \
{                                           \
    __VA_ARGS__                             \
}
#endif


DEFINE_FASTPATH_METHOD(PauseCycles, COMPILER)
{
    PauseCycleLoop(COMMON_PAUSE();)
    return true;
}

#if COMMON_ARCH_X86

# if COMMON_SIMD_LV >= 20
DEFINE_FASTPATH_METHOD(PauseCycles, SSE2)
{
    PauseCycleLoop(_mm_pause();)
    return true;
}
# endif

# if (COMMON_COMPILER_MSVC && COMMON_MSVC_VER >= 192200) || (COMMON_COMPILER_CLANG && COMMON_CLANG_VER >= 70000) || (COMMON_COMPILER_GCC && COMMON_GCC_VER >= 90000)
#   pragma message("Compiling MiscIntrins with WAITPKG")
#   if COMMON_COMPILER_GCC
#     pragma GCC push_options
#     pragma GCC target("waitpkg")
#   elif COMMON_COMPILER_CLANG
#     pragma clang attribute push (__attribute__((target("waitpkg"))), apply_to=function)
#   endif
DEFINE_FASTPATH_METHOD(PauseCycles, WAITPKG)
{
    const auto tsc = __rdtsc();
    return _umwait(0, tsc + cycles);
}
#   if COMMON_COMPILER_GCC
#     pragma GCC pop_options
#   elif COMMON_COMPILER_CLANG
#     pragma clang attribute pop
#   endif
# endif

#endif
#undef PauseCycleLoop


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


static void Hex2Str(std::string& ret, const uint8_t* data, size_t size, bool isCapital)
{
    constexpr auto ch = "0123456789abcdef0123456789ABCDEF";
    const auto ptr = isCapital ? ch + 16 : ch;
    for (size_t i = 0; i < size; ++i)
    {
        const uint8_t dat = data[i];
        ret.push_back(ptr[dat / 16]);
        ret.push_back(ptr[dat % 16]);
    }
}

DEFINE_FASTPATH_METHOD(Hex2Str, NAIVE)
{
    std::string ret;
    ret.reserve(size * 2);
    Hex2Str(ret, data, size, isCapital);
    return ret;
}

#if (COMMON_ARCH_X86 && COMMON_SIMD_LV >= 31) || (COMMON_ARCH_ARM && COMMON_SIMD_LV >= 10)
# if COMMON_ARCH_X86
#   define ALGO SIMDSSSE3
# else
#   define ALGO SIMD128
# endif
DEFINE_FASTPATH_METHOD(Hex2Str, ALGO)
{
    alignas(16) constexpr uint8_t strs[] = "0123456789abcdef0123456789ABCDEF";
    const U8x16 chars(isCapital ? strs + 16 : strs);
    std::string ret;
    ret.reserve(size * 2);
    ret.resize((size / 8) * 16);
    auto dst = reinterpret_cast<uint8_t*>(ret.data());
    while (size >= 16)
    {
        const U8x16 dat(data);
        const auto loData = dat.And(0x0f), hiData = dat.ShiftRightLogic<4>();
        const auto loChar = chars.Shuffle(loData), hiChar = chars.Shuffle(hiData);
        const auto part0  = hiChar.ZipLo(loChar), part1 = hiChar.ZipHi(loChar);
        part0.Save(dst), part1.Save(dst + 16);
        data += 16, size -= 16, dst += 32;
    }
    if (size >= 8)
    {
        const auto dat = U64x2::LoadLo(reinterpret_cast<const uint64_t*>(data)).As<U8x16>();
        const auto loData   = dat.And(0x0f), hiData = dat.ShiftRightLogic<4>();
        const auto fullData = hiData.ZipLo(loData);
        const auto fullChar = chars.Shuffle(fullData);
        fullChar.Save(dst);
        data += 8, size -= 8, dst += 16;
    }
    Hex2Str(ret, data, size, isCapital);
    return ret;
}
# undef ALGO
#endif


alignas(32) constexpr uint32_t SHA256RoundAdders[][4] =
{
    { 0x428A2F98u, 0x71374491u, 0xB5C0FBCFu, 0xE9B5DBA5u },
    { 0x3956C25Bu, 0x59F111F1u, 0x923F82A4u, 0xAB1C5ED5u },
    { 0xD807AA98u, 0x12835B01u, 0x243185BEu, 0x550C7DC3u },
    { 0x72BE5D74u, 0x80DEB1FEu, 0x9BDC06A7u, 0xC19BF174u },
    { 0xE49B69C1u, 0xEFBE4786u, 0x0FC19DC6u, 0x240CA1CCu },
    { 0x2DE92C6Fu, 0x4A7484AAu, 0x5CB0A9DCu, 0x76F988DAu },
    { 0x983E5152u, 0xA831C66Du, 0xB00327C8u, 0xBF597FC7u },
    { 0xC6E00BF3u, 0xD5A79147u, 0x06CA6351u, 0x14292967u },
    { 0x27B70A85u, 0x2E1B2138u, 0x4D2C6DFCu, 0x53380D13u },
    { 0x650A7354u, 0x766A0ABBu, 0x81C2C92Eu, 0x92722C85u },
    { 0xA2BFE8A1u, 0xA81A664Bu, 0xC24B8B70u, 0xC76C51A3u },
    { 0xD192E819u, 0xD6990624u, 0xF40E3585u, 0x106AA070u },
    { 0x19A4C116u, 0x1E376C08u, 0x2748774Cu, 0x34B0BCB5u },
    { 0x391C0CB3u, 0x4ED8AA4Au, 0x5B9CCA4Fu, 0x682E6FF3u },
    { 0x748F82EEu, 0x78A5636Fu, 0x84C87814u, 0x8CC70208u },
    { 0x90BEFFFAu, 0xA4506CEBu, 0xBEF9A3F7u, 0xC67178F2u },
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
struct Sha256State
{
    //U32x4 state0(0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a); // ABCD
    //U32x4 state1(0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19); // EFGH
    U32x4 State0, State1;
    Sha256State(U32x4 state0, U32x4 state1) : State0(state0), State1(state1) { }
};

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
    static_assert(std::is_base_of_v<Sha256State, Calc>);
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
    const auto bitsv = U64x2::LoadLo(static_cast<uint64_t>(size) * 8);
    const auto bitsvBE = bitsv.As<U32x4>().Shuffle<3, 3, 1, 0>();
    if (len >= 48)
    {
        const auto msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
        const auto msg1 = U32x4(ptr).SwapEndian(); ptr += 4;
        const auto msg2 = U32x4(ptr).SwapEndian(); ptr += 4;
              auto msg3 = Calc::Load128With80BE(ptr, len % 16);
        if (len < 56)
            msg3 |= bitsvBE;
        Sha256Block128(calc, msg0, msg1, msg2, msg3);
        if (len >= 56)
            Sha256Block128(calc, U32x4::AllZero(), U32x4::AllZero(), U32x4::AllZero(), bitsvBE);
    }
    else
    {
        U32x4 msg0, msg1, msg2, msg3;
        if (len < 16)
        {
            msg0 = Calc::Load128With80BE(ptr, len - 0); ptr += 4;
            msg1 = U32x4::AllZero();
            msg2 = U32x4::AllZero();
            msg3 = bitsvBE;
        }
        else if (len < 32)
        {
            msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
            msg1 = Calc::Load128With80BE(ptr, len - 16); ptr += 4;
            msg2 = U32x4::AllZero();
            msg3 = bitsvBE;
        }
        else // if (len < 48)
        {
            msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
            msg1 = U32x4(ptr).SwapEndian(); ptr += 4;
            msg2 = Calc::Load128With80BE(ptr, len - 32); ptr += 4;
            msg3 = bitsvBE;
        }
        Sha256Block128(calc, msg0, msg1, msg2, msg3);
    }
    return calc.Output();
}


# if COMMON_ARCH_X86 && (COMMON_COMPILER_MSVC || defined(__SHA__))

# pragma message("Compiling DigestFuncs with SHA_NI")
struct Sha256Round_SHANI : public Sha256State
{
    forceinline static U32x4 Load128With80BE(const uint32_t* data, const size_t len) noexcept
    {
        // Expects len < 16
        if (len == 0)
            return U32x4::LoadLo(0x80000000u);
        const auto val = U32x4(data).SwapEndian();
        const U8x16 IdxConst(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);
        const U8x16 SizeMask(static_cast<char>(len));                                                   // x, x, x, x
        const auto EndMask   = SizeMask.Compare<CompareType::Equal,       MaskType::FullEle>(IdxConst); // 00,00,ff,00 or 00,00,00,ff
        const auto KeepMask  = SizeMask.Compare<CompareType::GreaterThan, MaskType::FullEle>(IdxConst); // ff,ff,00,00 or ff,ff,ff,00
        const auto EndSigBit = EndMask.As<U16x8>().ShiftLeftLogic<7>().As<U8x16>();                     // 00,00,80,7f or 00,00,00,80
        const auto SuffixBit = EndSigBit & EndMask;                                                     // 00,00,80,00 or 00,00,00,80
        return SuffixBit.SelectWith<MaskType::FullEle>(val.As<U8x16>(), KeepMask).As<U32x4>();
    }

    Sha256Round_SHANI() noexcept : Sha256State(
        { 0x9b05688c, 0x510e527f, 0xbb67ae85, 0x6a09e667 }/*ABEF-rev*/, 
        { 0x5be0cd19, 0x1f83d9ab, 0xa54ff53a, 0x3c6ef372 }/*CDGH-rev*/) 
    { }
    /* Save state */
    forceinline std::array<std::byte, 32> Output() const noexcept
    {
        std::array<std::byte, 32> output;
        // state0: feba
        // state1: hgdc
        const U8x16 ReverseMask(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        const auto abef = State0.As<U8x16>().Shuffle(ReverseMask);
        const auto cdgh = State1.As<U8x16>().Shuffle(ReverseMask);
        const auto abcd = abef.As<U64x2>().ZipLo(cdgh.As<U64x2>()).As<U8x16>();
        const auto efgh = abef.As<U64x2>().ZipHi(cdgh.As<U64x2>()).As<U8x16>();
        abcd.Save(reinterpret_cast<uint8_t*>(&output[0]));
        efgh.Save(reinterpret_cast<uint8_t*>(&output[16]));
        return output;
    }

    // From http://software.intel.com/en-us/articles/intel-sha-extensions written by Sean Gulley.
    // Modifiled from code previously on https://github.com/mitls/hacl-star/tree/master/experimental/hash with BSD license.
    template<size_t Round>
    forceinline void VECCALL Calc([[maybe_unused]] U32x4& prev, U32x4& cur, [[maybe_unused]] U32x4& next) noexcept // msgs are BE
    {
        const U32x4 adder(SHA256RoundAdders[Round]);
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
        {
            prev = _mm_sha256msg1_epu32(prev, cur);
        }
    }
};
DEFINE_FASTPATH_METHOD(Sha256, SHANI)
{
    return Sha256Main128<Sha256Round_SHANI>(data, size);
}

# elif COMMON_ARCH_ARM && defined(__ARM_FEATURE_CRYPTO)

#   pragma message("Compiling DigestFuncs with SHA2")
struct Sha256Round_SHA2 : public Sha256State
{
    forceinline static U32x4 Load128With80BE(const uint32_t* data, const size_t len) noexcept
    {
        // Expects len < 16
        if (len == 0)
            return U32x4::LoadLo(0x80000000);
        const auto val = U32x4(data).SwapEndian();
        const U8x16 IdxConst(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);
        const U8x16 SizeMask(static_cast<char>(len));                                                   // x, x, x, x
        const auto EndMask   = SizeMask.Compare<CompareType::Equal,       MaskType::FullEle>(IdxConst); // 00,00,ff,00 or 00,00,00,ff
        const auto KeepMask  = SizeMask.Compare<CompareType::GreaterThan, MaskType::FullEle>(IdxConst); // ff,ff,00,00 or ff,ff,ff,00
        const auto SuffixBit = EndMask.ShiftLeftLogic<7>();                                             // 00,00,80,00 or 00,00,00,80
        return SuffixBit.SelectWith<MaskType::FullEle>(val.As<U8x16>(), KeepMask).As<U32x4>();
    }

    Sha256Round_SHA2() noexcept : Sha256State(
        { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a }/*ABCD*/,
        { 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 }/*EFGH*/) 
    { }
    /* Save state */
    forceinline std::array<std::byte, 32> Output() const noexcept
    {
        std::array<std::byte, 32> output;
        State0.SwapEndian().As<U8x16>().Save(reinterpret_cast<uint8_t*>(&output[0]));
        State1.SwapEndian().As<U8x16>().Save(reinterpret_cast<uint8_t*>(&output[16]));
        return output;
    }
    template<size_t Round>
    forceinline void VECCALL Calc([[maybe_unused]] U32x4& prev, U32x4& cur, [[maybe_unused]] U32x4& next) noexcept // msgs are BE
    {
        const U32x4 adder(SHA256RoundAdders[Round]);
        auto msg = cur + adder;
        const auto state0 = State0;
        State0 = vsha256hq_u32 (State0, State1, msg);
        State1 = vsha256h2q_u32(State1, state0, msg);
        if constexpr (SHA256RoundProcControl[Round][1])
        {
            next = vsha256su1q_u32(next, prev, cur);
        }
        if constexpr (SHA256RoundProcControl[Round][0])
        {
            prev = vsha256su0q_u32(prev, cur);
        }
    }
};
DEFINE_FASTPATH_METHOD(Sha256, SHA2)
{
    return Sha256Main128<Sha256Round_SHA2>(data, size);
}

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
    U32x4 msg0 = msg01.GetLoLane(), msg1 = msg01.GetHiLane(), msg2 = msg23.GetLoLane(), msg3 = msg23.GetHiLane();
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
    static_assert(std::is_base_of_v<Sha256State, Calc>);
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
    const auto bitsv = U64x2::LoadLo(static_cast<uint64_t>(size) * 8);
    //const auto bitsvBE = U32x8(U32x4::AllZero(), bitsv.As<U32x4>().Shuffle<3, 3, 1, 0>());
    const auto bitsvBE = bitsv.As<U32x4>().Shuffle<3, 3, 1, 0>();
    if (len >= 48)
    {
        const auto msg01 = U32x8(ptr).SwapEndian(); ptr += 8;
        const auto msg2  = U32x4(ptr).SwapEndian(); ptr += 4;
              auto msg3  = Calc::Load128With80BE(ptr, len % 16);
        if (len < 56)
            msg3 |= bitsvBE;
        Sha256Block256(calc, msg01, U32x8(msg2, msg3));
        if (len >= 56)
            Sha256Block256(calc, U32x8::AllZero(), { U32x4::AllZero(), bitsvBE });
    }
    else
    {
        U32x8 msg01, msg23;
        if (len < 16)
        {
            const auto msg0 = Calc::Load128With80BE(ptr, len - 0); ptr += 4;
            msg01 = U32x8::LoadLoLane(msg0);
            msg23 = U32x8(U32x4::AllZero(), bitsvBE);
        }
        else if (len < 32)
        {
            const auto msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
            const auto msg1 = Calc::Load128With80BE(ptr, len - 16); ptr += 4;
            msg01 = U32x8(msg0, msg1);
            msg23 = U32x8(U32x4::AllZero(), bitsvBE);
        }
        else // if (len < 48)
        {
            msg01 = U32x8(ptr).SwapEndian(); ptr += 8;
            const auto msg2 = Calc::Load128With80BE(ptr, len - 32); ptr += 4;
            msg23 = U32x8(msg2, bitsvBE);
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
        const U32x8 adderLH(SHA256RoundAdders[Round]);
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
        const U32x8 adderLH(SHA256RoundAdders[Round]);
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
        RegistFuncVars(MiscIntrins, Hex2Str, SIMDSSSE3, SIMD128, NAIVE);
        RegistFuncVars(MiscIntrins, PauseCycles, WAITPKG, SSE2, COMPILER);
        return ret;
    }();
    return list;
}
MiscIntrins::MiscIntrins(common::span<const VarItem> requests) noexcept { Init(requests); }
MiscIntrins::~MiscIntrins() {}
bool MiscIntrins::IsComplete() const noexcept
{
    return LeadZero32 && LeadZero64 && TailZero32 && TailZero64 && PopCount32 && PopCount64 && Hex2Str && PauseCycles;
}

const MiscIntrins MiscIntrin;


common::span<const DigestFuncs::PathInfo> DigestFuncs::GetSupportMap() noexcept
{
    static auto list = []()
    {
        std::vector<PathInfo> ret;
        RegistFuncVars(DigestFuncs, Sha256, SHANIAVX2, SHA2, SHANI, NAIVE);
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


namespace spinlock
{

template<typename FL, typename FF> 
forceinline void WaitFramework(FL&& lock, FF&& fix)
{
    for (uint32_t i = 0; i < 16; ++i)
    {
        IF_LIKELY(lock()) return;
        fix();
        COMMON_PAUSE();
    }
    while (true)
    {
        uint32_t delays[2] = { 512, 256 };
        for (uint32_t i = 0; i < 8; ++i)
        {
            IF_LIKELY(lock()) return;
            fix();
            MiscIntrin.Pause(delays[0]);
            delays[0] <<= 1;
            IF_LIKELY(lock()) return;
            fix();
            MiscIntrin.Pause(delays[1]);
            delays[1] <<= 1;
        }
        std::this_thread::yield();
    }
}

void SpinLocker::Lock() noexcept
{
    WaitFramework([&]() { return !Flag.test_and_set(); }, []() {});
}

void PreferSpinLock::LockWeak() noexcept
{
    uint32_t expected = Flag.load() & 0x0000ffff; // assume no strong
    WaitFramework([&]() { return Flag.load() == expected && Flag.compare_exchange_strong(expected, expected + 1); },
        [&]() { expected &= 0x0000ffff; });
}

void PreferSpinLock::LockStrong() noexcept
{
    Flag.fetch_add(0x00010000);
    // loop until no weak
    WaitFramework([&]() { return (Flag.load() & 0x0000ffff) == 0; }, []() {});
}

void WRSpinLock::LockRead() noexcept
{
    uint32_t expected = Flag.load() & 0x7fffffffu; // assume no writer
    WaitFramework([&]() { return Flag.load() == expected && Flag.compare_exchange_weak(expected, expected + 1); },
        [&]() { expected &= 0x7fffffffu; });
}

void WRSpinLock::LockWrite() noexcept
{
    uint32_t expected = Flag.load() & 0x7fffffffu;
    // assume no other writer
    WaitFramework([&]() { return Flag.load() == expected && Flag.compare_exchange_weak(expected, expected + 0x80000000u); },
        [&]() { expected &= 0x7fffffffu; });
    // loop until no reader
    WaitFramework([&]() { return (Flag.load() & 0x7fffffffu) == 0; }, []() {});
}

void RWSpinLock::LockRead() noexcept
{
    Flag++;
    // loop until no writer
    WaitFramework([&]() { return (Flag.load() & 0x80000000u) == 0; }, []() {});
}

void RWSpinLock::LockWrite() noexcept
{
    uint32_t expected = 0; // assume no other locker
    WaitFramework([&]() { return Flag.load() == expected && Flag.compare_exchange_weak(expected, 0x80000000u); },
        [&]() { expected = 0; });
}

}

}