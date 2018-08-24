#include "SIMDRely.h"
#include "common/TimeUtil.hpp"
#include "common/ColorConsole.inl"
#include <map>

using std::u16string;
using std::map;



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


static map<u16string, bool(*)()>& GetTestMap()
{
    static map<u16string, bool(*)()> testMap;
    return testMap;
}
uint32_t RegistTest(const char16_t *name, bool(*func)())
{
    GetTestMap()[name] = func;
    //printf("Test [%-30s] registed.\n", name);
    return 0;
}


int main()
{
    uint32_t count = 0;
    const auto& testMap = GetTestMap();
    Log(LogType::Success, u"[{}] Test found.", testMap.size());
    for (const auto&[name, func] : testMap)
    {
        Log(LogType::Verbose, u"[{:2}] {:<30}", count++, name);
    }
    Log(LogType::Success, u"Start Test.\n");

    uint32_t pass = 0;
    for (const auto&[name, func] : testMap)
    {
        Log(LogType::Info, u"begin {}", name);
        common::SimpleTimer timer;
        timer.Start();
        const auto ret = func();
        timer.Stop();
        if (ret)
            Log(LogType::Success, u"Test passes in {} ms.\n", timer.ElapseMs()), pass++;
        else
            Log(LogType::Error,   u"Test failed in {} ms.\n", timer.ElapseMs());
    }

    Log(pass == count ? LogType::Success : LogType::Error, u"[{}/{}] Test pases.\n", pass, count);

#if defined(COMPILER_MSVC) && COMPILER_MSVC
    getchar();
#endif
}