#include "common/CommonRely.hpp"
#include "common/ColorConsole.inl"
#include "fmt/utfext.h"
#include "boost.stacktrace/stacktrace.h"
using common::BaseException;
using common::console::ConsoleHelper;


int main()
{
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

