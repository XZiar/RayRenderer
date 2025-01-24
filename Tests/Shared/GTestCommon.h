#pragma once
#include "SystemCommon/SystemCommonRely.h"
#include "common/FileBase.hpp"
#include "common/TimeUtil.hpp"
#include "common/CommonRely.hpp"
#include "3rdParty/Projects/googletest/gtest-enhanced.h"
#include <type_traits>
#include <tuple>
#include <string>
#include <string_view>
#include <algorithm>

#if COMMON_COMPILER_GCC && COMMON_GCC_VER >= 120000 && COMMON_GCC_VER <= 130000 // gcc12 has many false-positive warning
#   pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif


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

void TestIntrinComplete(common::span<const common::FastPathBase::PathInfo> supports, common::span<const common::FastPathBase::VarItem> intrinMap, bool isComplete);
#define INTRIN_TESTSUITE(name, type, var)                                           \
struct name : public testing::Test { using HostType = type; };                      \
struct name ## Fixture : public name                                                \
{ void TestBody() override { TestIntrinComplete(HostType::GetSupportMap(),          \
    var.GetIntrinMap(), var.IsComplete()); } };                                     \
static testing::TestInfo* Dummy_##name = testing::RegisterTest(#name, "Complete",   \
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


struct PerfTester
{
    std::string TestName;
    std::string ExtraText;
    std::vector<uint64_t> TimeRecords;
    size_t OpPerRun;
    uint32_t LimitMs;
    float KeepRatio;
    bool NeedReport = true;

    PerfTester(std::string_view name, size_t opPerRun = 1, uint32_t limitMs = 300/*0.3s*/, float keepRatio = 0.9): 
        TestName(name), OpPerRun(opPerRun), LimitMs(limitMs), KeepRatio(keepRatio) { }
    ~PerfTester();
    size_t GetRunCount();
    void PreSelectIndex(size_t idx);
    void PostProcess(size_t partIdx, std::string_view itemVar);

    template<typename F>
    void RunUntilTime(F&& func)
    {
        TimeRecords.clear();
        TimeRecords.reserve(LimitMs * 1000);
        const auto firstTime = common::SimpleTimer::getCurTime();
        common::SimpleTimer timer;
        while (true)
        {
            timer.Start();
            func();
            timer.Stop();
            TimeRecords.emplace_back(timer.ElapseNs());
            const auto nowTime = common::SimpleTimer::getCurTime();
            if (nowTime - firstTime > LimitMs)
                break;
        }
    }
    
    template<typename T, typename F>
    void FastPathTest(F&& func)
    {
        std::vector<std::unique_ptr<T>> tests;
        for (const auto& path : T::GetSupportMap())
        {
            if (path.FuncName == TestName)
            {
                for (const auto& var : path.Variants)
                {
                    std::pair<std::string_view, std::string_view> info{ path.FuncName, var.MethodName };
                    tests.emplace_back(std::make_unique<T>(common::span<decltype(info)>{ &info, 1 }));
                }
            }
        }
        for (size_t i = 0; i < GetRunCount(); ++i)
        {
            PreSelectIndex(i);
            for (const auto& test : tests)
            {
                const auto intrinMap = test->GetIntrinMap();
                Ensures(intrinMap.size() == 1);
                Ensures(intrinMap[0].first == TestName);

                RunUntilTime([&]() { func(*test); });
                PostProcess(i, intrinMap[0].second);
            }
        }
    }

    template<typename F, typename... Args>
    void ManualTestSingle(size_t partIdx, std::string_view name, F&& func, Args&&... args)
    {
        RunUntilTime(func);
        PostProcess(partIdx, name);
        if constexpr (sizeof...(Args) > 0)
        {
            ManualTestSingle(partIdx, std::forward<Args>(args)...);
        }
    }
    template<typename... Args>
    void ManaulTest(const Args&... args)
    {
        for (size_t i = 0; i < GetRunCount(); ++i)
        {
            PreSelectIndex(i);
            ManualTestSingle(i, args...);
        }
    }


    template<typename T, typename F, typename... Args>
    static void DoFastPath(F T::* func, std::string_view name, size_t opPerRun, uint32_t limitMs, Args&&... args)
    {
        PerfTester tester(name, opPerRun, limitMs);
        tester.FastPathTest<T>([&](const T& host)
        {
            (host.*func)(std::forward<Args>(args)...);
        });
    }
};


void ListenOnSuiteBegin(std::string_view testsuite, std::function<void(void)> func);
