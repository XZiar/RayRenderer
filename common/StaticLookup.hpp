#pragma once

#include "CommonRely.hpp"
#include "StrBase.hpp"
#include <cstdint>
#include <optional>
#include <tuple>
#include <array>
#if defined(__cpp_lib_constexpr_algorithms) && __cpp_lib_constexpr_algorithms >= 201806L
#   include <algorithm>
#endif
#include <type_traits>
#include <utility>

namespace common
{

template<typename T>
inline constexpr void SortAscending(T* ptr, size_t count) noexcept
{
#if defined(__cpp_lib_constexpr_algorithms) && __cpp_lib_constexpr_algorithms >= 201806L
    std::sort(ptr, ptr + count);
#else
    for (size_t i = 0; i < count - 1; i++)
        for (size_t j = 0; j < count - 1 - i; j++)
            if (ptr[j + 1] < ptr[j])
            {
                const auto tmp = ptr[j];
                ptr[j] = ptr[j + 1];
                ptr[j + 1] = tmp;
            }
#endif
}

namespace detail
{
template<typename K, typename V>
struct StaticLookupItem
{
    K Key;
    V Value;
    constexpr StaticLookupItem() noexcept : Key{}, Value(V{}) { }
    constexpr StaticLookupItem(K key, V val) noexcept : Key(key), Value(val) { }
    constexpr bool operator<(const StaticLookupItem<K, V>& item) const noexcept
    {
        return Key < item.Key;
    }
    constexpr bool operator<(const K key) const noexcept
    {
        return Key < key;
    }
    template<size_t N>
    static constexpr bool CheckUnique(const std::pair<K,V> (&items)[N]) noexcept
    {
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < i; ++j)
                if (items[i].first == items[j].first)
                    return false;
        return true;
    }
};

template<typename K, typename V, size_t N>
struct StaticLookupTable
{
    std::array<detail::StaticLookupItem<K, V>, N> Items = {};
    template<typename T>
    constexpr std::optional<V> operator()(const T& key) const noexcept
    {
        const auto keyVal = static_cast<K>(key);
        size_t left = 0, right = N;
        while (left < right)
        {
            size_t idx = (left + right) / 2;
            if (Items[idx].Key < keyVal)
                left = idx + 1;
            else
                right = idx;
        }
        if (right < N && Items[right].Key == keyVal)
        {
            return Items[right].Value;
        }
        return {};
    }
};

template<typename Type>
constexpr inline auto BuildTableStore(Type&&) noexcept
{
    constexpr Type Dummy;
    using T = std::remove_reference_t<decltype(Dummy.Data)>;
    using U = remove_cvref_t<std::remove_extent_t<T>>;
    constexpr auto N = std::extent_v<T>;
    static_assert(is_specialization<U, std::pair>::value, "elements should be std::pair");
    using K = typename U::first_type;
    using V = typename U::second_type;
    static_assert(detail::StaticLookupItem<K, V>::CheckUnique(Dummy.Data), "cannot contain repeat key");
    StaticLookupTable<K, V, N> table;
    for (size_t i = 0; i < N; ++i)
    {
        table.Items[i].Key   = Dummy.Data[i].first;
        table.Items[i].Value = Dummy.Data[i].second;
    }
    SortAscending(table.Items.data(), N);
    return table;
}


template<typename NK, typename NV, typename K, typename V, size_t N>
constexpr inline auto BuildTableStoreFrom(const std::pair<K, V> (&arr)[N]) noexcept
{
    StaticLookupTable<NK, NV, N> table;
    for (size_t i = 0; i < N; ++i)
    {
        table.Items[i].Key   = NK(arr[i].first);
        table.Items[i].Value = NV(arr[i].second);
    }
    SortAscending(table.Items.data(), N);
    return table;
}


}

#if 0
#define BuildStaticLookup(k, v, ...)                    \
::common::detail::BuildTableStore([]()                  \
{                                                       \
    constexpr std::pair<k, v> tmp[] = { __VA_ARGS__ };  \
    constexpr size_t M = std::extent_v<decltype(tmp)>;  \
    struct Type                                         \
    {                                                   \
        std::pair<k, v> Data[M] = { __VA_ARGS__ };      \
    };                                                  \
    return Type{};                                      \
}())
#endif

#define BuildStaticLookupFrom(k, v, src) []()                       \
{                                                                   \
    using K = decltype(src[0].first);                               \
    using V = decltype(src[0].second);                              \
    static_assert(::common::detail::StaticLookupItem<K, V>::        \
        CheckUnique(src), "cannot contain repeat key");             \
    return ::common::detail::BuildTableStoreFrom<k, v, K, V>(src);  \
}()

#define BuildStaticLookup(k, v, ...) []()                           \
{                                                                   \
    constexpr std::pair<k, v> tmp[] = { __VA_ARGS__ };              \
    static_assert(::common::detail::StaticLookupItem<k, v>::        \
        CheckUnique(tmp), "cannot contain repeat key");             \
    return ::common::detail::BuildTableStoreFrom<k, v, k, v>(tmp);  \
}()

}