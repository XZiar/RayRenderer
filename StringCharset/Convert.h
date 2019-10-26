#pragma once

#include "StringCharsetRely.h"
#include "common/StrBase.hpp"


namespace common::strchset
{
using common::str::Charset;

namespace detail
{
STRCHSETAPI [[nodiscard]] std::string    to_string   (const common::span<const std::byte> data, const Charset outchset, const Charset inchset);
STRCHSETAPI [[nodiscard]] std::u32string to_u32string(const common::span<const std::byte> data, const Charset inchset);
STRCHSETAPI [[nodiscard]] std::u16string to_u16string(const common::span<const std::byte> data, const Charset inchset);
STRCHSETAPI [[nodiscard]] std::string    to_u8string (const common::span<const std::byte> data, const Charset inchset);
template<typename Char>
STRCHSETAPI [[nodiscard]] std::basic_string<Char> ToULEng(const std::basic_string_view<Char> str, const Charset inchset, const bool isUpper);
}


template<typename T>
[[nodiscard]] inline std::string to_string(const T& cont,                       const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    return detail::to_string(detail::ToByteSpan(cont), outchset, inchset);
}
template<typename Char>
[[nodiscard]] inline std::string to_string(const Char *str, const size_t size,  const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    return to_string(common::span<const Char>(str, size), outchset, inchset);
}


template<typename T>
[[nodiscard]] inline std::u32string to_u32string(const T& cont,                         const Charset inchset = Charset::ASCII)
{
    return detail::to_u32string(detail::ToByteSpan(cont), inchset);
}
template<typename Char>
[[nodiscard]] inline std::u32string to_u32string(const Char* str, const size_t size,    const Charset inchset = Charset::ASCII)
{
    return to_u32string(common::span<const Char>(str, size), inchset);
}


template<typename T>
[[nodiscard]] inline std::u16string to_u16string(const T& cont,                         const Charset inchset = Charset::ASCII)
{
    return detail::to_u16string(detail::ToByteSpan(cont), inchset);
}
template<typename Char>
[[nodiscard]] inline std::u16string to_u16string(const Char* str, const size_t size,    const Charset inchset = Charset::ASCII)
{
    return to_u16string(common::span<const Char>(str, size), inchset);
}


template<typename T>
[[nodiscard]] inline std::string to_u8string(const T& cont,                         const Charset inchset = Charset::ASCII)
{
    return detail::to_u8string(detail::ToByteSpan(cont), inchset);
}
template<typename Char>
[[nodiscard]] inline std::string to_u8string(const Char* str, const size_t size,    const Charset inchset = Charset::ASCII)
{
    return to_u8string(common::span<const Char>(str, size), inchset);
}



template<typename T>
[[nodiscard]] inline auto ToUpperEng(const T& str,                                          const Charset inchset = Charset::ASCII)
{
    return detail::ToULEng(detail::ToStringView(str), inchset, true);
}
template<typename Char>
[[nodiscard]] inline std::basic_string<Char> ToUpperEng(const Char* str, const size_t size, const Charset inchset = Charset::ASCII)
{
    return detail::ToULEng(std::basic_string_view<Char>(str, size), inchset, true);
}


template<typename T>
[[nodiscard]] inline auto ToLowerEng(const T& str,                                          const Charset inchset = Charset::ASCII)
{
    return detail::ToULEng(detail::ToStringView(str), inchset, false);
}
template<typename Char>
[[nodiscard]] inline std::basic_string<Char> ToLowerEng(const Char* str, const size_t size, const Charset inchset = Charset::ASCII)
{
    return detail::ToULEng(std::basic_string_view<Char>(str, size), inchset, false);
}


}

