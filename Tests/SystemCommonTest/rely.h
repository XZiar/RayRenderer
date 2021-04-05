#pragma once
#include "common/CommonRely.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "common/gtesthack.h"
#include <type_traits>
#include <tuple>
#include <string>
#include <string_view>



template<typename T>
uint32_t RegisterIntrinTest(const char* testsuite, const char* fileName, const int fileLine)
{
    for (const auto& item : T::HostType::GetSupportMap())
    {
        if (item.first == T::FuncName)
        {
            const std::string testName = std::string(item.first).append("/").append(item.second);
            testing::RegisterTest(
                testsuite, testName.c_str(),
                nullptr, nullptr,
                fileName, fileLine,
                [&]() -> testing::Test* { return new T(item); });
        }
    }
    return 0;
}

template<typename T>
class IntrinFixture : public testing::Test
{
    const std::pair<std::string_view, std::string_view> Info;

    void SetUp() override
    {
        Intrin = std::make_unique<T>(common::span<decltype(Info)>{ &Info,1 });
    }
    void TestBody() override
    {
        const auto result = Intrin->GetIntrinMap();
        ASSERT_EQ(result.size(), 1u);
        ASSERT_EQ(result[0], Info);
        InnerTest();
    }
protected:
    std::unique_ptr<T> Intrin;
    virtual void InnerTest() = 0;
public:
    using HostType = T;
    IntrinFixture(const std::pair<std::string_view, std::string_view> item) : Info(item)
    { }
};

#define INTRIN_TEST(type, func) \
struct func ## Fixture : public IntrinFixture<type> \
{ \
    static constexpr auto FuncName = #func; \
    using IntrinFixture<type>::IntrinFixture; \
    void InnerTest() override; \
}; \
static uint32_t Dummy_ ## func = RegisterIntrinTest \
    <func ## Fixture>(#type, __FILE__, __LINE__); \
void func ## Fixture::InnerTest() \

