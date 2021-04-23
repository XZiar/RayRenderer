#pragma once
#include "common/CommonRely.hpp"
#include "3rdParty/Projects/googletest/gtest-enhanced.h"
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
                [&]() -> typename T::Parent* { return new T(item); }); // explicit cast to match testsuit id
        }
    }
    return 0;
}

template<typename T>
struct IntrinFixture : public testing::Test
{
    using HostType = T;
};

#define INTRIN_TESTSUITE(name, type) struct name : public IntrinFixture<type> {}


template<typename T>
class FuncFixture : public T
{
    using U = typename T::HostType;
    const std::pair<std::string_view, std::string_view> Info;

    void SetUp() override
    {
        Intrin = std::make_unique<U>(common::span<decltype(Info)>{ &Info,1 });
    }
    void TestBody() override
    {
        const auto result = Intrin->GetIntrinMap();
        ASSERT_EQ(result.size(), 1u);
        ASSERT_EQ(result[0], Info);
        InnerTest();
    }
protected:
    std::unique_ptr<U> Intrin;
    virtual void InnerTest() = 0;
public:
    FuncFixture(const std::pair<std::string_view, std::string_view> item) : Info(item)
    { }
};

#define INTRIN_TEST(type, func)                     \
struct func ## Fixture : public FuncFixture<type>   \
{                                                   \
    using Parent = type;                            \
    static constexpr auto FuncName = #func;         \
    using FuncFixture<type>::FuncFixture;           \
    void InnerTest() override;                      \
};                                                  \
static uint32_t Dummy_ ## func = RegisterIntrinTest \
    <func ## Fixture>(#type, __FILE__, __LINE__);   \
void func ## Fixture::InnerTest()

