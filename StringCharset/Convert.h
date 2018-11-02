#pragma once

#include "StringCharsetRely.h"
#include "common/StrBase.hpp"


namespace common::strchset
{
using common::str::Charset;

namespace detail
{
STRCHSETAPI std::string    to_string   (const char *str, const size_t size, const Charset outchset, const Charset inchset);
STRCHSETAPI std::u32string to_u32string(const char *str, const size_t size, const Charset inchset);
STRCHSETAPI std::u16string to_u16string(const char *str, const size_t size, const Charset inchset);
STRCHSETAPI std::string    to_u8string (const char *str, const size_t size, const Charset inchset);
template<typename T>
inline constexpr bool IsDirectString()
{
    return common::is_specialization<T, std::basic_string>::value
        || common::is_specialization<T, std::basic_string_view>::value
        //|| common::is_specialization<T, std::array>::value
        || common::is_specialization<T, std::vector>::value;
}
}

template<typename Char>
inline std::string to_string(const Char *str, const size_t size, const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    return detail::to_string(reinterpret_cast<const char*>(str), size * sizeof(Char), outchset, inchset);
}
template<typename T>
inline std::enable_if_t<detail::IsDirectString<T>(), std::string>
to_string(const T& sv, const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    return detail::to_string(reinterpret_cast<const char*>(sv.data()), sv.size() * sizeof(typename T::value_type), outchset, inchset);
}
template<typename Char>
inline std::u32string to_string(const Char *str, const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    return detail::to_string(str, std::char_traits<Char>::length(str) * sizeof(Char), outchset, inchset);
}

template<typename Char>
inline std::u32string to_u32string(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    return detail::to_u32string(reinterpret_cast<const char*>(str), size * sizeof(Char), inchset);
}
template<typename T>
inline std::enable_if_t<detail::IsDirectString<T>(), std::u32string>
to_u32string(const T& sv, const Charset inchset = Charset::ASCII)
{
    return detail::to_u32string(reinterpret_cast<const char*>(sv.data()), sv.size() * sizeof(typename T::value_type), inchset);
}
template<typename Char>
inline std::u32string to_u32string(const Char *str, const Charset inchset = Charset::ASCII)
{
    return detail::to_u32string(str, std::char_traits<Char>::length(str) * sizeof(Char), inchset);
}

template<typename Char>
inline std::u16string to_u16string(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    return detail::to_u16string(reinterpret_cast<const char*>(str), size * sizeof(Char), inchset);
}
template<typename T>
inline std::enable_if_t<detail::IsDirectString<T>(), std::u16string>
to_u16string(const T& sv, const Charset inchset = Charset::ASCII)
{
    return detail::to_u16string(reinterpret_cast<const char*>(sv.data()), sv.size() * sizeof(typename T::value_type), inchset);
}
template<typename Char>
inline std::u16string to_u16string(const Char *str, const Charset inchset = Charset::ASCII)
{
    return detail::to_u16string(str, std::char_traits<Char>::length(str) * sizeof(Char), inchset);
}

template<typename Char>
inline std::string to_u8string(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    return detail::to_u8string(reinterpret_cast<const char*>(str), size * sizeof(Char), inchset);
}
template<typename T>
inline std::enable_if_t<detail::IsDirectString<T>(), std::string>
to_u8string(const T& sv, const Charset inchset = Charset::ASCII)
{
    return detail::to_u8string(reinterpret_cast<const char*>(sv.data()), sv.size() * sizeof(typename T::value_type), inchset);
}
template<typename Char>
inline std::string to_u8string(const Char *str, const Charset inchset = Charset::ASCII)
{
    return detail::to_u8string(str, std::char_traits<Char>::length(str) * sizeof(Char), inchset);
}

}

