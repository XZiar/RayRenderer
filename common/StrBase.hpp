#pragma once


#include <string_view>
#include <type_traits>

namespace common::str
{

template<typename T>
[[nodiscard]] inline constexpr auto ToStringView(T&& val) noexcept
{
    using U = std::decay_t<T>;
    if constexpr (std::is_pointer_v<U>)
        return std::basic_string_view(val);
    else if constexpr (common::has_valuetype_v<U>)
    {
        using Char = typename U::value_type;
        if constexpr (std::is_constructible_v<std::basic_string_view<Char>, T>)
            return std::basic_string_view<Char>(std::forward<T>(val));
        else if constexpr (std::is_convertible_v<T, std::basic_string_view<Char>>)
            return (std::basic_string_view<Char>)val;
        else if constexpr (std::is_constructible_v<common::span<const Char>, T>)
        {
            common::span<const Char> space(std::forward<T>(val));
            return std::basic_string_view<Char>(space.data(), space.size());
        }
        else if constexpr (std::is_convertible_v<T, common::span<const Char>>)
        {
            auto space = (common::span<const Char>)val;
            return std::basic_string_view<Char>(space.data(), space.size());
        }
        else
            static_assert(!common::AlwaysTrue<T>(), "with value_type, still not able to be converted into string_view");
    }
    else
        static_assert(!common::AlwaysTrue<T>(), "unsupported type to be converted into string_view");
}


enum class Charset { ASCII, UTF7 = ASCII, GB18030, UTF8, UTF16LE, UTF16BE, UTF32LE, UTF32BE };

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
    case "UTF-32LE"_hash:
        return Charset::UTF32LE;
    case "UTF-32BE"_hash:
        return Charset::UTF32BE;
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
    case Charset::UTF32LE:
        return "UTF-32LE"sv;
    case Charset::UTF32BE:
        return "UTF-32BE"sv;
    default:
        return "error"sv;
    }
}



}