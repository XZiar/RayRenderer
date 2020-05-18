#pragma once

#include "CommonRely.hpp"
#include "StrBase.hpp"
#include "SIMD.hpp"

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#if defined(__cpp_lib_constexpr_algorithms) && __cpp_lib_constexpr_algorithms >= 201806L
#   include <algorithm>
#endif
#include <array>
#include <optional>

namespace common::str::parsepack
{


template<typename T, size_t N>
forceinline bool ShortCompare(const std::basic_string_view<T> src, const T(&target)[N]) noexcept
{
    constexpr size_t len = N - 1;
    if (src.size() != len)
        return false;
    [[maybe_unused]] constexpr size_t targetSBytes = len * sizeof(T);
    if constexpr (targetSBytes <= (8 * 8u) && len <= 4)
    {
        for (size_t i = 0; i < len; ++i)
            if (src[i] != target[i])
                return false;
        return true;
    }
    else if constexpr (targetSBytes <= 128u / 8u)
    {
#if COMMON_SIMD_LV >= 20
        const auto mask = (1u << targetSBytes) - 1u;
        const auto vecT = _mm_loadu_si128(reinterpret_cast<const __m128i*>(target));
        const auto vecS = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src.data()));
        const auto cmpbits = (uint32_t)_mm_movemask_epi8(_mm_cmpeq_epi8(vecS, vecT));
        return (cmpbits & mask) == mask;
#else
        return src == target;
#endif
    }
    else if constexpr (targetSBytes <= 256u / 8u)
    {
#if COMMON_SIMD_LV >= 200
        const auto mask = static_cast<uint32_t>((uint64_t(1) << targetSBytes) - 1u);
        const auto vecT = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(target));
        const auto vecS = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src.data()));
        const auto cmpbits = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(vecS, vecT)));
        return (cmpbits & mask) == mask;
#elif COMMON_SIMD_LV >= 20
        const auto mask = static_cast<uint32_t>((uint64_t(1) << targetSBytes) - 1u);
        const auto vecT0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(target) + 0);
        const auto vecT1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(target) + 1);
        const auto vecS0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src.data()) + 0);
        const auto vecS1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src.data()) + 1);
        const auto cmpbits0 = static_cast<uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(vecS0, vecT0)));
        const auto cmpbits1 = static_cast<uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(vecS1, vecT1)));
        return cmpbits0 == 0xffffffffu && (cmpbits1 & mask) == mask;
#else
        return src == target;
#endif
    }
    else
        return src == target;
}


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


#define STR_SWITCH_PACK_CASE(r, ctype, tp) ctype##Case(arg, BOOST_PP_TUPLE_ELEM(2, 0, tp)) return BOOST_PP_TUPLE_ELEM(2, 1, tp);
#define STR_SWITCH_PACK_CASES(ctype, ...) BOOST_PP_SEQ_FOR_EACH(STR_SWITCH_PACK_CASE, ctype, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

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


#define HashCase(target, cstr)      case hash_(cstr): if (!::common::str::parsepack::ShortCompare(target, cstr)) break;
#define ConstEvalCase(target, cstr) case hash_(cstr): if (target != cstr) break;

}