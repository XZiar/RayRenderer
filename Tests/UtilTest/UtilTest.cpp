#include "TestRely.h"
#include <map>
#include <vector>
#include <algorithm>
#include "common/TimeUtil.hpp"
#include "common/StringEx.hpp"
#include "common/SIMD128.hpp"

using std::string;
using std::map;
using std::vector;

static common::mlog::MiniLogger<false>& log()
{
    static common::mlog::MiniLogger<false> logger(u"Main", { common::mlog::GetConsoleBackend() });
    return logger;
}

common::fs::path FindPath()
{
#if (defined(__x86_64__) && __x86_64__) || (defined(_WIN64) && _WIN64)
    return common::fs::current_path().parent_path().parent_path();
#else
    return common::fs::current_path().parent_path();
#endif
}

static map<string, void(*)()>& GetTestMap()
{
    static map<string, void(*)()> testMap;
    return testMap;
}
uint32_t RegistTest(const char *name, void(*func)())
{
    GetTestMap()[name] = func;
    //printf("Test [%-30s] registed.\n", name);
    return 0;
}


int main(int argc, char *argv[])
{
    log().info(u"UnitTest\n");
    uint32_t idx = 0;
    const auto& testMap = GetTestMap();
    for (const auto& pair : testMap)
    {
        log().info(u"[{}] {:<20} {}\n", idx++, pair.first, (void*)pair.second);
    }
    common::SimpleTimer timer;
    timer.Start();
    log().info(u"Select One To Execute...");
    timer.Stop();
    if (argc > 1)
        idx = (uint32_t)std::stoul(argv[1]);
    else
        std::cin >> idx;
    if (idx < testMap.size())
    {
        log().info(u"Simple logging cost {} ns.\n", timer.ElapseNs());
        auto it = testMap.cbegin();
        std::advance(it, idx);
        it->second();
    }
    else
    {
        log().error(u"Index out of range.\n");
    }
    getchar();
    static_assert(common::str::detail::is_str_vector_v<char, std::vector<char, common::AlignAllocator<char>>>(), "");
    static_assert(common::str::detail::is_str_vector_v<char, std::vector<char>>(), "");
    static_assert(common::str::detail::is_str_vector_v<char, common::vectorEx<char>>(), "");
    static_assert(common::is_specialization<std::vector<char>, std::vector>::value, "");
    static_assert(common::is_specialization<std::vector<char, common::AlignAllocator<char>>, std::vector>::value, "");
    static_assert(std::is_same_v<void, common::str::detail::CanBeStringView<std::string, char>>, "can't be sv");
    static_assert(common::str::IsBeginWith("here", "here"), "is not begin");

}
