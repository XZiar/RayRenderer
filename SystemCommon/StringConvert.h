#pragma once

#include "SystemCommonRely.h"


namespace common::str
{


namespace detail
{
SYSCOMMONAPI [[nodiscard]] std::string    to_string(const common::span<const std::byte> data, const Encoding outchset, const Encoding inchset);
SYSCOMMONAPI [[nodiscard]] std::u16string to_u16string(const common::span<const std::byte> data, const Encoding inchset);
SYSCOMMONAPI [[nodiscard]] std::u32string to_u32string(const common::span<const std::byte> data, const Encoding inchset);
SYSCOMMONAPI [[nodiscard]] str::u8string  to_u8string (const common::span<const std::byte> data, const Encoding inchset);
template<typename Char>
SYSCOMMONAPI [[nodiscard]] std::basic_string<Char> ToUEng(const std::basic_string_view<Char> str, const Encoding inchset);
template<typename Char>
SYSCOMMONAPI [[nodiscard]] std::basic_string<Char> ToLEng(const std::basic_string_view<Char> str, const Encoding inchset);
}



template<typename T>
[[nodiscard]] inline std::string to_string(const T& cont, const Encoding outchset = Encoding::ASCII, 
    const Encoding inchset = detail::GetDefaultEncoding<T>())
{
    return detail::to_string(detail::ToByteSpan(cont), outchset, inchset);
}
template<typename T>
[[nodiscard]] inline str::u8string to_u8string(const T& cont,
    const Encoding inchset = detail::GetDefaultEncoding<T>())
{
    return detail::to_u8string(detail::ToByteSpan(cont), inchset);
}
template<typename T>
[[nodiscard]] inline std::u16string to_u16string(const T& cont,
    const Encoding inchset = detail::GetDefaultEncoding<T>())
{
    return detail::to_u16string(detail::ToByteSpan(cont), inchset);
}
template<typename T>
[[nodiscard]] inline std::u32string to_u32string(const T& cont,
    const Encoding inchset = detail::GetDefaultEncoding<T>())
{
    return detail::to_u32string(detail::ToByteSpan(cont), inchset);
}


template<typename Char>
[[nodiscard]] inline std::string to_string(const Char* str, const size_t len, const Encoding outchset = Encoding::ASCII,
    const Encoding inchset = common::str::DefaultEncoding<Char>)
{
    return detail::to_string(detail::ToByteSpan(common::span<const Char>(str, len)), outchset, inchset);
}
template<typename Char>
[[nodiscard]] inline str::u8string to_u8string(const Char* str, const size_t len,
    const Encoding inchset = common::str::DefaultEncoding<Char>)
{
    return detail::to_u8string(detail::ToByteSpan(common::span<const Char>(str, len)), inchset);
}
template<typename Char>
[[nodiscard]] inline std::u16string to_u16string(const Char* str, const size_t len,
    const Encoding inchset = common::str::DefaultEncoding<Char>)
{
    return detail::to_u16string(detail::ToByteSpan(common::span<const Char>(str, len)), inchset);
}
template<typename Char>
[[nodiscard]] inline std::u32string to_u32string(const Char* str, const size_t len,
    const Encoding inchset = common::str::DefaultEncoding<Char>)
{
    return detail::to_u32string(detail::ToByteSpan(common::span<const Char>(str, len)), inchset);
}



template<typename T>
[[nodiscard]] inline auto ToUpperEng(const T& str,
    const Encoding inchset = detail::GetDefaultEncoding<T>())
{
    return detail::ToUEng(str::ToStringView(str), inchset);
}
template<typename Char>
[[nodiscard]] inline std::basic_string<Char> ToUpperEng(const Char* str, const size_t size,
    const Encoding inchset = common::str::DefaultEncoding<Char>)
{
    return detail::ToUEng(std::basic_string_view<Char>(str, size), inchset);
}


template<typename T>
[[nodiscard]] inline auto ToLowerEng(const T& str,
    const Encoding inchset = detail::GetDefaultEncoding<T>())
{
    return detail::ToLEng(str::ToStringView(str), inchset);
}
template<typename Char>
[[nodiscard]] inline std::basic_string<Char> ToLowerEng(const Char* str, const size_t size,
    const Encoding inchset = common::str::DefaultEncoding<Char>)
{
    return detail::ToLEng(std::basic_string_view<Char>(str, size), inchset);
}


}

