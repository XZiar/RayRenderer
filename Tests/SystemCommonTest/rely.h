#pragma once
#include "common/CommonRely.hpp"
#include "SystemCommon/SystemCommonRely.h"
#include "3rdParty/Projects/googletest/gtest-enhanced.h"
#include <type_traits>
#include <tuple>
#include <string>
#include <string_view>



template<typename T>
uint32_t RegisterIntrinTest(const char* testsuite, const char* fileName, const int fileLine)
{
    for (const auto& path : T::GetSupportMap())
    {
        if (path.FuncName == T::FuncName)
        {
            for (const auto& var : path.Variants)
            {
                const std::string testName = std::string(path.FuncName).append("/").append(var.MethodName);
                testing::RegisterTest(
                    testsuite, testName.c_str(),
                    nullptr, nullptr,
                    fileName, fileLine,
                    [&]() -> typename T::Parent* { return new T(path.FuncName, var.MethodName); }); // explicit cast to match testsuit id
            }
        }
    }
    return 0;
}

void TestIntrinComplete(common::span<const common::FastPathBase::PathInfo> supports, const common::FastPathBase& host);


#define INTRIN_TESTSUITE(name, type, var)                                               \
struct name : public testing::Test { using HostType = type; };                          \
struct name ## Fixture : public name                                                    \
{ void TestBody() override { TestIntrinComplete(HostType::GetSupportMap(), var); } };   \
static testing::TestInfo* Dummy_ ## name = testing::RegisterTest(#name, "Complete",     \
    nullptr, nullptr, __FILE__, __LINE__, []() -> name* { return new name ## Fixture(); })



template<typename T>
class FuncFixture : public T
{
    const std::pair<std::string_view, std::string_view> Info;

    void SetUp() override
    {
        Intrin = std::make_unique<typename T::HostType>(common::span<decltype(Info)>{ &Info,1 });
    }
    void TestBody() override
    {
        const auto result = Intrin->GetIntrinMap();
        ASSERT_EQ(result.size(), 1u);
        ASSERT_EQ(result[0], Info);
        InnerTest();
    }
protected:
    std::unique_ptr<typename T::HostType> Intrin;
    virtual void InnerTest() = 0;
public:
    using Parent = T;
    FuncFixture(std::string_view func, std::string_view var) : Info{ func, var }
    { }
    static auto GetSupportMap() noexcept { return T::HostType::GetSupportMap(); }
};

#define INTRIN_TEST(type, func)                     \
struct func ## Fixture : public FuncFixture<type>   \
{                                                   \
    static constexpr auto FuncName = #func;         \
    using FuncFixture<type>::FuncFixture;           \
    void InnerTest() override;                      \
};                                                  \
static uint32_t Dummy_ ## func = RegisterIntrinTest \
    <func ## Fixture>(#type, __FILE__, __LINE__);   \
void func ## Fixture::InnerTest()

