#include "TestRely.h"
#include "common/Linq.hpp"
#include "common/ContainerEx.hpp"

using namespace common::mlog;
using namespace common;
using std::vector;

static MiniLogger<false>& log()
{
    static MiniLogger<false> log(u"LinqTest", { GetConsoleBackend() });
    return log;
}
using namespace common::linq;
using std::string;

template<typename T>
string ToString(const T& data);

template<typename T, size_t Index = 0>
string TupleToString(const T& data, string str = "")
{
    if constexpr (Index == std::tuple_size_v<T>)
    {
        str.pop_back();
        str.back() = ')';
        return str;
    }
    else
        return TupleToString<T, Index + 1>(data, str + ", " + ToString(std::get<Index>(data)));
}

string ToString(const bool data)
{
    return data ? "true" : "false";
}
template<typename T1, typename T2>
string ToString(const std::pair<T1, T2>& data)
{
    return "(" + ToString(data.first) + ", " + ToString(data.second) + ")";
}
template<typename... Ts>
string ToString(const std::tuple<Ts...>& data)
{
    return TupleToString(data);
}
template<typename T>
string ToString(const T& data)
{
    if constexpr (std::is_arithmetic_v<T>)
        return std::to_string(data);
    else if constexpr (common::is_specialization<T, std::vector>::value
        || common::is_specialization<T, std::set>::value
        || common::is_specialization<T, std::list>::value)
    {
        string ret = "[";
        for (const auto& dat : data)
            ret.append(ToString(dat)).append(", ");
        ret.pop_back();
        ret.back() = ']';
        return ret;
    }
    else if constexpr (common::is_specialization<T, std::map>::value
        || common::is_specialization<T, std::unordered_set>::value
        || common::is_specialization<T, std::unordered_map>::value
        || common::is_specialization<T, std::multimap>::value
        || common::is_specialization<T, std::multiset>::value)
    {
        string ret = "{";
        for (const auto& [key, val] : data)
            ret.append(ToString(key)).append(": ").append(ToString(val)).append(", ");
        ret.pop_back();
        ret.back() = '}';
        return ret;
    }
    else
    {
        static_assert(!common::AlwaysTrue<T>(), "not support");
    }
}


static void TestLinq()
{
    std::vector<int> tmp0{ 0,1,2,3,4,5 };
    auto stage0 = Linq::FromIterable(tmp0);
    auto stage1 = Linq::FromIterable(std::vector<int>{ 0, 1, 2, 3, 4, 5 });
    const auto& tmp1 = tmp0;
    auto stage2 = Linq::FromIterable(tmp1);
    const auto ret0 = stage0.ToVector();
    log().info(u"====ret0====\n{}\n", ToString(ret0));
    const auto ret1 = stage1.ToSet<std::less<>>();
    log().info(u"====ret1====\n{}\n", ToString(ret1));
    const auto ret2 = stage2.Contains(5);
    log().info(u"====ret2====\n{}\n", ToString(ret2));
    Linq::FromIterable(tmp0)
        .Skip(1)
        .Select([](int i) { return i + 1; })
        .Where([](int i) { return i % 2 == 1; })
        .ForEach([](const int i) { log().info(u"{}\n", i); });
    const auto ret3 = Linq::FromRange(0, 10, 2)
        .Concat(Linq::FromRange(1, 10, 2))
        .Cast<uint64_t>()
        .Select([](const uint64_t i) { return std::tuple(i / 2, i); })
        .Reduce([](uint64_t& sum, const auto& i) { sum += std::get<1>(i); }, (uint64_t)1);
    log().info(u"====ret3====\n{}\n", ToString(ret3));
    const auto ret4 = Linq::FromRange(0, 4, 1)
        .SelectMany([](const int i) { return Linq::Repeat(i, i); })
        .Pair(Linq::FromRange(0, 4, 1))
        .AsMap();
    log().info(u"====ret4====\n{}\n", ToString(ret4));
    const auto ret5 = Linq::Repeat(5, 50)
        .AllIf([](const auto& i) { return i == 5; });
    log().info(u"====ret5====\n{}\n", ToString(ret5));
    constexpr auto ret6 = Linq::FromRange(5, 1, 1).Empty();
    log().info(u"====ret6====\n{}\n", ToString(ret6));
    const auto ret7 = Linq::FromRange(5, 1, -1)
        .OrderBy<std::less<>>()
        .ToMap<std::multimap<int32_t, int32_t>, false>(
            [](const int32_t i) { return i; },
            [](const int32_t i) { return i; });
    //for (const auto& p : common::container::ValSet(ret7))
    //{
    //    log().info(u"{}", p);
    //}
    log().info(u"====ret7====\n{}\n", ToString(ret7));
    getchar();
}

const static uint32_t ID = RegistTest("LinqTest", &TestLinq);
