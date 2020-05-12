#pragma once

#include "CommonRely.hpp"
#include "StrSIMD.hpp"

#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <algorithm>
#include <array>
#include <optional>

namespace common::str::parsepack
{


template<typename Char, typename Val>
struct ParseItem
{
    std::basic_string_view<Char> Str;
    uint64_t Hash;
    Val Value;
    constexpr ParseItem() noexcept : Hash(0), Value(Val{}) { }
    constexpr ParseItem(const std::basic_string_view<Char> str, Val val) noexcept :
        Str(str), Hash(hash_(str)), Value(val) { }
    constexpr bool operator<(const ParseItem<Char, Val>& item) const noexcept
    {
        return Hash < item.Hash;
    }
    constexpr bool operator<(const uint64_t hash) const noexcept
    {
        return Hash < hash;
    }
};


template<typename Char, typename Val, size_t N>
struct ParsePack
{
    std::array<ParseItem<Char, Val>, N> Cases;
    constexpr std::optional<Val> operator()(const std::basic_string_view<Char> str) const noexcept
    {
        const uint64_t hash = hash_(str);
        size_t left = 0, right = N;
        while (left < right)
        {
            size_t idx = (left + right) / 2;
            if (Cases[idx].Hash < hash)
                left = idx + 1;
            else
                right = idx;
        }
        while (right < N && Cases[right].Hash == hash)
        {
            if (Cases[right].Str == str)
                return Cases[right].Value;
            right++;
        }
        return {};
    }
};
template<typename Char, typename Val, size_t N>
ParsePack(std::array<ParseItem<Char, Val>, N>)->ParsePack<Char, Val, N>;

template<typename Char, typename Val, size_t N, size_t Idx, typename... Args>
constexpr inline void PutItem(std::array<ParseItem<Char, Val>, N>& store, const std::basic_string_view<Char> str, const Val val, const Args... args) noexcept
{
    static_assert(Idx < N);
    store[Idx].Str = str, store[Idx].Value = val, store[Idx].Hash = hash_(str);
    if constexpr (sizeof...(Args) == 0)
        return;
    else
        return PutItem<Char, Val, N, Idx + 1>(store, args...);
}


template<typename Char, typename Val, typename... Args>
constexpr inline auto BuildStore(const std::basic_string_view<Char> str, const Val val, const Args... args) noexcept
{
    static_assert(sizeof...(Args) % 2 == 0, "need pairs");
    constexpr auto N = sizeof...(Args) / 2 + 1;
    std::array<ParseItem<Char, Val>, N> store;
    PutItem<Char, Val, N, 0>(store, str, val, args...);
#if defined(__cpp_lib_constexpr_algorithms) && __cpp_lib_constexpr_algorithms >= 201806L
    std::sort(store.begin(), store.end());
#else
    for (size_t i = 0; i < N - 1; i++)
        for (size_t j = 0; j < N - 1 - i; j++)
            if (store[j + 1] < store[j])
            {
                const auto tmp = store[j];
                store[j] = store[j + 1];
                store[j + 1] = tmp;
            }
#endif
    return store;
}
#define STR_PARSE_PACK_BUILD_STORE(...) [](){ return ::common::str::parsepack::BuildStore(__VA_ARGS__); }


template<typename STORE>
constexpr inline bool CheckUnique(const STORE wrapper) noexcept
{
    const auto store = wrapper();
    for (size_t i = 1; i < store.size(); ++i)
    {
        if (store[i - 1].Str == store[i].Str)
            return false;
    }
    return true;
}
template<typename STORE>
constexpr inline auto BuildPack(const STORE wrapper) noexcept
{
    static_assert(CheckUnique(wrapper), "cannot contain repeat key");
    return ParsePack{ wrapper() };
}

#define PARSE_PACK(...) ::common::str::parsepack::BuildPack(STR_PARSE_PACK_BUILD_STORE(__VA_ARGS__))


using namespace std::string_view_literals;
namespace test
{
constexpr auto txy = PARSE_PACK(U"abc"sv, 101, U"dev", 202, U"axy", 303, U"hellothere", 404, U"dontknowwho", 505);
constexpr auto t01 = txy(U"abc");
constexpr auto t02 = txy(U"dev");
constexpr auto t03 = txy(U"axy");
constexpr auto t04 = txy(U"hellothere");
constexpr auto t05 = txy(U"dontknowwho");
constexpr auto t00 = txy(U"notexists");
}

//#define STR_SWITCH_PACK_CASE(r, name, i, tp) HashCase(arg, BOOST_PP_TUPLE_ELEM(2, 0, tp)) return BOOST_PP_TUPLE_ELEM(2, 1, tp);
#define ConstEvalCase(target, cstr) case hash_(cstr): if (target != cstr) break;
#define STR_SWITCH_PACK_CASE(r, ctype, i, tp) ctype##Case(arg, BOOST_PP_TUPLE_ELEM(2, 0, tp)) return BOOST_PP_TUPLE_ELEM(2, 1, tp);
#define STR_SWITCH_PACK_CASES(ctype, ...) BOOST_PP_SEQ_FOR_EACH_I(STR_SWITCH_PACK_CASE, ctype, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define SWITCH_PACK(ctype, tp, ...) [](){               \
    constexpr auto sv = ::common::str::ToStringView(BOOST_PP_TUPLE_ELEM(2, 0, tp)); \
    using K = decltype(sv);                             \
    using V = decltype(BOOST_PP_TUPLE_ELEM(2, 1, tp));  \
    return [](K arg) -> std::optional<V>                \
    {                                                   \
        switch (hash_(arg))                             \
        {                                               \
        STR_SWITCH_PACK_CASES(ctype, tp, __VA_ARGS__)   \
        default: break;                                 \
        }                                               \
        return {};                                      \
    }; }()                                              \

namespace test
{
constexpr auto fkl = SWITCH_PACK(ConstEval, (U"abc", 101), (U"dev", 202), (U"axy", 303), (U"hellothere", 404), (U"dontknowwho", 505));
constexpr auto f01 = fkl(U"abc");
constexpr auto f02 = fkl(U"dev");
constexpr auto f03 = fkl(U"axy");
constexpr auto f04 = fkl(U"hellothere");
constexpr auto f05 = fkl(U"dontknowwho");
constexpr auto f00 = fkl(U"notexists");
}


}