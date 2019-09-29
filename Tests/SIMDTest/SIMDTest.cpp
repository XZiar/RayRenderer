#include "SIMDRely.h"
#include "common/TimeUtil.hpp"
#include "3rdParty/cpuid/libcpuid.h"
#include <vector>

using std::u16string;
using std::vector;



fmt::basic_memory_buffer<char16_t>& GetFmtBuffer()
{
    static fmt::basic_memory_buffer<char16_t> buffer;
    return buffer;
}
static common::console::ConsoleHelper TheConsole;
const common::console::ConsoleHelper& GetConsole()
{
    return TheConsole;
}

struct TestItem
{
    u16string Name;
    bool(*Func)();
    uint32_t SIMDLevel;
};
static vector<TestItem>& GetTestMap()
{
    static vector<TestItem> testMap;
    return testMap;
}
uint32_t RegistTest(const char16_t *name, const uint32_t simdLv, bool(*func)())
{
    GetTestMap().emplace_back(TestItem{ name, func, simdLv });
    //printf("Test [%-30s] registed.\n", name);
    return 0;
}

static uint32_t GetSIMDLevel()
{
    if (!cpuid_present())               return 0;
    struct cpu_raw_data_t raw;
    if (cpuid_get_raw_data(&raw) < 0)   return 0;
    struct cpu_id_t data;
    if (cpu_identify(&raw, &data) < 0)  return 0;

    if (data.flags[CPU_FEATURE_AVX2])   return 200;
    if (data.flags[CPU_FEATURE_FMA3])   return 150;
    if (data.flags[CPU_FEATURE_AVX])    return 100;
    if (data.flags[CPU_FEATURE_SSE4_2]) return 42;
    if (data.flags[CPU_FEATURE_SSE4_1]) return 41;
    if (data.flags[CPU_FEATURE_SSSE3])  return 31;
    if (data.flags[CPU_FEATURE_PNI])    return 30;
    if (data.flags[CPU_FEATURE_SSE2])   return 20;
    if (data.flags[CPU_FEATURE_SSE])    return 10;
    return 0;
}


int main()
{
    const auto& tests = GetTestMap();
    Log(LogType::Success, u"[{}] Test found.\n", tests.size());
    {
        uint32_t idx = 0;
        for (const auto test : tests)
            Log(LogType::Verbose, u"[{:2}] {:<30}\n", idx++, test.Name);
    }
    const auto simdLv = GetSIMDLevel();
    Log(LogType::Info, u"Runtime SIMD Level: [{}]\n", simdLv);
    Log(LogType::Success, u"Start Test.\n\n");

    uint32_t pass = 0, total = 0;
    for (const auto test : tests)
    {
        Log(LogType::Info, u"begin [{}]\n", test.Name);
        if (simdLv < test.SIMDLevel)
        {
            Log(LogType::Warning, u"SIMD [{}] required unsatisfied, skip.\n", test.SIMDLevel);
            continue;
        }
        total++;
        common::SimpleTimer timer;
        timer.Start();
        const auto ret = test.Func();
        timer.Stop();
        if (ret)
            Log(LogType::Success, u"Test passes in {} ms.\n", timer.ElapseMs()), pass++;
        else
            Log(LogType::Error,   u"Test failed in {} ms.\n", timer.ElapseMs());
    }

    Log(pass == total ? LogType::Success : LogType::Error, u"\n[{}/{}] Test pases.\n", pass, total);

}