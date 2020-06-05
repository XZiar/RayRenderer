#pragma once

#include "StringCharsetRely.h"
#include "common/StrBase.hpp"


namespace common::strchset
{
using common::str::Charset;
using common::str::u8string;


namespace detail
{
[[nodiscard]] STRCHSETAPI std::string    to_string   (const common::span<const std::byte> data, const Charset outchset, const Charset inchset);
[[nodiscard]] STRCHSETAPI std::u16string to_u16string(const common::span<const std::byte> data, const Charset inchset);
[[nodiscard]] STRCHSETAPI std::u32string to_u32string(const common::span<const std::byte> data, const Charset inchset);
[[nodiscard]] STRCHSETAPI str::u8string  to_u8string (const common::span<const std::byte> data, const Charset inchset);
template<typename Char>
[[nodiscard]] STRCHSETAPI std::basic_string<Char> ToUEng(const std::basic_string_view<Char> str, const Charset inchset);
template<typename Char>
[[nodiscard]] STRCHSETAPI std::basic_string<Char> ToLEng(const std::basic_string_view<Char> str, const Charset inchset);
}



template<typename T>
[[nodiscard]] inline std::string to_string(const T& cont, const Charset outchset = Charset::ASCII, 
    const Charset inchset = detail::GetDefaultCharset<T>())
{
    return detail::to_string(detail::ToByteSpan(cont), outchset, inchset);
}
template<typename T>
[[nodiscard]] inline str::u8string to_u8string(const T& cont,
    const Charset inchset = detail::GetDefaultCharset<T>())
{
    return detail::to_u8string(detail::ToByteSpan(cont), inchset);
}
template<typename T>
[[nodiscard]] inline std::u16string to_u16string(const T& cont,
    const Charset inchset = detail::GetDefaultCharset<T>())
{
    return detail::to_u16string(detail::ToByteSpan(cont), inchset);
}
template<typename T>
[[nodiscard]] inline std::u32string to_u32string(const T& cont,
    const Charset inchset = detail::GetDefaultCharset<T>())
{
    return detail::to_u32string(detail::ToByteSpan(cont), inchset);
}


template<typename Char>
[[nodiscard]] inline std::string to_string(const Char* str, const size_t len, const Charset outchset = Charset::ASCII,
    const Charset inchset = common::str::DefaultCharset<Char>)
{
    return detail::to_string(detail::ToByteSpan(common::span<const Char>(str, len)), outchset, inchset);
}
template<typename Char>
[[nodiscard]] inline str::u8string to_u8string(const Char* str, const size_t len,
    const Charset inchset = common::str::DefaultCharset<Char>)
{
    return detail::to_u8string(detail::ToByteSpan(common::span<const Char>(str, len)), inchset);
}
template<typename Char>
[[nodiscard]] inline std::u16string to_u16string(const Char* str, const size_t len,
    const Charset inchset = common::str::DefaultCharset<Char>)
{
    return detail::to_u16string(detail::ToByteSpan(common::span<const Char>(str, len)), inchset);
}
template<typename Char>
[[nodiscard]] inline std::u32string to_u32string(const Char* str, const size_t len,
    const Charset inchset = common::str::DefaultCharset<Char>)
{
    return detail::to_u32string(detail::ToByteSpan(common::span<const Char>(str, len)), inchset);
}



template<typename T>
[[nodiscard]] inline auto ToUpperEng(const T& str,
    const Charset inchset = detail::GetDefaultCharset<T>())
{
    return detail::ToUEng(str::ToStringView(str), inchset);
}
template<typename Char>
[[nodiscard]] inline std::basic_string<Char> ToUpperEng(const Char* str, const size_t size,
    const Charset inchset = common::str::DefaultCharset<Char>)
{
    return detail::ToUEng(std::basic_string_view<Char>(str, size), inchset);
}


template<typename T>
[[nodiscard]] inline auto ToLowerEng(const T& str,
    const Charset inchset = detail::GetDefaultCharset<T>())
{
    return detail::ToLEng(str::ToStringView(str), inchset);
}
template<typename Char>
[[nodiscard]] inline std::basic_string<Char> ToLowerEng(const Char* str, const size_t size,
    const Charset inchset = common::str::DefaultCharset<Char>)
{
    return detail::ToLEng(std::basic_string_view<Char>(str, size), inchset);
}


}

