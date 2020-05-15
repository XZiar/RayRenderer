#include "SystemCommonPch.h"
#include "MiscIntrins.h"
#include "common/SIMD.hpp"
#include "cpuid/libcpuid.h"

#pragma message("Compiling MiscIntrins with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace common
{
namespace intrin
{
struct LeadZero32 { using RetType = uint32_t; };
struct LeadZero64 { using RetType = uint32_t; };
struct TailZero32 { using RetType = uint32_t; };
struct TailZero64 { using RetType = uint32_t; };
struct PopCount32 { using RetType = uint32_t; };
struct PopCount64 { using RetType = uint32_t; };
struct ByteSwap16 { using RetType = uint16_t; };
struct ByteSwap32 { using RetType = uint32_t; };
struct ByteSwap64 { using RetType = uint64_t; };

struct NAIVE {};
struct OS {};
struct LZCNT {};
struct TZCNT {};
struct POPCNT {};
struct BMI1 {};
}
template<typename Intrin, typename Method>
constexpr bool CheckExists() noexcept 
{
    return false;
}
template<typename Intrin, typename Method, typename F>
constexpr void InjectFunc(F& func) noexcept
{
    func = nullptr;
}


#define DEFINE_INTRIN_METHOD(func, var, ...) \
template<> \
constexpr bool CheckExists<intrin::func, intrin::var>() noexcept \
{ return true; } \
static typename intrin::func::RetType func##_##var(__VA_ARGS__) noexcept; \
template<> \
constexpr void InjectFunc<intrin::func, intrin::var>(decltype(&func##_##var)& f) noexcept \
{ f = func##_##var; } \
static typename intrin::func::RetType func##_##var(__VA_ARGS__) noexcept \



#if COMPILER_MSVC

DEFINE_INTRIN_METHOD(LeadZero32, OS, const uint32_t num)
{
    unsigned long idx = 0;
    return _BitScanReverse(&idx, num) ? 31 - idx : 32;
}
DEFINE_INTRIN_METHOD(LeadZero64, OS, const uint64_t num)
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

DEFINE_INTRIN_METHOD(TailZero32, OS, const uint32_t num)
{
    unsigned long idx = 0;
    return _BitScanForward(&idx, num) ? idx : 32;
}
DEFINE_INTRIN_METHOD(TailZero64, OS, const uint64_t num)
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

#elif COMPILER_GCC || COMPILER_CLANG

DEFINE_INTRIN_METHOD(LeadZero32, OS, const uint32_t num)
{
    return num == 0 ? 32 : __builtin_clz(num);
}
DEFINE_INTRIN_METHOD(LeadZero64, OS, const uint64_t num)
{
    return num == 0 ? 64 : __builtin_clzll(num);
}

DEFINE_INTRIN_METHOD(TailZero32, OS, const uint32_t num)
{
    return num == 0 ? 32 : __builtin_ctz(num);
}
DEFINE_INTRIN_METHOD(TailZero64, OS, const uint64_t num)
{
    return num == 0 ? 64 : __builtin_ctzll(num);
}

DEFINE_INTRIN_METHOD(PopCount32, OS, const uint32_t num)
{
    return __builtin_popcount(num);
}
DEFINE_INTRIN_METHOD(PopCount64, OS, const uint64_t num)
{
    return __builtin_popcountll(num);
}

#endif


//DEFINE_INTRIN_METHOD(LeadZero32, NAIVE, const uint32_t num)
//{
//}
//DEFINE_INTRIN_METHOD(LeadZero64, NAIVE, const uint64_t num)
//{
//}

//DEFINE_INTRIN_METHOD(TailZero32, NAIVE, const uint32_t num)
//{
//}
//DEFINE_INTRIN_METHOD(TailZero64, NAIVE, const uint64_t num)
//{
//}

DEFINE_INTRIN_METHOD(PopCount32, NAIVE, const uint32_t num)
{
    auto tmp = num - ((num >> 1) & 0x55555555u);
    tmp = (tmp & 0x33333333u) + ((tmp >> 2) & 0x33333333u);
    tmp = (tmp + (tmp >> 4)) & 0x0f0f0f0fu;
    return (tmp * 0x01010101u) >> 24;
}
DEFINE_INTRIN_METHOD(PopCount64, NAIVE, const uint64_t num)
{
    auto tmp = num - ((num >> 1) & 0x5555555555555555u);
    tmp = (tmp & 0x3333333333333333u) + ((tmp >> 2) & 0x3333333333333333u);
    tmp = (tmp + (tmp >> 4)) & 0x0f0f0f0f0f0f0f0fu;
    return (tmp * 0x0101010101010101u) >> 56;
}


#if (COMPILER_MSVC/* && COMMON_SIMD_LV >= 200*/) || (!COMPILER_MSVC && (defined(__LZCNT__) || defined(__BMI__)))

DEFINE_INTRIN_METHOD(LeadZero32, LZCNT, const uint32_t num)
{
    return _lzcnt_u32(num);
}
DEFINE_INTRIN_METHOD(LeadZero64, LZCNT, const uint64_t num)
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


#if (COMPILER_MSVC/* && COMMON_SIMD_LV >= 200*/) || (!COMPILER_MSVC && defined(__BMI__))

DEFINE_INTRIN_METHOD(TailZero32, TZCNT, const uint32_t num)
{
    return _tzcnt_u32(num);
}
DEFINE_INTRIN_METHOD(TailZero64, TZCNT, const uint64_t num)
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


#if (COMPILER_MSVC/* && COMMON_SIMD_LV >= 42*/) || (!COMPILER_MSVC && defined(__POPCNT__))

DEFINE_INTRIN_METHOD(PopCount32, POPCNT, const uint32_t num)
{
    return _mm_popcnt_u32(num);
}
DEFINE_INTRIN_METHOD(PopCount64, POPCNT, const uint64_t num)
{
#   if COMMON_OSBIT == 64
    return static_cast<uint32_t>(_mm_popcnt_u64(num));
#   else
    const auto lo = static_cast<uint32_t>(num), hi = static_cast<uint32_t>(num >> 32);
    return _mm_popcnt_u32(lo) + _mm_popcnt_u32(hi);
#   endif
}

#endif


MiscIntrins::MiscIntrins() noexcept
{
    const auto& data = GetCPUInfo();

#define TestFeature(feat) (data.has_value() && data->flags[feat])
#define SetFunc(func, variant, test) do \
    { \
        if constexpr (CheckExists<intrin::func, intrin::variant>()) \
        { \
            if (func == nullptr && test) \
            { \
                InjectFunc<intrin::func, intrin::variant>(func); \
                VariantMap.emplace_back(STRINGIZE(func), STRINGIZE(variant)); \
            } \
        } \
    } while(0)
#define UseIfExist(func, variant, feat) SetFunc(func, variant, TestFeature(feat))
#define FallbackTo(func, variant) SetFunc(func, variant, true)

    UseIfExist(LeadZero32, LZCNT, CPU_FEATURE_BMI1);
    UseIfExist(LeadZero64, LZCNT, CPU_FEATURE_BMI1);
    UseIfExist(LeadZero32, LZCNT, CPU_FEATURE_ABM);
    UseIfExist(LeadZero64, LZCNT, CPU_FEATURE_ABM);
    FallbackTo(LeadZero32, OS);
    FallbackTo(LeadZero64, OS);

    UseIfExist(TailZero32, TZCNT, CPU_FEATURE_BMI1);
    UseIfExist(TailZero64, TZCNT, CPU_FEATURE_BMI1);
    FallbackTo(TailZero32, OS);
    FallbackTo(TailZero64, OS);

    UseIfExist(PopCount32, POPCNT, CPU_FEATURE_POPCNT);
    UseIfExist(PopCount64, POPCNT, CPU_FEATURE_POPCNT);
    FallbackTo(PopCount32, OS);
    FallbackTo(PopCount64, OS);
    FallbackTo(PopCount32, NAIVE);
    FallbackTo(PopCount64, NAIVE);

#undef TestFeature
#undef SetFunc
#undef UseIfExist
#undef FallbackTo
}

const MiscIntrins MiscIntrin;

}