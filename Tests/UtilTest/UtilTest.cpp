#include "TestRely.h"
#include <map>
#include <vector>
#include <algorithm>
#include "common/TimeUtil.hpp"
#include "common/StringEx.hpp"
#include "common/AlignedContainer.hpp"
#include "common/SIMD128.hpp"
#include "3DBasic/miniBLAS/VecNew.hpp"
#include "common/ResourceHelper.inl"

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

string LoadShaderFallback(const std::u16string& filename, int32_t id)
{
    static const fs::path shdpath(UTF16ER(__FILE__));
    const auto shaderPath = fs::path(shdpath).replace_filename(filename);
    try
    {
        return common::file::ReadAllText(shaderPath);
    }
    catch (const common::file::FileException& fe)
    {
        log().error(u"unable to load shader from [{}]({}) : {}\nFallback to default embeded shader.\n", shaderPath.u16string(), shdpath.u16string(), fe.message);
    }
    auto data = common::ResourceHelper::GetData(L"BIN", id);
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

void QuickTest()
{
    common::AlignedBuffer buf(65536, 32);
    auto buf1 = buf.CreateSubBuffer(16, 32768);
    buf = common::AlignedBuffer(16384, 64);
    auto buf11 = buf1.CreateSubBuffer(32, 120);
    buf1 = buf;
}

int main(int argc, char *argv[])
{
    common::ResourceHelper::Init(nullptr);
    log().info(u"UnitTest\n");

    if (argc > 1)
        BasePathHolder() = fs::absolute(argv[1]);
    log().debug(FMT_STRING(u"Locate BasePath to [{}]\n"), BasePathHolder().u16string());

    uint32_t idx = 0;
    const auto& testMap = GetTestMap();
    for (const auto& pair : testMap)
    {
        log().info(FMT_STRING(u"[{}] {:<20} {}\n"), idx++, pair.first, (void*)pair.second);
    }
    common::SimpleTimer timer;
    timer.Start();
    log().info(u"Select One To Execute...");
    timer.Stop();
    QuickTest();
    if (argc > 2)
        idx = (uint32_t)std::stoul(argv[2]);
    else
    {
        std::cin >> idx;
        ClearReturn();
    }
    if (idx < testMap.size())
    {
        log().info(FMT_STRING(u"Simple logging cost {} ns.\n"), timer.ElapseNs());
        auto it = testMap.cbegin();
        std::advance(it, idx);
        try
        {
            it->second();
        }
        catch (const common::BaseException& be)
        {
            log().error(FMT_STRING(u"Error when performing test:\n{}\n"), be.message);
            fmt::basic_memory_buffer<char16_t> buf;
            for (const auto& stack : be.Stack())
                fmt::format_to(buf, FMT_STRING(u"{}:[{}]\t{}\n"), stack.File, stack.Line, stack.Func);
            log().error(FMT_STRING(u"stack trace:\n{}\n"), std::u16string_view(buf.data(), buf.size()));
        }
    }
    else
    {
        log().error(u"Index out of range.\n");
        auto txtdat = common::ResourceHelper::GetData(L"BIN", IDR_CL_TEST);
        std::string_view txt(reinterpret_cast<const char*>(txtdat.data()), txtdat.size());
        log().verbose(u"\n{}\n\n", txt);
    }
    ClearReturn();
    getchar();
    static_assert(common::str::detail::is_str_vector_v<char, std::vector<char, common::AlignAllocator<char>>>(), "");
    static_assert(common::str::detail::is_str_vector_v<char, std::vector<char>>(), "");
    static_assert(common::str::detail::is_str_vector_v<char, common::container::vectorEx<char>>(), "");
    static_assert(common::is_specialization<std::vector<char>, std::vector>::value, "");
    static_assert(common::is_specialization<std::vector<char, common::AlignAllocator<char>>, std::vector>::value, "");
    static_assert(std::is_same_v<void, common::str::detail::CanBeStringView<std::string, char>>, "can't be sv");
    static_assert(common::str::IsBeginWith("here", "here"), "is not begin");

}
