#pragma once

#include "StrBase.hpp"
#include <optional>
#include <cctype>
#include <cwctype>


namespace common
{

namespace str
{


/**
** @brief split source using judger, do something for each slice
** @param src source
** @param len length of source content
** @param judger a function that accepts one element and return (bool) whether it is delim
** @param consumer a function that accepts start pos AND length for each slice and do something
** @param keepblank whether should keep blank splice
**/
template<typename Char, class Judger, class Consumer, class = std::enable_if_t<std::is_invocable_r_v<bool, Judger, Char>>>
inline void SplitAndDo(const Char *src, const size_t len, const Judger judger, const Consumer consumer, const bool keepblank = true)
{
    size_t cur = 0, last = 0;
    for (; cur < len; cur++)
    {
        if (judger(src[cur]))
        {
            if (keepblank || cur != last)
                consumer(&src[last], cur - last);
            last = cur + 1;
        }
    }
    if (keepblank || cur != last)
        consumer(&src[last], cur - last);
}
template<typename Char, typename T, class Judger, class Consumer, class = detail::CanBeStringView<T, Char>, class = std::enable_if_t<std::is_invocable_r_v<bool, Judger, Char>>>
inline void SplitAndDo(const T& src, const Judger judger, const Consumer consumer, const bool keepblank = true)
{
    const auto srcsv = detail::ToStringView<Char>(src);
    SplitAndDo(srcsv.data(), srcsv.length(), judger, consumer, keepblank);
}
template<typename Char, size_t N, class Judger, class Consumer, class = std::enable_if_t<std::is_invocable_r_v<bool, Judger, Char>>>
inline void SplitAndDo(const Char(&src)[N], const Judger judger, const Consumer consumer, const bool keepblank = true)
{
    SplitAndDo(src, N - 1, judger, consumer, keepblank);
}

/**
** @brief split source using judger, do something for each slice
** @param src source
** @param len length of source content
** @param delim the char as delim
** @param consumer a function that accepts start pos AND length for each slice and do something
** @param keepblank whether should keep blank splice
**/
template<typename Char, class Consumer>
inline void SplitAndDo(const Char *src, const size_t len, const Char delim, const Consumer consumer, const bool keepblank = true)
{
    SplitAndDo(src, len, [delim](const Char obj) { return obj == delim; }, consumer, keepblank);
}
template<typename Char, typename T, class Consumer, class = detail::CanBeStringView<T, Char>>
inline void SplitAndDo(const T& src, const Char delim, const Consumer consumer, const bool keepblank = true)
{
    const auto srcsv = detail::ToStringView<Char>(src);
    SplitAndDo(srcsv.data(), srcsv.length(), delim, consumer, keepblank);
}
template<typename Char, size_t N, class Consumer>
inline void SplitAndDo(const Char(&src)[N], const Char delim, const Consumer consumer, const bool keepblank = true)
{
    SplitAndDo(src, N - 1, delim, consumer, keepblank);
}


/**
** @brief split source using judger, putting slice into container
** @param src source
** @param len length of source content
** @param judger a function that accepts one element and return (bool) whether it is delim
** @param container a container that allows push_back to put a slice of std::basic_string_view<Char>
** @param keepblank whether should keep blank splice
** @return container
**/
template<typename Char, class Judger, class Container, class = std::enable_if_t<std::is_invocable_r_v<bool, Judger, Char>>>
inline Container& SplitInto(const Char *src, const size_t len, const Judger judger, Container& container, const bool keepblank = true)
{
    SplitAndDo(src, len, judger, [&container](const Char *ptr, const size_t size) { container.push_back(std::basic_string_view<Char>(ptr, size)); }, keepblank);
    return container;
}
template<typename Char, typename T, class Judger, class Container, class = detail::CanBeStringView<T, Char>, class = std::enable_if_t<std::is_invocable_r_v<bool, Judger, Char>>>
inline Container& SplitInto(const T& src, const Judger judger, Container& container, const bool keepblank = true)
{
    const auto srcsv = detail::ToStringView<Char>(src);
    return SplitInto(srcsv.data(), srcsv.length(), judger, container, keepblank);
}
template<typename Char, size_t N, class Judger, class Container, class = std::enable_if_t<std::is_invocable_r_v<bool, Judger, Char>>>
inline Container& SplitInto(const Char(&src)[N], const Judger judger, Container& container, const bool keepblank = true)
{
    return SplitInto(src, N - 1, judger, container, keepblank);
}


/**
** @brief split source using judger, putting slice into container
** @param src source
** @param len length of source content
** @param delim the char as delim
** @param container a container that allows push_back to put a slice of std::basic_string_view<Char>
** @param keepblank whether should keep blank splice
** @return container
**/
template<typename Char, class Container>
inline Container& SplitInto(const Char *src, const size_t len, const Char delim, Container& container, const bool keepblank = true)
{
    SplitAndDo(src, len, delim, [&container](const Char *ptr, const size_t size) { container.push_back(std::basic_string_view<Char>(ptr, size)); }, keepblank);
    return container;
}
template<typename Char, typename T, class Container, class = detail::CanBeStringView<T, Char>>
inline Container& SplitInto(const T& src, const Char delim, Container& container, const bool keepblank = true)
{
    const auto srcsv = detail::ToStringView<Char>(src);
    return SplitInto(srcsv.data(), srcsv.length(), delim, container, keepblank);
}
template<typename Char, size_t N, class Container>
inline Container& SplitInto(const Char(&src)[N], const Char delim, Container& container, const bool keepblank = true)
{
    return SplitInto(src, N - 1, delim, container, keepblank);
}


/**
** @brief split source using judger, putting slice into container
** @param src source
** @param len length of source content
** @param judger a function that accepts one element and return (bool) whether it is delim
** @param container a container that allows push_back to put a slice of std::basic_string_view<Char>
** @param keepblank whether should keep blank splice
** @return container
**/
template<typename Char, class Judger, class = std::enable_if_t<std::is_invocable_r_v<bool, Judger, Char>>>
inline std::vector<std::basic_string_view<Char>> Split(const Char *src, const size_t len, const Judger judger, const bool keepblank = true)
{
    std::vector<std::basic_string_view<Char>> container;
    SplitAndDo(src, len, judger, [&container](const Char *ptr, const size_t size) { container.push_back(std::basic_string_view<Char>(ptr, size)); }, keepblank);
    return container;
}
template<typename Char, typename T, class Judger, class = detail::CanBeStringView<T, Char>, class = std::enable_if_t<std::is_invocable_r_v<bool, Judger, Char>>>
inline std::vector<std::basic_string_view<Char>> Split(const T& src, const Judger judger, const bool keepblank = true)
{
    const auto srcsv = detail::ToStringView<Char>(src);
    return Split(srcsv.data(), srcsv.length(), judger, keepblank);
}
template<typename Char, size_t N, class Judger, class = std::enable_if_t<std::is_invocable_r_v<bool, Judger, Char>>>
inline std::vector<std::basic_string_view<Char>> Split(const Char(&src)[N], const Judger judger, const bool keepblank = true)
{
    return Split(src, N - 1, judger, keepblank);
}


/**
** @brief split source using judger, putting slice into container
** @param src source
** @param len length of source content
** @param delim the char as delim
** @param container a container that allows push_back to put a slice of std::basic_string_view<Char>
** @param keepblank whether should keep blank splice
** @return container
**/
template<typename Char>
inline std::vector<std::basic_string_view<Char>> Split(const Char *src, const size_t len, const Char delim, const bool keepblank = true)
{
    std::vector<std::basic_string_view<Char>> container;
    SplitAndDo(src, len, delim, [&container](const Char *ptr, const size_t size) { container.push_back(std::basic_string_view<Char>(ptr, size)); }, keepblank);
    return container;
}
template<typename Char, typename T, class = detail::CanBeStringView<T, Char>>
inline std::vector<std::basic_string_view<Char>> Split(const T& src, const Char delim, const bool keepblank = true)
{
    const auto srcsv = detail::ToStringView<Char>(src);
    return Split(srcsv.data(), srcsv.length(), delim, keepblank);
}
template<typename Char, size_t N>
inline std::vector<std::basic_string_view<Char>> Split(const Char(&src)[N], const Char delim, const bool keepblank = true)
{
    return Split(src, N - 1, delim, keepblank);
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


template<typename Char>
inline constexpr bool IsBeginWith(const Char* src, const size_t srclen, const Char *prefix, const size_t objlen)
{
    if (srclen < objlen)
        return false;
    return std::char_traits<Char>::compare(src, prefix, objlen) == 0;
}
template<typename Char, typename T, class = detail::CanBeStringView<T, Char>>
inline constexpr bool IsBeginWith(const T& src, const Char *prefix, const size_t objlen)
{
    const auto srcsv = detail::ToStringView<Char>(src);
    return IsBeginWith(srcsv.data(), srcsv.length(), prefix, objlen);
}
template<typename Char, typename T1, typename T2, class = detail::CanBeStringView<T1, Char>, class = detail::CanBeStringView<T2, Char>>
inline constexpr bool IsBeginWith(const T1& src, const T2 &prefix)
{
    const auto srcsv = detail::ToStringView<Char>(src), prefixsv = detail::ToStringView<Char>(prefix);
    return IsBeginWith(srcsv.data(), srcsv.length(), prefixsv.data(), prefixsv.length());
}
template<typename Char, typename T, size_t N, class = detail::CanBeStringView<T, Char>>
inline constexpr bool IsBeginWith(const T& src, const Char(&prefix)[N])
{
    const auto srcsv = detail::ToStringView<Char>(src);
    return IsBeginWith(srcsv.data(), srcsv.length(), prefix, N - 1);
}
template<typename Char, size_t N1, size_t N2>
inline constexpr bool IsBeginWith(const Char(&src)[N1], const Char(&prefix)[N2])
{
    return IsBeginWith(src, N1 - 1, prefix, N2 - 1);
}
template<typename Char, typename T, size_t N, class = detail::CanBeStringView<T, Char>>
inline constexpr bool IsBeginWith(const Char(&src)[N], const T& prefix)
{
    const auto prefixsv = detail::ToStringView<Char>(prefix);
    return IsBeginWith(src, N - 1, prefixsv.data(), prefixsv.length());
}


namespace detail
{

template<class CharT>
inline size_t getsize(const size_t size)
{
    return size;
}
template<class CharT, class... Args>
inline size_t getsize(const size_t size, const std::basic_string_view<CharT>& str, Args&&... args)
{
    return getsize<CharT>(size + str.length(), std::forward<Args>(args)...);
}
template<class CharT, typename A, class... Args>
inline size_t getsize(const size_t size, const std::vector<std::basic_string_view<CharT>, A>& strs, Args&&... args)
{
    size_t thissize = 0;
    for (const auto& str : strs)
        thissize += str.length();
    return getsize<CharT>(size + thissize, std::forward<Args>(args)...);
}

template<class CharT>
inline void appendstr(std::basic_string<CharT>&)
{ }
template<class CharT, class... Args>
inline void appendstr(std::basic_string<CharT>& obj, const std::basic_string_view<CharT>& str, Args&&... args)
{
    obj.append(str);
    appendstr<CharT>(obj, std::forward<Args>(args)...);
}
template<class CharT, typename A, class... Args>
inline void appendstr(std::basic_string<CharT>& obj, const std::vector<std::basic_string_view<CharT>, A>& strs, Args&&... args)
{
    for (const auto& str : strs)
        obj.append(str);
    appendstr<CharT>(obj, std::forward<Args>(args)...);
}

}
template<class CharT, class... Args>
inline std::basic_string<CharT> Concat(Args&&... args)
{
    std::basic_string<CharT> ret;
    size_t size = detail::getsize<CharT>(0, std::forward<Args>(args)...);
    ret.reserve(size + 1);
    detail::appendstr<CharT>(ret, std::forward<Args>(args)...);
    return ret;
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


