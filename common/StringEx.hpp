#pragma once

#include "CommonRely.hpp"
#include "StrBase.hpp"
#include <optional>
#include <cctype>
#include <cwctype>


namespace common
{

namespace str
{

template<typename Char>
[[nodiscard]] inline constexpr std::basic_string_view<Char> TrimStringView(std::basic_string_view<Char> sv, const Char obj) noexcept
{
    const auto pStart = sv.find_first_not_of(obj);
    if (pStart == sv.npos)
        return {};
    sv.remove_prefix(pStart);
    const auto pEnd = sv.find_last_not_of(obj); // ensured not npos
    return sv.substr(0, pEnd + 1);
}

namespace detail
{
template<typename Char>
inline constexpr std::basic_string_view<Char> GetTrimEndSv() noexcept
{
    using namespace std::string_view_literals;
         if constexpr (std::is_same_v<Char, char>)     return   "\0 "sv;
    else if constexpr (std::is_same_v<Char, wchar_t>)  return  L"\0 "sv;
    else if constexpr (std::is_same_v<Char, char16_t>) return  u"\0 "sv;
    else if constexpr (std::is_same_v<Char, char32_t>) return  U"\0 "sv;
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    else if constexpr (std::is_same_v<Char, char8_t>)  return u8"\0 "sv;
#endif
    else static_assert(!AlwaysTrue<Char>, "unsupportted char type");

}
}

// trim space and null-terminator
template<typename Char>
[[nodiscard]] inline constexpr std::basic_string_view<Char> TrimStringView(std::basic_string_view<Char> sv) noexcept
{
    const auto pStart = sv.find_first_not_of(static_cast<Char>(' '));
    if (pStart == sv.npos)
        return {};
    const auto pEnd = sv.find_last_not_of(detail::GetTrimEndSv<Char>());
    if (pEnd == sv.npos)
        return {};
    return sv.substr(pStart, pEnd - pStart + 1);
}

// resize to null-terminated and trim trailing space
template<typename Char>
inline void TrimString(std::basic_string<Char>& str) noexcept
{
    const auto pStart = str.find_first_not_of(static_cast<Char>(' '));
    if (pStart == str.npos)
        str.clear();
    else
    {
        const auto pEnd = str.find_last_not_of(detail::GetTrimEndSv<Char>());
        if (pEnd == str.npos)
            str.clear();
        else if (pStart == 0)
            str.resize(pEnd + 1);
        else
            str = str.substr(pStart, pEnd - pStart + 1);
    }
}

// trim some pairs of chars
template<typename Char>
[[nodiscard]] inline constexpr std::basic_string_view<Char> TrimPairStringView(std::basic_string_view<Char> sv, const Char obj) noexcept
{
    const auto pStart = sv.find_first_not_of(obj);
    if (pStart == sv.npos) // all are obj or empty
        return sv.substr(sv.size() / 2, sv.empty() ? 0 : 1);
    const auto pEnd = sv.find_last_not_of(obj); // ensured not npos
    const auto count = std::min(pStart, sv.size() - pEnd - 1);
    return sv.substr(count, sv.size() - 2 * count);
}

template<typename Char>
inline size_t CutStringViewPrefix(std::basic_string_view<Char>& sv, const Char obj)
{
    const auto pos = sv.find_first_of(obj);
    if (pos != std::basic_string_view<Char>::npos)
        sv.remove_prefix(sv.size() - pos);
    return pos;
}
template<typename Char>
inline size_t CutStringViewSuffix(std::basic_string_view<Char>& sv, const Char obj)
{
    const auto pos = sv.find_first_of(obj);
    if (pos != std::basic_string_view<Char>::npos)
        sv.remove_suffix(sv.size() - pos);
    return pos;
}


template<typename Char>
inline std::basic_string<Char> ReplaceStr(const std::basic_string<Char>& str, const std::basic_string_view<Char>& obj, const std::basic_string_view<Char>& newstr)
{
    const auto pos = str.find(obj);
    if (pos == std::basic_string<Char>::npos)
        return str;
    std::basic_string<Char> ret;
    ret.reserve(str.size() - obj.size() + newstr.size());
    const auto posend = pos + obj.size();
    return ret.append(str, 0, pos).append(newstr).append(str, posend);
}



template<typename T1, typename T2>
inline constexpr bool IsBeginWith(const T1& src, const T2& prefix) noexcept
{
    const auto srcsv    = ToStringView(src);
    const auto prefixsv = ToStringView(prefix);
    static_assert(std::is_same_v<decltype(srcsv), decltype(prefixsv)>, "src and prefix has different char type");
    using Char = typename decltype(srcsv)::value_type;
    if (srcsv.length() < prefixsv.length())
        return false;
    return std::char_traits<Char>::compare(srcsv.data(), prefixsv.data(), prefixsv.length()) == 0;
}

template<typename T1, typename T2>
inline constexpr bool IsEndWith(const T1& src, const T2& prefix) noexcept
{
    const auto srcsv = ToStringView(src);
    const auto prefixsv = ToStringView(prefix);
    static_assert(std::is_same_v<decltype(srcsv), decltype(prefixsv)>, "src and prefix has different char type");
    using Char = typename decltype(srcsv)::value_type;
    if (srcsv.length() < prefixsv.length())
        return false;
    return std::char_traits<Char>::compare(srcsv.data() + (srcsv.size() - prefixsv.size()), prefixsv.data(), prefixsv.length()) == 0;
}


inline char ToUpper(const char ch)
{
    return (char)std::toupper(ch);
}
inline wchar_t ToUpper(const wchar_t ch)
{
    return (wchar_t)std::towupper(ch);
}
inline std::string ToUpper(const std::string& src)
{
    std::string ret;
    ret.reserve(src.length());
    for (const auto ch : src)
        ret.push_back((char)std::toupper(ch));
    return ret;
}
inline std::wstring ToUpper(const std::wstring& src)
{
    std::wstring ret;
    ret.reserve(src.length());
    for (const auto ch : src)
        ret.push_back((wchar_t)std::towupper(ch));
    return ret;
}

namespace detail
{

inline bool upperComp(const char ch1, const char ch2)
{
    return (char)std::toupper(ch1) == ch2;
}
inline bool upperComp(const wchar_t ch1, const wchar_t ch2)
{
    return (wchar_t)std::towupper(ch1) == ch2;
}

}
template<class T, typename charT = typename T::value_type, class itT = typename T::const_iterator>
inline std::optional<itT> ifind_first(T src, const std::basic_string<charT>& obj)
{
    const size_t srclen = src.length(), objlen = obj.length();
    if (srclen < objlen)
        return {};
    const size_t morelen = srclen - objlen;
    for (size_t a = 0; a < objlen; ++a)
        src[a] = ToUpper(src[a]);
    auto upobj = ToUpper(obj);
    for (size_t a = 0; ; ++a)
    {
        if (src.compare(a, objlen, upobj) == 0)
        {
            auto it = src.cbegin();
            std::advance(it, a);
            return it;
        }
        if (a >= morelen)
            break;
        src[objlen + a] = ToUpper(src[objlen + a]);
    }
    return {};
}
template<class T, typename charT = typename T::value_type, class itT = typename T::const_iterator, size_t N>
inline std::optional<itT> ifind_first(const T& src, const charT(&obj)[N])
{
    return ifind_first(src, std::basic_string<charT>(obj));
}
}


}


