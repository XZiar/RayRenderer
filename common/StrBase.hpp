#pragma once

#include "CommonRely.hpp"
#include <string>
#include <string_view>
#include <type_traits>

namespace common::str
{
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
using u8string = std::u8string;
using u8string_view = std::u8string_view;
#else
using u8string = std::string;
using u8string_view = std::string_view;
#endif


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
            static_assert(!common::AlwaysTrue<T>, "with value_type, still not able to be converted into string_view");
    }
    else
        static_assert(!common::AlwaysTrue<T>, "unsupported type to be converted into string_view");
}

template<typename Ch>
struct StrVariant
{
private:
    std::basic_string<Ch> Str;
    std::basic_string_view<Ch> View;
public:
    constexpr StrVariant() noexcept { }
    StrVariant(std::basic_string<Ch>&& str) noexcept : Str(std::move(str)), View(Str) { }
    constexpr StrVariant(const std::basic_string<Ch>& str) noexcept : View(str) { }
    constexpr StrVariant(const std::basic_string_view<Ch> str) noexcept : View(str) { }
    StrVariant(const StrVariant<Ch>& other) noexcept :
        Str(other.Str), View(Str.empty() ? other.View : Str) { }
    constexpr StrVariant(StrVariant<Ch>&& other) noexcept : 
        Str(std::move(other.Str)), View(Str.empty() ? other.View : Str) 
    {
        other.View = {};
    }

    StrVariant& operator=(const StrVariant<Ch>& other) noexcept
    {
        View = {};
        Str = other.Str;
        if (!Str.empty())
            View = Str;
        return *this;
    }
    StrVariant& operator=(StrVariant<Ch>&& other) noexcept
    {
        View = {};
        Str = std::move(other.Str);
        if (!Str.empty())
            View = Str;
        other.View = {};
        return *this;
    }

    [[nodiscard]] constexpr std::basic_string_view<Ch> StrView() const noexcept
    {
        return View;
    }
    constexpr operator common::span<const Ch>() const noexcept
    {
        return common::span<const Ch>(View.data(), View.size());
    }
    constexpr bool operator==(const std::basic_string_view<Ch> str) const noexcept
    {
        return View == str;
    }

    decltype(auto) begin() const noexcept
    {
        return View.begin();
    }
    decltype(auto) end() const noexcept
    {
        return View.end();
    }
    decltype(auto) data() const noexcept
    {
        return View.data();
    }
    decltype(auto) size() const noexcept
    {
        return View.size();
    }

};


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