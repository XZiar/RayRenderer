#include "common/CommonRely.hpp"
#include "common/ColorConsole.inl"
#include "common/Linq.hpp"
#include "fmt/utfext.h"
#include "boost.stacktrace/stacktrace.h"
#include <vector>
#include <cstdio>
using common::BaseException;
using common::console::ConsoleHelper;
using common::linq::Linq;


int main()
{
    std::vector<int> src{ 1,2,3,4,5,6,7 };
    auto lq = Linq::FromIterable(src);
    for (auto& item : lq)
    {
        item = item + 1;
    }
    auto lqw = Linq::FromIterable(src).ToAnyEnumerable();
    for (const auto& item : lqw)
    {
        printf("%d", item);
    }

    fmt::basic_memory_buffer<char16_t> out;
    ConsoleHelper console;
    try
    {
        COMMON_THROWEX(BaseException, u"hey");
    }
    catch (const BaseException& be)
    {
        try
        {
            COMMON_THROWEX(BaseException, u"hey");
        }
        catch (const BaseException& be2)
        {
            for (const auto kk : be2.Stack())
            {
                out.clear();
                fmt::format_to(out, u"[{}]:[{}]\t{}\n", (std::u16string_view)kk.File, kk.Line, (std::u16string_view)kk.Func);
                console.Print(std::u16string_view(out.data(), out.size()));
            }
            getchar();
        }
    }
}
