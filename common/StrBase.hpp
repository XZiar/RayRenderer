#pragma once


#include <string>
#include <string_view>
#include <vector>
#include <type_traits>

namespace common::str
{

namespace detail
{

template <typename Char, class T>
inline constexpr bool is_str_vector_v()
{
    if constexpr(TemplateDerivedHelper<T, std::vector>::IsDerivedFromBase)
        return std::is_same_v<Char, typename T::value_type>;
    return false;
}

template<typename T, typename Char>
using CanBeStringView = std::enable_if_t<
    is_str_vector_v<Char, T>() ||
    (!std::is_convertible_v<const T&, const Char*> &&
        (std::is_convertible_v<const T&, std::basic_string_view<Char>> ||
            std::is_convertible_v<const T&, std::basic_string<Char>>
        )
    )
>;


template<typename Char, typename T, class = CanBeStringView<T, Char>>
constexpr inline std::basic_string_view<Char> ToStringView(const T& str)
{
    if constexpr(std::is_convertible_v<const T&, std::basic_string_view<Char>>)
        return str;
    else
        return std::basic_string<Char>(str);
}

template<typename Char, typename A>
constexpr inline std::basic_string_view<Char> ToStringView(const std::vector<Char, A>& str)
{
    return std::basic_string_view<Char>(str.data(), str.size());
}


}


}