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
namespace fs = common::fs;

static common::mlog::MiniLogger<false>& log()
{
    static common::mlog::MiniLogger<false> logger(u"Main", { common::mlog::GetConsoleBackend() });
    return logger;
}

static fs::path& BasePathHolder()
{
#if (defined(__x86_64__) && __x86_64__) || (defined(_WIN64) && _WIN64)
    static auto basePath = fs::current_path().parent_path().parent_path();
#else
    static auto basePath = fs::current_path().parent_path();
#endif
    return basePath;
}

fs::path FindPath()
{
    return BasePathHolder();
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

    if (argc > 1)
        BasePathHolder() = fs::absolute(argv[1]);
    log().debug(u"Locate BasePath to [{}]\n", BasePathHolder().u16string());

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
    if (argc > 2)
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
