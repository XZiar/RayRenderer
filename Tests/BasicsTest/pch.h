//
// pch.h
// Header for standard system include files.
//

#pragma once

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <iterator>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>
#include <array>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <limits>
#include <tuple>


template<uint8_t Base, uint8_t Exp>
inline constexpr uint64_t Pow()
{
    uint64_t ret = Base;
    for (uint8_t i = 1; i < Exp; ++i)
        ret *= Base;
    return ret;
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

#define RegisterSIMDTest(Test, Level, ...)                  \
uint32_t SIMDFactory<__VA_ARGS__, Level>::Dummy =           \
    SIMDFactory<__VA_ARGS__, Level>::                       \
    PendingRegister(Test, __FILE__, __LINE__);              \

