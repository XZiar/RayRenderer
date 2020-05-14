#include "SystemCommonPch.h"
#include "MiscIntrins.h"
#include "common/SIMD.hpp"
#include "cpuid/libcpuid.h"

namespace common
{

#define VAR_HAS_OS 1
#define VAR_HAS_NAIVE 1

uint32_t LeadZero32_OS(const uint32_t num) noexcept
{
#if COMPILER_MSVC
    unsigned long idx = 0;
    return _BitScanReverse(&idx, num) ? 31 - idx : 32;
#else
    return num == 0 ? 32 : __builtin_clz(num);
#endif
}
uint32_t LeadZero64_OS(const uint64_t num) noexcept
{
#if COMPILER_MSVC
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
#else
    return num == 0 ? 64 : __builtin_clzll(num);
#endif
}

uint32_t TailZero32_OS(const uint32_t num) noexcept
{
#if COMPILER_MSVC
    unsigned long idx = 0;
    return _BitScanForward(&idx, num) ? idx : 32;
#else
    return num == 0 ? 32 : __builtin_ctz(num);
#endif
}
uint32_t TailZero64_OS(const uint64_t num) noexcept
{
#if COMPILER_MSVC
    unsigned long idx = 0;
#   if COMMON_OSBIT == 64
    return _BitScanForward64(&idx, num) ? idx : 64;
#   else
    const auto lo = static_cast<uint32_t>(num), hi = static_cast<uint32_t>(num >> 32);
    if (!_BitScanForward(&idx, lo))
        return _BitScanForward(&idx, hi) ? idx + 32 : 64;
    else
        return idx;
#   endif
#else
    return num == 0 ? 64 : __builtin_ctzll(num);
#endif
}

#if !COMPILER_MSVC
uint32_t PopCount32_OS(const uint32_t num) noexcept
{
    return __builtin_popcount(num);
}
uint32_t PopCount64_OS(const uint64_t num) noexcept
{
    return __builtin_popcountll(num);
}
#endif
uint32_t PopCount32_NAIVE(const uint32_t num) noexcept
{
    auto tmp = num - ((num >> 1) & 0x55555555u);
    tmp = (tmp & 0x33333333u) + ((tmp >> 2) & 0x33333333u);
    tmp = (tmp + (tmp >> 4)) & 0x0f0f0f0fu;
    return (tmp * 0x01010101u) >> 24;
}
uint32_t PopCount64_NAIVE(const uint64_t num) noexcept
{
    auto tmp = num - ((num >> 1) & 0x5555555555555555u);
    tmp = (tmp & 0x3333333333333333u) + ((tmp >> 2) & 0x3333333333333333u);
    tmp = (tmp + (tmp >> 4)) & 0x0f0f0f0f0f0f0f0fu;
    return (tmp * 0x0101010101010101u) >> 56;
}


#if (COMPILER_MSVC/* && COMMON_SIMD_LV >= 200*/) || (!COMPILER_MSVC && defined(__LZCNT__))
#   define VAR_HAS_LZCNT 1
uint32_t LeadZero32_LZCNT(const uint32_t num) noexcept
{
    return _lzcnt_u32(num);
}
uint32_t LeadZero64_LZCNT(const uint64_t num) noexcept
{
#   if COMMON_OSBIT == 64
    return static_cast<uint32_t>(_lzcnt_u64(num));
#   else
    const auto lo = static_cast<uint32_t>(num), hi = static_cast<uint32_t>(num >> 32);
    const auto hiCnt = _lzcnt_u32(hi);
    return hiCnt == 32 ? _lzcnt_u32(lo) + 32 : hiCnt;
#   endif
}
#else
#   define VAR_HAS_LZCNT 0
#endif


#if (COMPILER_MSVC/* && COMMON_SIMD_LV >= 200*/) || (!COMPILER_MSVC && defined(__BMI__))
#   define VAR_HAS_BMI1 1
uint32_t TailZero32_BMI1(const uint32_t num) noexcept
{
    return _tzcnt_u32(num);
}
uint32_t TailZero64_BMI1(const uint64_t num) noexcept
{
#   if COMMON_OSBIT == 64
    return static_cast<uint32_t>(_tzcnt_u64(num));
#   else
    const auto lo = static_cast<uint32_t>(num), hi = static_cast<uint32_t>(num >> 32);
    const auto loCnt = _tzcnt_u32(lo);
    return loCnt == 32 ? _tzcnt_u32(hi) + 32 : loCnt;
#   endif
}
#else
#   define VAR_HAS_BMI1 0
#endif


#if (COMPILER_MSVC/* && COMMON_SIMD_LV >= 42*/) || (!COMPILER_MSVC && defined(__POPCNT__))
#   define VAR_HAS_POPCNT 1
uint32_t PopCount32_POPCNT(const uint32_t num) noexcept
{
    return _mm_popcnt_u32(num);
}
uint32_t PopCount64_POPCNT(const uint64_t num) noexcept
{
#   if COMMON_OSBIT == 64
    return static_cast<uint32_t>(_mm_popcnt_u64(num));
#   else
    const auto lo = static_cast<uint32_t>(num), hi = static_cast<uint32_t>(num >> 32);
    return _mm_popcnt_u32(lo) + _mm_popcnt_u32(hi);
#   endif
}
#else
#   define VAR_HAS_POPCNT 0
#endif


uint32_t (*MiscIntrins::LeadZero32)(const uint32_t) noexcept = nullptr;
uint32_t (*MiscIntrins::LeadZero64)(const uint64_t) noexcept = nullptr;
uint32_t (*MiscIntrins::TailZero32)(const uint32_t) noexcept = nullptr;
uint32_t (*MiscIntrins::TailZero64)(const uint64_t) noexcept = nullptr;
uint32_t (*MiscIntrins::PopCount32)(const uint32_t) noexcept = nullptr;
uint32_t (*MiscIntrins::PopCount64)(const uint64_t) noexcept = nullptr;


static std::vector<std::pair<std::string_view, std::string_view>>& GetVariantMap()
{
    static std::vector<std::pair<std::string_view, std::string_view>> variantMap;
    return variantMap;
}

void MiscIntrins::InitIntrins() noexcept
{
    struct cpu_id_t data;
    bool cpuidOK = false;
    if (cpuid_present())
    {
        struct cpu_raw_data_t raw;
        if (cpuid_get_raw_data(&raw) >= 0)
        {
            if (cpu_identify(&raw, &data) >= 0)
                cpuidOK = true;
        }
    }
    auto& variantMap = GetVariantMap();

#define TestFeature(feat) (cpuidOK && data.flags[feat])
#define SetFunc0(func, variant, test) ((void)0)
#define SetFunc1(func, variant, test) \
    do \
    { \
        if (func == nullptr && test) \
        { \
            func = func##_##variant; \
            variantMap.emplace_back(STRINGIZE(func), STRINGIZE(variant)); \
        } \
    } while(0)

#define UseIfExist(func, variant, feat) PPCAT(SetFunc, PPCAT(VAR_HAS_, variant))(func, variant, TestFeature(feat))
#define FallbackTo(func, variant) PPCAT(SetFunc, PPCAT(VAR_HAS_, variant))(func, variant, true)

    UseIfExist(LeadZero32, LZCNT, CPU_FEATURE_ABM);
    UseIfExist(LeadZero64, LZCNT, CPU_FEATURE_ABM);
    FallbackTo(LeadZero32, OS);
    FallbackTo(LeadZero64, OS);

    UseIfExist(TailZero32, BMI1, CPU_FEATURE_BMI1);
    UseIfExist(TailZero64, BMI1, CPU_FEATURE_BMI1);
    FallbackTo(TailZero32, OS);
    FallbackTo(TailZero64, OS);

    UseIfExist(PopCount32, POPCNT, CPU_FEATURE_POPCNT);
    UseIfExist(PopCount64, POPCNT, CPU_FEATURE_POPCNT);
#if !COMPILER_MSVC
    FallbackTo(PopCount32, OS);
    FallbackTo(PopCount64, OS);
#endif
    FallbackTo(PopCount32, NAIVE);
    FallbackTo(PopCount64, NAIVE);

#undef TestFeature
#undef SetFunc0
#undef SetFunc1
#undef UseIfExist
#undef FallbackTo
}

common::span<std::pair<std::string_view, std::string_view>> MiscIntrins::GetIntrinMap() noexcept
{
    return GetVariantMap();
}

uint32_t InnerInject() noexcept
{
    // printf("==[BulitinMisc]==\tRegister here.\n");
    return RegisterInitializer(&MiscIntrins::InitIntrins);
}

static uint32_t Dummy = InnerInject();

}