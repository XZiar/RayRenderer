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
inline std::basic_string_view<Char> TrimStringView(const std::basic_string_view<Char> sv, const Char obj)
{
    const auto pStart = sv.find_first_not_of(obj);
    if (pStart == sv.npos)
        return {};
    const auto pEnd = sv.find_last_not_of(obj);
    return sv.substr(pStart, pEnd - pStart + 1);
}

template<typename Char>
inline std::basic_string_view<Char> TrimPairStringView(const std::basic_string_view<Char> sv, const Char front, const Char back)
{
    const auto pStart = sv.find_first_not_of(front);
    if (pStart == sv.npos)
        return sv;
    const auto pEnd = sv.find_last_not_of(back);
    if (pStart == sv.npos)
        return sv;
    const auto count = std::min(pStart, sv.size() - pEnd - 1);
    return sv.substr(count, sv.size() - 2 * count);
}

template<typename Char>
inline std::basic_string_view<Char> TrimPairStringView(const std::basic_string_view<Char> sv, const Char obj)
{
    return TrimPairStringView(sv, obj, obj);
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


