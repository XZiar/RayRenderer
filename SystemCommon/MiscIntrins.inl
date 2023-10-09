
#include "FastPathCategory.h"
#include "MiscIntrins.h"
#include "common/simd/SIMD.hpp"
#include "common/SpinLock.hpp"

#include "3rdParty/digestpp/algorithm/sha2.hpp"

#include <bit>

#define LeadZero32Info  (uint32_t)(const uint32_t num)
#define LeadZero64Info  (uint32_t)(const uint64_t num)
#define TailZero32Info  (uint32_t)(const uint32_t num)
#define TailZero64Info  (uint32_t)(const uint64_t num)
#define PopCount32Info  (uint32_t)(const uint32_t num)
#define PopCount64Info  (uint32_t)(const uint64_t num)
#define PopCountsInfo   (uint32_t)(const std::byte* data, size_t count)
#define Hex2StrInfo     (::std::string)(const uint8_t* data, size_t size, bool isCapital)
#define PauseCyclesInfo (bool)(uint32_t cycles)
#define Sha256Info      (::std::array<std::byte, 32>)(const std::byte* data, const size_t size)


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


namespace
{
using namespace COMMON_SIMD_NAMESPACE;
using ::common::MiscIntrins;
using ::common::DigestFuncs;


DEFINE_FASTPATHS(MiscIntrins,
    LeadZero32, LeadZero64, TailZero32, TailZero64, PopCount32, PopCount64, PopCounts, Hex2Str, PauseCycles)


DEFINE_FASTPATHS(DigestFuncs, Sha256)


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


template<typename F64, typename F32>
static forceinline uint32_t PopCountsLoop(F64&& func64, F32&& func32, const std::byte* data, size_t count)
{
    uint32_t result = 0;
    while (count >= 32)
    {
        const auto cnt0 = static_cast<uint32_t>(func64(reinterpret_cast<const uint64_t*>(data)[0]));
        const auto cnt1 = static_cast<uint32_t>(func64(reinterpret_cast<const uint64_t*>(data)[1]));
        const auto cnt2 = static_cast<uint32_t>(func64(reinterpret_cast<const uint64_t*>(data)[2]));
        const auto cnt3 = static_cast<uint32_t>(func64(reinterpret_cast<const uint64_t*>(data)[3]));
        result += cnt0 + cnt1 + cnt2 + cnt3;
        data += 32; count -= 32;
    }
    while (count >= 8)
    {
        result += static_cast<uint32_t>(func64(reinterpret_cast<const uint64_t*>(data)[0]));
        data += 8; count -= 8;
    }
    if (count >= 4)
    {
        result += static_cast<uint32_t>(func32(reinterpret_cast<const uint32_t*>(data)[0]));
        data += 4; count -= 4;
    }
    switch (count)
    {
    case 3: result += static_cast<uint32_t>(func32(static_cast<uint32_t>(*data))); data++;
        [[fallthrough]];
    case 2: result += static_cast<uint32_t>(func32(static_cast<uint32_t>(*data))); data++;
        [[fallthrough]];
    case 1: result += static_cast<uint32_t>(func32(static_cast<uint32_t>(*data))); data++;
        [[fallthrough]];
    default:
        break;
    }
    return result;
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

forceinline uint32_t PopCount_COMPILER_32(uint32_t num)
{
    return __builtin_popcount(num);
}
forceinline uint32_t PopCount_COMPILER_64(uint64_t num)
{
    return __builtin_popcountll(num);
}
DEFINE_FASTPATH_METHOD(PopCount32, COMPILER)
{
    return PopCount_COMPILER_32(num);
}
DEFINE_FASTPATH_METHOD(PopCount64, COMPILER)
{
    return PopCount_COMPILER_64(num);
}
DEFINE_FASTPATH_METHOD(PopCounts, COMPILER)
{
    return PopCountsLoop(PopCount_COMPILER_64, PopCount_COMPILER_32, data, count);
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

forceinline uint32_t PopCount_NAIVE_32(uint32_t num)
{
    auto tmp = num - ((num >> 1) & 0x55555555u);
    tmp = (tmp & 0x33333333u) + ((tmp >> 2) & 0x33333333u);
    tmp = (tmp + (tmp >> 4)) & 0x0f0f0f0fu;
    return (tmp * 0x01010101u) >> 24;
}
forceinline uint32_t PopCount_NAIVE_64(uint64_t num)
{
    auto tmp = num - ((num >> 1) & 0x5555555555555555u);
    tmp = (tmp & 0x3333333333333333u) + ((tmp >> 2) & 0x3333333333333333u);
    tmp = (tmp + (tmp >> 4)) & 0x0f0f0f0f0f0f0f0fu;
    return (tmp * 0x0101010101010101u) >> 56;
}

DEFINE_FASTPATH_METHOD(PopCount32, NAIVE)
{
    return PopCount_NAIVE_32(num);
}
DEFINE_FASTPATH_METHOD(PopCount64, NAIVE)
{
    return PopCount_NAIVE_64(num);
}
DEFINE_FASTPATH_METHOD(PopCounts, NAIVE)
{
    return PopCountsLoop(PopCount_NAIVE_64, PopCount_NAIVE_32, data, count);
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

forceinline uint32_t PopCount_POPCNT_32(uint32_t num) noexcept
{
    return _mm_popcnt_u32(num);
}
forceinline uint32_t PopCount_POPCNT_64(uint64_t num) noexcept
{
#   if COMMON_OSBIT == 64
    return static_cast<uint32_t>(_mm_popcnt_u64(num));
#   else
    const auto lo = static_cast<uint32_t>(num), hi = static_cast<uint32_t>(num >> 32);
    return _mm_popcnt_u32(lo) + _mm_popcnt_u32(hi);
#   endif
}
DEFINE_FASTPATH_METHOD(PopCount32, POPCNT)
{
    return PopCount_POPCNT_32(num);
}
DEFINE_FASTPATH_METHOD(PopCount64, POPCNT)
{
    return PopCount_POPCNT_64(num);
}
DEFINE_FASTPATH_METHOD(PopCounts, POPCNT)
{
    return PopCountsLoop(PopCount_POPCNT_64, PopCount_POPCNT_32, data, count);
}

#endif


#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 200

DEFINE_FASTPATH_METHOD(PopCounts, AVX2)
{
    uint32_t ret = 0;
    if (count >= 32)
    {
        // 0000=0, 0001=1, 0010=1, 0011=2, 0100=1, 0101=2, 0110=2, 0111=3, 1000=1, 1001=2, 1010=2, 1011=3, 1100=2, 1101=3, 1110=3, 1111=4
        const U8x32 LUT(0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4);
        const U8x32 MaskLo(0x0f);
        auto accum = U64x4::AllZero();
        constexpr auto Lookup = [](const uint8_t* ptr, const U8x32& lut, const U8x32& mask) -> U8x32
        {
            U8x32 dat(ptr);
            const auto partLo = _mm256_shuffle_epi8(lut.Data, dat.And(mask).Data);
            const auto partHi = _mm256_shuffle_epi8(lut.Data, dat.As<U16x16>().ShiftRightLogic<4>().As<U8x32>().And(mask).Data);
            return _mm256_add_epi8(partLo, partHi);
        };
        while (count >= 256)
        {
            const auto mid0 = Lookup(reinterpret_cast<const uint8_t*>(data) +   0, LUT, MaskLo);
            const auto mid1 = Lookup(reinterpret_cast<const uint8_t*>(data) +  32, LUT, MaskLo);
            const auto mid2 = Lookup(reinterpret_cast<const uint8_t*>(data) +  64, LUT, MaskLo);
            const auto mid3 = Lookup(reinterpret_cast<const uint8_t*>(data) +  96, LUT, MaskLo);
            const auto mid4 = Lookup(reinterpret_cast<const uint8_t*>(data) + 128, LUT, MaskLo);
            const auto mid5 = Lookup(reinterpret_cast<const uint8_t*>(data) + 160, LUT, MaskLo);
            const auto mid6 = Lookup(reinterpret_cast<const uint8_t*>(data) + 192, LUT, MaskLo);
            const auto mid7 = Lookup(reinterpret_cast<const uint8_t*>(data) + 224, LUT, MaskLo);
            const auto tmp = ((mid0 + mid1) + (mid2 + mid3)) + ((mid4 + mid5) + (mid6 + mid7));
            accum = accum + U64x4(_mm256_sad_epu8(tmp, _mm256_setzero_si256()));
            data += 256; count -= 256;
        }
        while (count >= 32)
        {
            const auto mid = Lookup(reinterpret_cast<const uint8_t*>(data) + 0, LUT, MaskLo);
            accum = accum + U64x4(_mm256_sad_epu8(mid, _mm256_setzero_si256()));
            data += 32; count -= 32;
        }
        // with in 32bit range
        const auto ret0 = _mm256_extract_epi32(accum.Data, 0);
        const auto ret1 = _mm256_extract_epi32(accum.Data, 2);
        const auto ret2 = _mm256_extract_epi32(accum.Data, 4);
        const auto ret3 = _mm256_extract_epi32(accum.Data, 6);
        ret = ret0 + ret1 + ret2 + ret3;
    }
    CM_ASSUME(count < 32);
    const auto trailing = count > 0 ? PopCountsLoop(PopCount_POPCNT_64, PopCount_POPCNT_32, data, count) : 0u;
    return trailing + ret;
}

#endif

#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 320 && (defined(__AVX512VPOPCNTDQ__) || COMMON_COMPILER_MSVC)
#pragma message("Compiling MiscIntrins with AVX512-VPOPCNTDQ")

DEFINE_FASTPATH_METHOD(PopCounts, AVX512VPOPCNT)
{
    uint32_t ret = 0;
    if (count >= 32)
    {
        __m512i accum = _mm512_setzero_si512();
        while (count >= 512)
        {
            const auto mid0 = _mm512_popcnt_epi64(_mm512_loadu_epi64(data +   0));
            const auto mid1 = _mm512_popcnt_epi64(_mm512_loadu_epi64(data +  64));
            const auto mid2 = _mm512_popcnt_epi64(_mm512_loadu_epi64(data + 128));
            const auto mid3 = _mm512_popcnt_epi64(_mm512_loadu_epi64(data + 192));
            const auto mid4 = _mm512_popcnt_epi64(_mm512_loadu_epi64(data + 256));
            const auto mid5 = _mm512_popcnt_epi64(_mm512_loadu_epi64(data + 320));
            const auto mid6 = _mm512_popcnt_epi64(_mm512_loadu_epi64(data + 384));
            const auto mid7 = _mm512_popcnt_epi64(_mm512_loadu_epi64(data + 448));
            accum = _mm512_add_epi64(_mm512_add_epi64(
                _mm512_add_epi64(_mm512_add_epi64(mid0, mid1), _mm512_add_epi64(mid2, mid3)),
                _mm512_add_epi64(_mm512_add_epi64(mid4, mid5), _mm512_add_epi64(mid6, mid7))), accum);
            data += 512; count -= 512;
        }
        while (count >= 64)
        {
            const auto mid = _mm512_popcnt_epi64(_mm512_loadu_epi64(data));
            accum = _mm512_add_epi64(accum, mid);
            data += 64; count -= 64;
        }
        auto accum2 = _mm512_cvtepi64_epi32(accum);
        if (count >= 32)
        {
            const auto mid = _mm256_popcnt_epi32(_mm256_loadu_epi32(data));
            accum2 = _mm256_add_epi32(accum2, mid);
            data += 32; count -= 32;
        }
        auto accum3 = _mm256_hadd_epi32(accum2, accum2);
        const auto ret0 = _mm256_extract_epi32(accum3, 0);
        const auto ret1 = _mm256_extract_epi32(accum3, 1);
        const auto ret2 = _mm256_extract_epi32(accum3, 4);
        const auto ret3 = _mm256_extract_epi32(accum3, 5);
        ret = ret0 + ret1 + ret2 + ret3;
    }
    CM_ASSUME(count < 32);
    const auto trailing = count > 0 ? PopCountsLoop(PopCount_POPCNT_64, PopCount_POPCNT_32, data, count) : 0u;
    return trailing + ret;
}

#endif

#if COMMON_ARCH_ARM && COMMON_SIMD_LV >= 30

DEFINE_FASTPATH_METHOD(PopCounts, SIMD128)
{
    uint32_t ret = 0;
    constexpr auto Sum16x8x8 = [](const uint8_t* ptr) -> U8x16
    {
        const auto dat0123 = vld1q_u8_x4(ptr);
        const auto dat4567 = vld1q_u8_x4(ptr + 64);
        const U8x16 mid0 = vcntq_u8(dat0123.val[0]);
        const U8x16 mid1 = vcntq_u8(dat0123.val[1]);
        const U8x16 mid2 = vcntq_u8(dat0123.val[2]);
        const U8x16 mid3 = vcntq_u8(dat0123.val[3]);
        const U8x16 mid4 = vcntq_u8(dat4567.val[0]);
        const U8x16 mid5 = vcntq_u8(dat4567.val[1]);
        const U8x16 mid6 = vcntq_u8(dat4567.val[2]);
        const U8x16 mid7 = vcntq_u8(dat4567.val[3]);
        return ((mid0 + mid1) + (mid2 + mid3)) + ((mid4 + mid5) + (mid6 + mid7));
    };
    if (count >= 8)
    {
#   if COMMON_SIMD_LV >= 200
#   else
        auto accum = U32x4::AllZero();
        auto accum2 = U8x16::AllZero();
#   endif

        while (count >= 512)
        {
            const auto part0 = Sum16x8x8(reinterpret_cast<const uint8_t*>(data) + 0);
            const auto part1 = Sum16x8x8(reinterpret_cast<const uint8_t*>(data) + 128);
            const auto part2 = Sum16x8x8(reinterpret_cast<const uint8_t*>(data) + 256);
            const auto part3 = Sum16x8x8(reinterpret_cast<const uint8_t*>(data) + 384);
            const auto part = ((part0 + part1) + (part2 + part3));
#   if COMMON_SIMD_LV >= 200
            ret += vaddlvq_u8(part);
#   else
            const U32x4 part32x4 = vpaddlq_u16(vpaddlq_u8(part));
            accum = accum + part32x4;
#   endif
            data += 512; count -= 512;
        }

#   if COMMON_SIMD_LV >= 200
#       define PartialAdd(var) ret += vaddlvq_u8(var)
#   else
#       define PartialAdd(var) accum2 = accum2 + var
#   endif
        switch (count / 128)
        {
#   define Iter_Sum16x8x8 { const auto part = Sum16x8x8(reinterpret_cast<const uint8_t*>(data)); PartialAdd(part); data += 128; count -= 128; } [[fallthrough]]
        case 3: Iter_Sum16x8x8;
        case 2: Iter_Sum16x8x8;
        case 1: Iter_Sum16x8x8;
#   undef Iter_Sum16x8x8
        default:
            break;
        }
        switch (count / 16)
        {
#   define Iter_Sum16x8 { const auto part = vcntq_u8(vld1q_u8(reinterpret_cast<const uint8_t*>(data))); PartialAdd(part); data += 16; count -= 16; }
        case 7: Iter_Sum16x8 [[fallthrough]];
        case 6: Iter_Sum16x8 [[fallthrough]];
        case 5: Iter_Sum16x8 [[fallthrough]];
        case 4: Iter_Sum16x8 [[fallthrough]];
        case 3: Iter_Sum16x8 [[fallthrough]];
        case 2: Iter_Sum16x8 [[fallthrough]];
        case 1: Iter_Sum16x8
#   undef Iter_Sum16x8
#   if COMMON_SIMD_LV >= 200
#   else
        {
            const U32x4 part32x4 = vpaddlq_u16(vpaddlq_u8(accum2));
            accum = accum + part32x4;
            const auto tmp = vpadd_u32(vget_low_u32(accum), vget_high_u32(accum));
            ret += vget_lane_u32(tmp, 0) + vget_lane_u32(tmp, 1);
        }
#   endif
            [[fallthrough]];
        default:
            break;
        }
#   undef PartialAdd

        if (count >= 8)
        {
            const auto part = vcnt_u8(vld1_u8(reinterpret_cast<const uint8_t*>(data))); 
#   if COMMON_SIMD_LV >= 200
            ret += vaddlv_u8(part);
#   else
            const auto tmp = vpaddl_u16(vpaddl_u8(part));
            ret += vget_lane_u32(tmp, 0) + vget_lane_u32(tmp, 1);
#   endif
            data += 8; count -= 8;
        }
    }
    CM_ASSUME(count < 8);
    const auto trailing = count > 0 ? PopCountsLoop(PopCount_NAIVE_64, PopCount_NAIVE_32, data, count) : 0u;
    return trailing + ret;
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
#   define ALGO SSSE3
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


alignas(64) constexpr uint32_t SHA256RoundAdders[][4] =
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
    forceinline Sha256State(U32x4 state0, U32x4 state1) : State0(state0), State1(state1) { }
    forceinline static U32x4 Load128With80BESafe(const uint32_t* data, const size_t len) noexcept
    {
        // Expects len < 16
        if (len == 0)
            return U32x4::LoadLo(0x80000000u);
        const auto ptr = reinterpret_cast<const uint8_t*>(data);
        alignas(16) uint8_t tmp[16] = { 0 };
        tmp[len] = 0x80;
        switch (len)
        {
        case 15:
            *(uint64_t*)(tmp +  0) = *(const uint64_t*)(ptr +  0);
            *(uint32_t*)(tmp +  8) = *(const uint32_t*)(ptr +  8);
            *(uint16_t*)(tmp + 12) = *(const uint16_t*)(ptr + 12);
            tmp[14] = ptr[14];
            break;
        case 14:
            *(uint64_t*)(tmp +  0) = *(const uint64_t*)(ptr +  0);
            *(uint32_t*)(tmp +  8) = *(const uint32_t*)(ptr +  8);
            *(uint16_t*)(tmp + 12) = *(const uint16_t*)(ptr + 12);
            break;
        case 13:
            *(uint64_t*)(tmp + 0) = *(const uint64_t*)(ptr + 0);
            *(uint32_t*)(tmp + 8) = *(const uint32_t*)(ptr + 8);
            tmp[12] = ptr[12];
            break;
        case 12:
            *(uint64_t*)(tmp + 0) = *(const uint64_t*)(ptr + 0);
            *(uint32_t*)(tmp + 8) = *(const uint32_t*)(ptr + 8);
            break;
        case 11:
            *(uint64_t*)(tmp + 0) = *(const uint64_t*)(ptr + 0);
            *(uint16_t*)(tmp + 8) = *(const uint16_t*)(ptr + 8);
            tmp[10] = ptr[10];
            break;
        case 10:
            *(uint64_t*)(tmp + 0) = *(const uint64_t*)(ptr + 0);
            *(uint16_t*)(tmp + 8) = *(const uint16_t*)(ptr + 8);
            break;
        case 9:
            *(uint64_t*)(tmp + 0) = *(const uint64_t*)(ptr + 0);
            tmp[8] = ptr[8];
            break;
        case 8:
            *(uint64_t*)(tmp + 0) = *(const uint64_t*)(ptr + 0);
            break;
        case 7:
            *(uint32_t*)(tmp + 0) = *(const uint32_t*)(ptr + 0);
            *(uint16_t*)(tmp + 4) = *(const uint16_t*)(ptr + 4);
            tmp[6] = ptr[6];
            break;
        case 6:
            *(uint32_t*)(tmp + 0) = *(const uint32_t*)(ptr + 0);
            *(uint16_t*)(tmp + 4) = *(const uint16_t*)(ptr + 4);
            break;
        case 5:
            *(uint32_t*)(tmp + 0) = *(const uint32_t*)(ptr + 0);
            tmp[4] = ptr[4];
            break;
        case 4:
            *(uint32_t*)(tmp + 0) = *(const uint32_t*)(ptr + 0);
            break;
        case 3:
            *(uint16_t*)(tmp + 0) = *(const uint16_t*)(ptr + 0);
            tmp[2] = ptr[2];
            break;
        case 2:
            *(uint16_t*)(tmp + 0) = *(const uint16_t*)(ptr + 0);
            break;
        default:
            tmp[0] = ptr[0];
            break;
        }
        return U8x16(tmp).As<U32x4>().SwapEndian();
    }
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
              auto msg3 = Calc::Load128With80BE(true, ptr, len % 16);
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
            msg0 = Calc::Load128With80BE(size >= 16, ptr, len - 0); ptr += 4;
            msg1 = U32x4::AllZero();
            msg2 = U32x4::AllZero();
            msg3 = bitsvBE;
        }
        else if (len < 32)
        {
            msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
            msg1 = Calc::Load128With80BE(true, ptr, len - 16); ptr += 4;
            msg2 = U32x4::AllZero();
            msg3 = bitsvBE;
        }
        else // if (len < 48)
        {
            msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
            msg1 = U32x4(ptr).SwapEndian(); ptr += 4;
            msg2 = Calc::Load128With80BE(true, ptr, len - 32); ptr += 4;
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
    forceinline static U32x4 Load128With80BE(bool hasMoreThan16, const uint32_t* data, const size_t len) noexcept
    {
        IF_LIKELY (len == 0)
            return U32x4::LoadLo(0x80000000u);
        IF_UNLIKELY(!hasMoreThan16)
            return Load128With80BESafe(data, len);
        CM_ASSUME(len > 0 && len < 16);
        const auto ptr = reinterpret_cast<const uint8_t*>(data) - (16 - len);
        const auto rawData = U8x16(ptr);
        const U8x16 tail = U8x16::LoadLo(static_cast<uint8_t>(0x80));
        switch (len)
        {
        case 15: return U32x4(_mm_alignr_epi8(tail, rawData, 15)).SwapEndian();
        case 14: return U32x4(_mm_alignr_epi8(tail, rawData, 14)).SwapEndian();
        case 13: return U32x4(_mm_alignr_epi8(tail, rawData, 13)).SwapEndian();
        case 12: return U32x4(_mm_alignr_epi8(tail, rawData, 12)).SwapEndian();
        case 11: return U32x4(_mm_alignr_epi8(tail, rawData, 11)).SwapEndian();
        case 10: return U32x4(_mm_alignr_epi8(tail, rawData, 10)).SwapEndian();
        case  9: return U32x4(_mm_alignr_epi8(tail, rawData,  9)).SwapEndian();
        case  8: return U32x4(_mm_alignr_epi8(tail, rawData,  8)).SwapEndian();
        case  7: return U32x4(_mm_alignr_epi8(tail, rawData,  7)).SwapEndian();
        case  6: return U32x4(_mm_alignr_epi8(tail, rawData,  6)).SwapEndian();
        case  5: return U32x4(_mm_alignr_epi8(tail, rawData,  5)).SwapEndian();
        case  4: return U32x4(_mm_alignr_epi8(tail, rawData,  4)).SwapEndian();
        case  3: return U32x4(_mm_alignr_epi8(tail, rawData,  3)).SwapEndian();
        case  2: return U32x4(_mm_alignr_epi8(tail, rawData,  2)).SwapEndian();
        default: return U32x4(_mm_alignr_epi8(tail, rawData,  1)).SwapEndian();
        }
    }

    forceinline Sha256Round_SHANI() noexcept : Sha256State(
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
    forceinline static U32x4 Load128With80BE(bool, const uint32_t* data, const size_t len) noexcept
    {
        return Load128With80BESafe(data, len);
    }

    forceinline Sha256Round_SHA2() noexcept : Sha256State(
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

    U32x4 msg0, msg1, msg2, msg3;
    ///* Rounds 0-3 */ calc.template Calc< 0>(msg3, msg0, msg1);
    ///* Rounds 4-7 */ calc.template Calc< 1>(msg0, msg1, msg2);
    ///* Rounds 8-11 */  calc.template Calc< 2>(msg1, msg2, msg3);
    ///* Rounds 12-15 */ calc.template Calc< 3>(msg2, msg3, msg0);
    calc.Calc0123(msg01, msg23, msg0, msg1, msg2, msg3);

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
    const auto bitsvBE128 = bitsv.As<U32x4>().Shuffle<3, 3, 1, 0>();
    //const auto bitsvBE = U32x8(U32x4::AllZero(), bitsv.As<U32x4>().Shuffle<3, 3, 1, 0>());
    const auto bitsvBE = _mm256_inserti128_si256(_mm256_setzero_si256(), bitsvBE128, 1);
    if (len >= 48)
    {
        const auto msg01 = U32x8(ptr).SwapEndian(); ptr += 8;
        const auto msg2  = U32x4(ptr).SwapEndian(); ptr += 4;
        const auto msg3  = Calc::Load128With80BE(true, ptr, len % 16);
        U32x8 msg23 = _mm256_inserti128_si256(_mm256_castsi128_si256(msg2), msg3, 1);
        if (len < 56)
            msg23 |= bitsvBE;
        Sha256Block256(calc, msg01, msg23);
        if (len >= 56)
            Sha256Block256(calc, U32x8::AllZero(), bitsvBE);// _mm256_inserti128_si256(_mm256_setzero_si256(), bitsvBE, 1));
    }
    else
    {
        U32x8 msg01, msg23;
        if (len < 16)
        {
            const auto msg0 = Calc::Load128With80BE(size >= 16, ptr, len - 0); ptr += 4;
            msg01 = U32x8::LoadLoLane(msg0);
            msg23 = bitsvBE;// U32x8(U32x4::AllZero(), bitsvBE);
        }
        else if (len < 32)
        {
            const auto msg0 = U32x4(ptr).SwapEndian(); ptr += 4;
            const auto msg1 = Calc::Load128With80BE(true, ptr, len - 16); ptr += 4;
            msg01 = U32x8(msg0, msg1);
            msg23 = bitsvBE;// U32x8(U32x4::AllZero(), bitsvBE);
        }
        else // if (len < 48)
        {
            msg01 = U32x8(ptr).SwapEndian(); ptr += 8;
            const auto msg2 = Calc::Load128With80BE(true, ptr, len - 32); ptr += 4;
            msg23 = _mm256_inserti128_si256(bitsvBE, msg2, 0);// U32x8(msg2, bitsvBE);
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
    forceinline void VECCALL Calc0123(const U32x8 msg01, const U32x8& msg23, U32x4& msg0, U32x4& msg1, U32x4& msg2, U32x4& msg3) noexcept // msgs are BE
    {
        static_assert(SHA256RoundProcControl[0][0] == false && SHA256RoundProcControl[0][1] == false &&
            SHA256RoundProcControl[0 + 1][0] == true && SHA256RoundProcControl[0 + 1][0 + 1] == false); // FF, TF
        const U32x8 adderLH01(SHA256RoundAdders[0]);
        const auto msgAddLH01 = msg01 + adderLH01;
        const auto msgShufLH01 = msgAddLH01.ShuffleLane<2, 3, 0, 0>(); // _mm256_shuffle_epi32(msgAddLH, 0x0E);
        State1 = _mm_sha256rnds2_epu32(State1, State0, msgAddLH01.GetLoLane());
        State0 = _mm_sha256rnds2_epu32(State0, State1, msgShufLH01.GetLoLane());
        State1 = _mm_sha256rnds2_epu32(State1, State0, msgAddLH01.GetHiLane());
        State0 = _mm_sha256rnds2_epu32(State0, State1, msgShufLH01.GetHiLane());
        msg1 = msg01.GetHiLane();
        msg0 = _mm_sha256msg1_epu32(msg01.GetLoLane(), msg1);

        static_assert(SHA256RoundProcControl[2][0] == true && SHA256RoundProcControl[2][1] == false &&
            SHA256RoundProcControl[2 + 1][0] == true && SHA256RoundProcControl[2 + 1][0 + 1] == true); // TF, TT
        const U32x8 adderLH23(SHA256RoundAdders[2]);
        const auto msgAddLH23 = msg23 + adderLH23;
        const auto msgShufLH23 = msgAddLH23.ShuffleLane<2, 3, 0, 0>(); // _mm256_shuffle_epi32(msgAddLH, 0x0E);
        State1 = _mm_sha256rnds2_epu32(State1, State0, msgAddLH23.GetLoLane());
        State0 = _mm_sha256rnds2_epu32(State0, State1, msgShufLH23.GetLoLane());
        State1 = _mm_sha256rnds2_epu32(State1, State0, msgAddLH23.GetHiLane());
        State0 = _mm_sha256rnds2_epu32(State0, State1, msgShufLH23.GetHiLane());
        msg2 = msg23.GetLoLane();
        msg3 = msg23.GetHiLane();
        const U32x4 tmp = _mm_alignr_epi8(msg3, msg2, 4);
        msg1 = _mm_sha256msg1_epu32(msg1, msg2);
        msg0 = _mm_sha256msg2_epu32(msg0 + tmp, msg3);
        msg2 = _mm_sha256msg1_epu32(msg2, msg3);
    }
};
DEFINE_FASTPATH_METHOD(Sha256, SHANIAVX2)
{
    return Sha256Main256<Sha256Round_SHANIAVX2>(data, size);
}
# endif
#endif


#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 320 && (COMMON_COMPILER_MSVC || defined(__SHA__))
// From http://software.intel.com/en-us/articles/intel-sha-extensions written by Sean Gulley.
template<typename Calc>
forceinline static void VECCALL Sha256Block512(Calc& calc, __m512i msg0123) noexcept // msgs are BE
{
    /* Save current state */
    const auto abef_save = calc.State0;
    const auto cdgh_save = calc.State1;

    U32x4 msg0, msg1, msg2, msg3;
    ///* Rounds 0-3 */ calc.template Calc< 0>(msg3, msg0, msg1);
    ///* Rounds 4-7 */ calc.template Calc< 1>(msg0, msg1, msg2);
    ///* Rounds 8-11 */  calc.template Calc< 2>(msg1, msg2, msg3);
    ///* Rounds 12-15 */ calc.template Calc< 3>(msg2, msg3, msg0);
    calc.Calc0123(msg0123, msg0, msg1, msg2, msg3);
    //calc.Calc0123(_mm512_extracti64x4_epi64(msg0123, 0), _mm512_extracti64x4_epi64(msg0123, 1), msg0, msg1, msg2, msg3);
    
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
inline std::array<std::byte, 32> Sha256Main512(const std::byte* data, const size_t size) noexcept
{
    static_assert(std::is_base_of_v<Sha256State, Calc>);
    Calc calc;

    size_t len = size;
    const uint32_t* __restrict ptr = reinterpret_cast<const uint32_t*>(data);
    const auto SwapMask = _mm512_set_epi8(
        12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3,
        12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3,
        12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3,
        12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);
    
    while (len >= 64)
    {
        const auto msg0123 = _mm512_shuffle_epi8(_mm512_loadu_si512(ptr), SwapMask); ptr += 16;
        Sha256Block512(calc, msg0123);
        len -= 64;
    }
    {
        const auto bitsTotal = static_cast<uint64_t>(size) * 8;
        const auto bitsVal = (bitsTotal >> 32) | (bitsTotal << 32);
        const auto bitsMask = _cvtu32_mask8(0x80);

        const auto endSigMask = static_cast<uint64_t>(0x1u) << static_cast<uint8_t>(len);
        const auto keepMask = endSigMask - 1u;
        const auto mark80 = _mm512_maskz_set1_epi8(_cvtu64_mask64(endSigMask), static_cast<char>(0x80));
        const auto val = _mm512_mask_loadu_epi8(mark80, _cvtu64_mask64(keepMask), ptr);
        const auto msgBE = _mm512_shuffle_epi8(val, SwapMask);

        __m512i msg0123 = msgBE;
        if (len < 56) // have enough space
            msg0123 = _mm512_mask_set1_epi64(msgBE, bitsMask, bitsVal);
        Sha256Block512(calc, msg0123);
        if (len >= 56)
            Sha256Block512(calc, _mm512_maskz_set1_epi64(bitsMask, bitsVal));
    }
    return calc.Output();
}
struct Sha256Round_SHANIAVX512BW : public Sha256Round_SHANIAVX2
{
    forceinline static U32x4 Load128With80BE(const uint32_t* data, const size_t len) noexcept
    {
        // Expects len < 16
        if (len == 0)
            return U32x4::LoadLo(0x80000000u);
        const auto endSigMask = 0x1u << static_cast<uint8_t>(len);
        const auto keepMask = endSigMask - 1;
        const auto mark80 = _mm_maskz_set1_epi8(_cvtu32_mask16(endSigMask), static_cast<char>(0x80));
        const U32x4 val = _mm_mask_loadu_epi8(mark80, _cvtu32_mask16(keepMask), data);
        const auto ret = val.SwapEndian();
        return ret;
    }
    // From http://software.intel.com/en-us/articles/intel-sha-extensions written by Sean Gulley.
    // Modifiled from code previously on https://github.com/mitls/hacl-star/tree/master/experimental/hash with BSD license.
    forceinline void VECCALL Calc0123(__m512i msg0123, U32x4& msg0, U32x4& msg1, U32x4& msg2, U32x4& msg3) noexcept // msgs are BE
    {
        static_assert(SHA256RoundProcControl[0][0] == false && SHA256RoundProcControl[0][1] == false &&
            SHA256RoundProcControl[0 + 1][0] == true && SHA256RoundProcControl[0 + 1][0 + 1] == false); // FF, TF
        static_assert(SHA256RoundProcControl[2][0] == true && SHA256RoundProcControl[2][1] == false &&
            SHA256RoundProcControl[2 + 1][0] == true && SHA256RoundProcControl[2 + 1][0 + 1] == true); // TF, TT
        const auto adderLH0123 = _mm512_load_si512(SHA256RoundAdders);
        const auto msgAddLH0123 = _mm512_add_epi32(msg0123, adderLH0123);
        const auto msgShufLH0123 = _mm512_shuffle_epi32(msgAddLH0123, (_MM_PERM_ENUM)0x0E);
        
        State1 = _mm_sha256rnds2_epu32(State1, State0, _mm512_extracti64x2_epi64(msgAddLH0123,  0));
        State0 = _mm_sha256rnds2_epu32(State0, State1, _mm512_extracti64x2_epi64(msgShufLH0123, 0));
        State1 = _mm_sha256rnds2_epu32(State1, State0, _mm512_extracti64x2_epi64(msgAddLH0123,  1));
        State0 = _mm_sha256rnds2_epu32(State0, State1, _mm512_extracti64x2_epi64(msgShufLH0123, 1));
        msg1 = _mm512_extracti64x2_epi64(msg0123, 1);
        msg0 = _mm_sha256msg1_epu32(_mm512_extracti64x2_epi64(msg0123, 0), msg1);

        State1 = _mm_sha256rnds2_epu32(State1, State0, _mm512_extracti64x2_epi64(msgAddLH0123,  2));
        State0 = _mm_sha256rnds2_epu32(State0, State1, _mm512_extracti64x2_epi64(msgShufLH0123, 2));
        State1 = _mm_sha256rnds2_epu32(State1, State0, _mm512_extracti64x2_epi64(msgAddLH0123,  3));
        State0 = _mm_sha256rnds2_epu32(State0, State1, _mm512_extracti64x2_epi64(msgShufLH0123, 3));
        msg2 = _mm512_extracti64x2_epi64(msg0123, 2);
        msg3 = _mm512_extracti64x2_epi64(msg0123, 3);
        const U32x4 tmp = _mm_alignr_epi8(msg3, msg2, 4);
        msg1 = _mm_sha256msg1_epu32(msg1, msg2);
        msg0 = _mm_sha256msg2_epu32(msg0 + tmp, msg3);
        msg2 = _mm_sha256msg1_epu32(msg2, msg3);
    }
};
DEFINE_FASTPATH_METHOD(Sha256, SHANIAVX512BW)
{
    return Sha256Main512<Sha256Round_SHANIAVX512BW>(data, size);
}
#endif


}
