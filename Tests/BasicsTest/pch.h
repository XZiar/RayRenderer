//
// pch.h
// Header for standard system include files.
//

#pragma once

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "3rdParty/Projects/googletest/gtest-enhanced.h"
#include <type_traits>
#include <iterator>
#include <utility>
#include <limits>
#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include <array>
#include <tuple>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <atomic>
#include <boost/predef/other/endian.h>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>


inline constexpr bool IsLittleEndian = BOOST_ENDIAN_LITTLE_BYTE;


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


template<size_t N>
constexpr std::array<uint8_t, N> GeneratePoses(uint64_t val) noexcept
{
    std::array<uint8_t, N> poses = {};
    for (size_t i = 0; i < N; ++i)
    {
        poses[i] = val % N;
        val /= N;
    }
    return poses;
}

std::mt19937& GetRanEng();
uint32_t GetARand();



class SIMDFixture : public testing::Test
{
public:
    static uint32_t GetSIMDLevel();
    static std::string_view GetSIMDLevelName(const uint32_t level);
};
template<typename T, uint32_t LV>
struct SIMDFactory
{
    static_assert(std::is_base_of_v<SIMDFixture, T>, "Test type shuld derive from SIMDFixture");
    static uint32_t Dummy;
    static uint32_t PendingRegister(std::string testName, const char* fileName, const int fileLine)
    {
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
};

#define RegisterSIMDTest(Test, Level, ...)                          \
template<> uint32_t SIMDFactory<__VA_ARGS__, Level>::Dummy =        \
    SIMDFactory<__VA_ARGS__, Level>::                               \
    PendingRegister(Test, __FILE__, __LINE__);                      \

