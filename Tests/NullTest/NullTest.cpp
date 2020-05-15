#include "SystemCommon/ColorConsole.h"
#include "common/CommonRely.hpp"
#include "common/Linq2.hpp"
#include "common/AlignedBuffer.hpp"
#include "common/StrParsePack.hpp"
#define FMT_STRING_ALIAS 1
#include "fmt/utfext.h"
#include "boost.stacktrace/stacktrace.h"
#include <vector>
#include <cstdio>
#include <iostream>

using namespace std::string_view_literals;
using common::BaseException;
using common::console::ConsoleHelper;


constexpr auto txy = PARSE_PACK(U"abc"sv, 101, U"dev", 202, U"axy", 303, U"hellothere", 404, U"dontknowwho", 505);
constexpr auto t01 = txy(U"abc");
constexpr auto t02 = txy(U"dev");
constexpr auto t03 = txy(U"axy");
constexpr auto t04 = txy(U"hellothere");
constexpr auto t05 = txy(U"dontknowwho");
constexpr auto t00 = txy(U"notexists");


constexpr auto fkl = SWITCH_PACK(ConstEval, (U"abc", 101), (U"dev", 202), (U"axy", 303), (U"hellothere", 404), (U"dontknowwho", 505));
constexpr auto f01 = fkl(U"abc");
constexpr auto f02 = fkl(U"dev");
constexpr auto f03 = fkl(U"axy");
constexpr auto f04 = fkl(U"hellothere");
constexpr auto f05 = fkl(U"dontknowwho");
constexpr auto f00 = fkl(U"notexists");


struct KK
{
    int32_t Num;
};

auto TL(int16_t n)
{
    using namespace common::linq;
    return FromRange<int16_t>(0, n, 1)
        .Select([](const auto i) { printf("[Map[%d]]->", i); return KK{ i * 2 }; })
        .Take(20).Skip(3)
        .Where([](const auto i) { printf("[Filter[%d]]->", i.Num); return i.Num % 3 == 0; });
}

void TestStacktrace()
{
    fmt::basic_memory_buffer<char16_t> out;
    ConsoleHelper console;
    try
    {
        COMMON_THROWEX(BaseException, u"hey");
    }
    catch (const BaseException&)
    {
        try
        {
            COMMON_THROWEX(BaseException, u"hey");
        }
        catch (const BaseException & be2)
        {
            for (const auto kk : be2.Stack())
            {
                out.clear();
                fmt::format_to(out, FMT_STRING(u"[{}]:[{:d}]\t{}\n"), kk.File, kk.Line, kk.Func);
                console.Print(std::u16string_view(out.data(), out.size()));
            }
        }
    }
}

struct ppl
{
    std::vector<std::shared_ptr<void>> ptrs;
    template<typename... Args>
    ppl(std::shared_ptr<Args>... args)
    {
        (ptrs.push_back(args), ...);
    }
};

int main()
{

    const auto pp = ppl{ std::shared_ptr<int>(), std::shared_ptr<float>() };
    printf("\n\n");

    TestStacktrace();
    printf("\n\n");

    getchar();
}

