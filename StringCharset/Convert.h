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
STRCHSETAPI std::string    to_u8string(const char *str, const size_t size, const Charset inchset);
template<typename Char>
STRCHSETAPI std::basic_string<Char> ToULEng(const Char *str, const size_t size, const Charset inchset, const bool isUpper);
}

template<typename Char>
inline std::string to_string(const Char *str, const size_t size, const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    return detail::to_string(reinterpret_cast<const char*>(str), size * sizeof(Char), outchset, inchset);
}
template<typename T>
inline std::enable_if_t<container::ContiguousHelper<T>::IsContiguous, std::string>
    to_string(const T& cont, const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    using Helper = common::container::ContiguousHelper<T>;
    return detail::to_string(reinterpret_cast<const char*>(Helper::Data(cont)), Helper::Count(cont) * Helper::EleSize, outchset, inchset);
}
template<typename Char>
inline std::string to_string(const Char *str, const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    return detail::to_string(reinterpret_cast<const char*>(str), std::char_traits<Char>::length(str) * sizeof(Char), outchset, inchset);
}

template<typename Char>
inline std::u32string to_u32string(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    return detail::to_u32string(reinterpret_cast<const char*>(str), size * sizeof(Char), inchset);
}
template<typename T>
inline std::enable_if_t<container::ContiguousHelper<T>::IsContiguous, std::u32string>
    to_u32string(const T& cont, const Charset inchset = Charset::ASCII)
{
    using Helper = common::container::ContiguousHelper<T>;
    return detail::to_u32string(reinterpret_cast<const char*>(Helper::Data(cont)), Helper::Count(cont) * Helper::EleSize, inchset);
}
template<typename Char>
inline std::u32string to_u32string(const Char *str, const Charset inchset = Charset::ASCII)
{
    return detail::to_u32string(reinterpret_cast<const char*>(str), std::char_traits<Char>::length(str) * sizeof(Char), inchset);
}

template<typename Char>
inline std::u16string to_u16string(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    return detail::to_u16string(reinterpret_cast<const char*>(str), size * sizeof(Char), inchset);
}
template<typename T>
inline std::enable_if_t<container::ContiguousHelper<T>::IsContiguous, std::u16string>
    to_u16string(const T& cont, const Charset inchset = Charset::ASCII)
{
    using Helper = common::container::ContiguousHelper<T>;
    return detail::to_u16string(reinterpret_cast<const char*>(Helper::Data(cont)), Helper::Count(cont) * Helper::EleSize, inchset);
}
template<typename Char>
inline std::u16string to_u16string(const Char *str, const Charset inchset = Charset::ASCII)
{
    return detail::to_u16string(reinterpret_cast<const char*>(str), std::char_traits<Char>::length(str) * sizeof(Char), inchset);
}

template<typename Char>
inline std::string to_u8string(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    return detail::to_u8string(reinterpret_cast<const char*>(str), size * sizeof(Char), inchset);
}
template<typename T>
inline std::enable_if_t<container::ContiguousHelper<T>::IsContiguous, std::string>
    to_u8string(const T& cont, const Charset inchset = Charset::ASCII)
{
    using Helper = common::container::ContiguousHelper<T>;
    return detail::to_u8string(reinterpret_cast<const char*>(Helper::Data(cont)), Helper::Count(cont) * Helper::EleSize, inchset);
}
template<typename Char>
inline std::string to_u8string(const Char *str, const Charset inchset = Charset::ASCII)
{
    return detail::to_u8string(reinterpret_cast<const char*>(str), std::char_traits<Char>::length(str) * sizeof(Char), inchset);
}


template<typename Char>
inline std::basic_string<Char> ToUpperEng(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    return detail::ToULEng(str, size, inchset, true);
}
template<typename T>
inline std::enable_if_t<container::ContiguousHelper<T>::IsContiguous, std::basic_string<typename container::ContiguousHelper<T>::EleType>>
    ToUpperEng(const T& str, const Charset inchset = Charset::ASCII)
{
    return detail::ToULEng(str.data(), str.size(), inchset, true);
}
template<typename Char>
inline std::u32string ToUpperEng(const Char *str, const Charset inchset = Charset::ASCII)
{
    return detail::ToULEng(str, std::char_traits<Char>::length(str), inchset, true);
}

template<typename Char>
inline std::basic_string<Char> ToLowerEng(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    return detail::ToULEng(str, size, inchset, false);
}
template<typename T>
inline std::enable_if_t<container::ContiguousHelper<T>::IsContiguous, std::basic_string<typename container::ContiguousHelper<T>::EleType>>
    ToLowerEng(const T& str, const Charset inchset = Charset::ASCII)
{
    return detail::ToULEng(str.data(), str.size(), inchset, false);
}
template<typename Char>
inline std::u32string ToLowerEng(const Char *str, const Charset inchset = Charset::ASCII)
{
    return detail::ToULEng(str, std::char_traits<Char>::length(str), inchset, false);
}

}

