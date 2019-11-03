#include "SystemCommon/ColorConsole.h"
#include "common/CommonRely.hpp"
#include "common/Linq2.hpp"
#include "common/AlignedBuffer.hpp"
#define FMT_STRING_ALIAS 1
#include "fmt/utfext.h"
#include "boost.stacktrace/stacktrace.h"
#include <vector>
#include <cstdio>
#include <iostream>
using common::BaseException;
using common::console::ConsoleHelper;



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



int main()
{

    printf("\n\n");

    TestStacktrace();
    printf("\n\n");

    getchar();
}

