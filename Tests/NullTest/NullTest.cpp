#include "SystemCommon/ColorConsole.h"
#include "common/CommonRely.hpp"
#include "common/Linq2.hpp"
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

void TestLinq()
{
    using namespace common::linq;
    using common::linq::detail::EnumerableChecker;
    //std::vector<std::unique_ptr<int>> kkk;
    //kkk.emplace_back(std::make_unique<int>(23));
    //kkk.emplace_back(std::make_unique<int>(24));
    //const auto ky = Linq::FromRange(0, 10)
    //    .Pair(Linq::FromIterable(kkk))
    //    //.Select([](auto& val) { return std::make_pair(true, std::move(val)); })
    //    .Where([](const auto&) { return true; })
    //    /*.ToMap<std::map<int16_t, std::unique_ptr<int>>>(
    //        [](const auto& p) -> int16_t { return p.first; },
    //        [](auto& p) { return std::move(p.second); });*/
    //    .ToVector();

    //std::vector<int> src{ 1,2,3,4,5,6,7 };
    //auto lq = Linq::FromIterable(src);
    //for (auto& item : lq)
    //{
    //    item = item + 1;
    //}
    //auto lqw = Linq::FromIterable(src).ToAnyEnumerable();
    //for (const auto& item : lqw)
    //{
    //    printf("%d", item);
    //}
    int16_t i = 0;
    std::cin >> i;
    for (const auto k : TL(i))
    {
        printf("[%d]->", k.Num);
    }
    printf("\n");
    std::vector<int32_t> src{ 1,2,3,4,5,6,7 };
    for (auto& k : FromContainer(std::move(src)))
    {
        printf("[%d]->", k);
    }
    printf("\n");

    /*auto ppy = detail::IteratorSource(std::begin(src), std::end(src));
    decltype(ppy)::OutType;*/

    const auto src2 = src;
    auto d1 = FromIterable(src)
        .Select([](const auto i) { return KK{ i * 2 }; });

    ;
    static_assert(std::is_same_v<decltype(d1)::ProviderType::OutType, KK>, "should be KK");
    static_assert(decltype(d1)::ProviderType::ShouldCache, "should be KK");
    auto d2 = d1.Select([](const KK& p) { return p; });
    /*static_assert(std::is_same_v<decltype(d2.Provider)::OutType, KK>, "should be KK");*/
    //decltype(FromContainer(src).Provider)::OutType;

    return;
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
    std::vector<int> kk;

    TestLinq();
    printf("\n\n");

    TestStacktrace();
    printf("\n\n");

    getchar();
}

