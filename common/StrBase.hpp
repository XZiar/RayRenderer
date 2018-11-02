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
        (std::is_convertible_v<const T&, const std::basic_string_view<Char>&> ||
            std::is_convertible_v<const T&, const std::basic_string<Char>&>
        )
    )
>;


template<typename Char, typename T, class = CanBeStringView<T, Char>>
inline constexpr std::basic_string_view<Char> ToStringView(const T& str)
{
    if constexpr (std::is_convertible_v<const T&, const std::basic_string_view<Char>&>)
        return str;
    else if constexpr (std::is_convertible_v<const T&, const std::basic_string<Char>&>)
        return static_cast<const std::basic_string<Char>&>(str);
    else if constexpr (std::is_convertible_v<const T&, const Char*>)
        return static_cast<const Char*>(str);
    else
        static_assert(AlwaysTrue<Char>(), "cannot be cast to string_view");
}

template<typename Char, typename A>
inline constexpr std::basic_string_view<Char> ToStringView(const std::vector<Char, A>& str) noexcept
{
    return std::basic_string_view<Char>(str.data(), str.size());
}


}

enum class Charset { ASCII, UTF7 = ASCII, GB18030, UTF8, UTF16LE, UTF16BE, UTF32 };

inline constexpr Charset toCharset(const std::string_view chset)
{
    switch (hash_(chset))
    {
    case "GB18030"_hash:
        return Charset::GB18030;
    case "UTF-8"_hash:
        return Charset::UTF8;
    case "UTF-16LE"_hash:
        return Charset::UTF16LE;
    case "UTF-16BE"_hash:
        return Charset::UTF16BE;
    case "error"_hash:
        return Charset::ASCII;
    default:
        return Charset::ASCII;
    }
}

inline constexpr std::string_view getCharsetName(const Charset chset)
{
    using namespace std::string_view_literals;
    switch (chset)
    {
    case Charset::ASCII:
        return "ASCII"sv;
    case Charset::GB18030:
        return "GB18030"sv;
    case Charset::UTF8:
        return "UTF-8"sv;
    case Charset::UTF16BE:
        return "UTF-16BE"sv;
    case Charset::UTF16LE:
        return "UTF-16LE"sv;
    case Charset::UTF32:
        return "UTF-32"sv;
    default:
        return "error"sv;
    }
}



}