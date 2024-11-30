#pragma once

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "3rdParty/Projects/googletest/gtest-enhanced.h"
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <type_traits>
#include <utility>
#include <limits>
#include <vector>
#include <array>
#include <tuple>
#include <cinttypes>


::common::fp16_t FP32ToFP16(float val) noexcept;
float FP16ToFP32(::common::fp16_t val) noexcept;

template<typename U, bool Saturate, typename T>
forceinline U CastSingle(T val) noexcept
{
    if constexpr (Saturate)
    {
        constexpr auto dstMin = std::numeric_limits<U>::min(), dstMax = std::numeric_limits<U>::max();
        constexpr auto minVal = static_cast<T>(dstMin), maxVal = static_cast<T>(dstMax);
        if (val <= minVal) return dstMin;
        if (val >= maxVal) return dstMax;
    }
    if constexpr (std::is_same_v<U, ::common::fp16_t>)
        return FP32ToFP16(static_cast<float>(val));
    else if constexpr (std::is_same_v<T, ::common::fp16_t>)
        return static_cast<U>(FP16ToFP32(val));
    else
        return static_cast<U>(val);
}

template<uint8_t Base, uint8_t Exp>
inline constexpr uint64_t Pow()
{
    uint64_t ret = Base;
    for (uint8_t i = 1; i < Exp; ++i)
        ret *= Base;
    return ret;
}

template<uint8_t N>
struct PosesHolder
{
    static std::vector<std::array<uint8_t, N>> Poses;
};
template<>
std::vector<std::array<uint8_t, 2>> PosesHolder<2>::Poses;
template<>
std::vector<std::array<uint8_t, 4>> PosesHolder<4>::Poses;
template<>
std::vector<std::array<uint8_t, 8>> PosesHolder<8>::Poses;


constexpr size_t RandValBytes = 512;
alignas(32) extern const std::array<uint8_t, RandValBytes / 1> RandVals;
alignas(32) extern const std::array<::common::fp16_t, RandValBytes / 2> RandValsF16;
alignas(32) extern const std::array<float,   RandValBytes / 4> RandValsF32;
alignas(32) extern const std::array<double,  RandValBytes / 8> RandValsF64;
template<typename T, typename U>
static const T* GetRandPtr() noexcept
{
    if constexpr (std::is_same_v<U, double>)
        return reinterpret_cast<const T*>(RandValsF64.data());
    else if constexpr (std::is_same_v<U, float>)
        return reinterpret_cast<const T*>(RandValsF32.data());
    else if constexpr (std::is_same_v<U, ::common::fp16_t>)
        return reinterpret_cast<const T*>(RandValsF16.data());
    else
        return reinterpret_cast<const T*>(RandVals.data());
}

inline constexpr auto StaticRands = []() 
{
    constexpr uint64_t multiplier = 0x5DEECE66Du;
    constexpr uint64_t addend = 0xBu;
    constexpr uint64_t mask = (uint64_t(1) << 48) - 1;
    uint64_t seed = 19937u;
    std::array<uint32_t, 4096> ret = {};
    for (auto& val : ret)
    {
        val = static_cast<uint32_t>(seed >> 16);
        seed = (seed * multiplier + addend) & mask; // allow repeat
    }
    return ret;
}();


class SIMDFixture : public testing::Test
{
public:
    static uint32_t GetSIMDLevel();
    static std::string_view GetSIMDLevelName(const uint32_t level);
};
template<typename T, uint32_t LV>
inline uint32_t PendingRegister(std::string testName, const char* fileName, const int fileLine)
{
    static_assert(std::is_base_of_v<SIMDFixture, T>, "Test type shuld derive from SIMDFixture");
    if (SIMDFixture::GetSIMDLevel() >= LV)
    {
        testName.append("/").append(SIMDFixture::GetSIMDLevelName(LV));
        testing::RegisterTest(
            T::TestSuite, testName.c_str(),
            nullptr, nullptr,
            fileName, fileLine,
            [=]() -> SIMDFixture* { return new T(); });
    }
    return 0;
}


#define RegisterSIMDTest(Type, Level, ...)                                      \
static uint32_t PPCAT(PPCAT(Dummy_, Type), PPCAT(_, __LINE__)) =                \
    PendingRegister<__VA_ARGS__, Level>(STRINGIZE(Type), __FILE__, __LINE__)
