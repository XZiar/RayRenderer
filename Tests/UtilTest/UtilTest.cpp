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
        log().Warning(u"unable to load shader from [{}]({}) : {}\nFallback to default embeded shader.\n", shaderPath.u16string(), shdpath.u16string(), fe.Message());
    }
    auto data = common::ResourceHelper::GetData(L"BIN", id);
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}


common::mlog::detail::LoggerFormatter<char16_t>& GetLogFmt()
{
    thread_local common::mlog::detail::LoggerFormatter<char16_t> fmter;
    fmter.Reset();
    return fmter;
}

void PrintToConsole(const common::mlog::detail::LoggerFormatter<char16_t>& fmter)
{
    const auto& console = common::console::ConsoleEx::Get();
    if (const auto seg = fmter.GetColorSegements(); seg.empty())
    {
        console.Print(fmter.Str);
    }
    else
    {
        console.PrintSegments().Print(fmter.Str, seg);
    }
}



void PrintException(const common::BaseException& be, std::u16string_view info)
{
    auto& fmter = GetLogFmt();
    fmter.FormatToStatic(fmter.Str, FmtString(u"{@<R}{}:{}\n{@<r}{}\n"), info, be.Message(), be.GetDetailMessage());
    fmter.FormatToStatic(fmter.Str, FmtString(u"{@<W}stack trace:\n"));
    for (const auto& stack : be.InnerInfo()->GetStacks())
        fmter.FormatToStatic(fmter.Str, FmtString(u"{@<w}[{}:{}]\t{@<y}{}\n"), stack.File, stack.Line, stack.Func);
    common::mlog::SyncConsoleBackend();
    PrintToConsole(fmter);
    if (const auto inEx = be.NestedException(); inEx.has_value())
        return PrintException(*inEx, u"Caused by");
}

void ClearReturn()
{
    std::cin.ignore(1024, '\n');
}

char16_t GetIdx36(const size_t idx)
{
    constexpr std::u16string_view Idx36 = u"0123456789abcdefghijklmnopqrstuvwxyz";
    if (idx < 36)
        return Idx36[idx];
    return u'*';
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

char EnterOneOf(std::u16string_view prompt, std::string_view str)
{
    common::mlog::SyncConsoleBackend();
    common::console::ConsoleEx::Get().Print(prompt);
    char ch = '\0';
    do
    {
        ch = common::console::ConsoleEx::ReadCharImmediate(false);
        if (std::isalpha(ch))
            ch = static_cast<char>(std::tolower(ch));
    } while (str.find_first_of(ch) == str.npos);
    return ch;
}


int main(int argc, char *argv[])
{
    common::ResourceHelper::Init(nullptr);
    GetConsole().Print(CommonColor::BrightGreen, u"UnitTest\n");
    common::PrintSystemVersion();

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
            ColorPrint(u"{@<G}[{}] {@<W}{:<20} {@<w}{}\n", GetIdx36(idx++), pair.first, pair.second);
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
            PrintException(be, FMTSTR2(u"Error when performing test [{}]", it->first));
        }
    }
    else
    {
        log().Error(u"Index out of range.\n");
        auto txtdat = common::ResourceHelper::GetData(L"BIN", IDR_CL_TEST);
        std::string_view txt(reinterpret_cast<const char*>(txtdat.data()), txtdat.size());
        log().Verbose(u"\n{}\n\n", txt);
    }
    ClearReturn();
    getchar();

}
