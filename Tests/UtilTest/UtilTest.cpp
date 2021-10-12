#include "TestRely.h"
#include "common/Linq2.hpp"
#include "common/TimeUtil.hpp"
#include "common/StringEx.hpp"
#include "common/AlignedContainer.hpp"
#include "common/ResourceHelper.inl"
#include <map>
#include <vector>
#include <algorithm>
#include <iostream>

using std::string;
using std::map;
using std::vector;
using common::CommonColor;
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


std::vector<std::string_view>& GetCmdArgs_()
{
    static std::vector<std::string_view> ARGS;
    return ARGS;
}
const std::vector<std::string_view>& GetCmdArgs()
{
    return GetCmdArgs_();
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
        log().warning(u"unable to load shader from [{}]({}) : {}\nFallback to default embeded shader.\n", shaderPath.u16string(), shdpath.u16string(), fe.Message());
    }
    auto data = common::ResourceHelper::GetData(L"BIN", id);
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

void PrintException(const common::BaseException& be, std::u16string_view info)
{
    common::mlog::SyncConsoleBackend();
    GetConsole().Print(CommonColor::BrightRed,
        FMTSTR(u"{}:{}\n{}\n", info, be.Message(), be.GetDetailMessage()));
    {
        std::u16string str;
        for (const auto& stack : be.Stack())
            fmt::format_to(std::back_inserter(str), FMT_STRING(u"[{}:{}]\t{}\n"), stack.File, stack.Line, stack.Func);
        GetConsole().Print(CommonColor::BrightWhite, u"stack trace:\n");
        GetConsole().Print(CommonColor::BrightYellow, str);
    }
    if (const auto inEx = be.NestedException(); inEx.has_value())
        return PrintException(*inEx, u"Caused by");
}

void ClearReturn()
{
    std::cin.ignore(1024, '\n');
}

char GetIdx36(const size_t idx)
{
    constexpr std::string_view Idx36 = "0123456789abcdefghijklmnopqrstuvwxyz";
    if (idx < 36)
        return Idx36[idx];
    return '*';
}

uint32_t Select36(const size_t size)
{
    uint32_t idx = UINT32_MAX;
    do
    {
        const auto ch = common::console::ConsoleEx::ReadCharImmediate(false);
        if (ch >= '0' && ch <= '9')
            idx = ch - '0';
        else if (ch >= 'a' && ch <= 'z')
            idx = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'Z')
            idx = ch - 'A' + 10;
    } while (idx >= size);
    return idx;
}


int main(int argc, char *argv[])
{
    common::ResourceHelper::Init(nullptr);
    GetConsole().Print(CommonColor::BrightGreen, u"UnitTest\n");

    std::vector<std::string_view> args;
    {
        for (const std::string_view arg : common::span<char*>(argv, argv + argc))
        {
            if (common::str::IsBeginWith(arg, "-"))
                GetCmdArgs_().push_back(arg.substr(1));
            else
                args.push_back(arg);
        }
    }

    if (args.size() >= 1)
        BasePathHolder() = fs::absolute(args[0]).parent_path().parent_path().parent_path();
    GetConsole().Print(CommonColor::BrightMagenta, FMTSTR(u"Locate BasePath to [{}]\n", BasePathHolder().u16string()));

    const auto& testMap = GetTestMap();
    {
        uint32_t idx = 0;
        for (const auto& pair : testMap)
        {
            GetConsole().Print(CommonColor::BrightWhite,
                FMTSTR(u"[{}] {:<20} {}\n", GetIdx36(idx++), pair.first, (void*)pair.second));
        }
    }
    GetConsole().Print(CommonColor::BrightWhite, u"Select One To Execute...\n");
    
    const auto idx = args.size() >= 2 ?
        [&]() -> uint32_t 
        {
            uint32_t idx = 0;
            for (const auto& pair : testMap)
            {
                if (pair.first == args[1])
                    return idx;
                idx++;
            }
            return UINT32_MAX;
        }() : 
        Select36(testMap.size());

    if (idx < testMap.size())
    {
        auto it = testMap.cbegin();
        std::advance(it, idx);
        try
        {
            it->second();
        }
        catch (const common::BaseException& be)
        {
            PrintException(be, FMTSTR(u"Error when performing test [{}]", it->first));
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

}
