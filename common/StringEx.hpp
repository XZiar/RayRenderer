#pragma once


#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <type_traits>
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
template<typename CharT, class Judger, class Consumer>
inline void SplitAndDo(const CharT *src, const size_t len, const Judger judger, const Consumer consumer, const bool keepblank = true)
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
template<typename CharT, class Judger, class Consumer>
inline void SplitAndDo(const std::basic_string_view<CharT>& src, const Judger judger, const Consumer consumer, const bool keepblank = true)
{
    SplitAndDo(src.data(), src.length(), judger, consumer, keepblank);
}
template<typename CharT, class Judger, class Consumer>
inline void SplitAndDo(const std::basic_string<CharT>& src, const Judger judger, const Consumer consumer, const bool keepblank = true)
{
    SplitAndDo(src.data(), src.length(), judger, consumer, keepblank);
}
template<typename CharT, class Judger, class Consumer>
inline void SplitAndDo(const std::vector<CharT>& src, const Judger judger, const Consumer consumer, const bool keepblank = true)
{
    SplitAndDo(src.data(), src.length(), judger, consumer, keepblank);
}
template<typename CharT, size_t N, class Judger, class Consumer>
inline void SplitAndDo(const CharT(&src)[N], const Judger judger, const Consumer consumer, const bool keepblank = true)
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
template<typename CharT, class Consumer>
inline void SplitAndDo(const CharT *src, const size_t len, const CharT delim, const Consumer consumer, const bool keepblank = true)
{
    SplitAndDo(src, len, [delim](const CharT obj) { return obj == delim; }, consumer, keepblank);
}
template<typename CharT, class Consumer>
inline void SplitAndDo(const std::basic_string_view<CharT>& src, const CharT delim, const Consumer consumer, const bool keepblank = true)
{
    SplitAndDo(src.data(), src.length(), delim, consumer, keepblank);
}
template<typename CharT, class Consumer>
inline void SplitAndDo(const std::basic_string<CharT>& src, const CharT delim, const Consumer consumer, const bool keepblank = true)
{
    SplitAndDo(src.data(), src.length(), delim, consumer, keepblank);
}
template<typename CharT, class Consumer>
inline void SplitAndDo(const std::vector<CharT>& src, const CharT delim, const Consumer consumer, const bool keepblank = true)
{
    SplitAndDo(src.data(), src.length(), delim, consumer, keepblank);
}
template<typename CharT, size_t N, class Consumer>
inline void SplitAndDo(const CharT(&src)[N], const CharT delim, const Consumer consumer, const bool keepblank = true)
{
    SplitAndDo(src, N - 1, delim, consumer, keepblank);
}


/**
** @brief split source using judger, putting slice into container
** @param src source
** @param len length of source content
** @param judger a function that accepts one element and return (bool) whether it is delim
** @param container a container that allows push_back to put a slice of std::basic_string_view<CharT>
** @param keepblank whether should keep blank splice
** @return container
**/
template<typename CharT, class Judger, class Container>
inline Container& Split(const CharT *src, const size_t len, const Judger judger, Container& container, const bool keepblank = true)
{
    SplitAndDo(src, len, judger, [&container](const CharT *ptr, const size_t size) { container.push_back(std::basic_string_view<CharT>(ptr, size)); }, keepblank);
    return container;
}
template<typename CharT, class Judger, class Container>
inline Container& Split(const std::basic_string_view<CharT>& src, const Judger judger, Container& container, const bool keepblank = true)
{
    return Split(src.data(), src.length(), judger, container, keepblank);
}
template<typename CharT, class Judger, class Container>
inline Container& Split(const std::basic_string<CharT>& src, const Judger judger, Container& container, const bool keepblank = true)
{
    return Split(src.data(), src.length(), judger, container, keepblank);
}
template<typename CharT, class Judger, class Container>
inline Container& Split(const std::vector<CharT>& src, const Judger judger, Container& container, const bool keepblank = true)
{
    return Split(src.data(), src.length(), judger, container, keepblank);
}
template<typename CharT, size_t N, class Judger, class Container>
inline Container& Split(const CharT(&src)[N], const Judger judger, Container& container, const bool keepblank = true)
{
    return Split(src, N - 1, judger, container, keepblank);
}

/**
** @brief split source using judger, putting slice into container
** @param src source
** @param len length of source content
** @param delim the char as delim
** @param container a container that allows push_back to put a slice of std::basic_string_view<CharT>
** @param keepblank whether should keep blank splice
** @return container
**/
template<typename CharT, class Container>
inline Container& Split(const CharT *src, const size_t len, const CharT delim, Container& container, const bool keepblank = true)
{
    SplitAndDo(src, len, delim, [&container](const CharT *ptr, const size_t size) { container.push_back(std::basic_string_view<CharT>(ptr, size)); }, keepblank);
    return container;
}
template<typename CharT, class Container>
inline Container& Split(const std::basic_string_view<CharT>& src, const CharT delim, Container& container, const bool keepblank = true)
{
    return Split(src.data(), src.length(), delim, container, keepblank);
}
template<typename CharT, class Container>
inline Container& Split(const std::basic_string<CharT>& src, const CharT delim, Container& container, const bool keepblank = true)
{
    return Split(src.data(), src.length(), delim, container, keepblank);
}
template<typename CharT, class Container>
inline Container& Split(const std::vector<CharT>& src, const CharT delim, Container& container, const bool keepblank = true)
{
    return Split(src.data(), src.length(), delim, container, keepblank);
}
template<typename CharT, size_t N, class Container>
inline Container& Split(const CharT(&src)[N], const CharT delim, Container& container, const bool keepblank = true)
{
    return Split(src, N - 1, delim, container, keepblank);
}


/**
** @brief split source using judger, putting slice into container
** @param src source
** @param len length of source content
** @param judger a function that accepts one element and return (bool) whether it is delim
** @param container a container that allows push_back to put a slice of std::basic_string_view<CharT>
** @param keepblank whether should keep blank splice
** @return container
**/
template<typename CharT, class Judger>
inline auto& Split(const CharT *src, const size_t len, const Judger judger, const bool keepblank = true)
{
    std::vector<std::basic_string_view<CharT>> container;
    SplitAndDo(src, len, judger, [&container](const CharT *ptr, const size_t size) { container.push_back(std::basic_string_view<CharT>(ptr, size)); }, keepblank);
    return container;
}
template<typename CharT, class Judger>
inline auto Split(const std::basic_string_view<CharT>& src, const Judger judger, const bool keepblank = true)
{
    return Split(src.data(), src.length(), judger, keepblank);
}
template<typename CharT, class Judger>
inline auto Split(const std::basic_string<CharT>& src, const Judger judger, const bool keepblank = true)
{
    return Split(src.data(), src.length(), judger, keepblank);
}
template<typename CharT, class Judger>
inline auto Split(const std::vector<CharT>& src, const Judger judger, const bool keepblank = true)
{
    return Split(src.data(), src.length(), judger, keepblank);
}
template<typename CharT, size_t N, class Judger>
inline auto Split(const CharT(&src)[N], const Judger judger, const bool keepblank = true)
{
    return Split(src, N - 1, judger, keepblank);
}

/**
** @brief split source using judger, putting slice into container
** @param src source
** @param len length of source content
** @param delim the char as delim
** @param container a container that allows push_back to put a slice of std::basic_string_view<CharT>
** @param keepblank whether should keep blank splice
** @return container
**/
template<typename CharT>
inline auto Split(const CharT *src, const size_t len, const CharT delim, const bool keepblank = true)
{
    std::vector<std::basic_string_view<CharT>> container;
    SplitAndDo(src, len, delim, [&container](const CharT *ptr, const size_t size) { container.push_back(std::basic_string_view<CharT>(ptr, size)); }, keepblank);
    return container;
}
template<typename CharT>
inline auto Split(const std::basic_string_view<CharT>& src, const CharT delim, const bool keepblank = true)
{
    return Split(src.data(), src.length(), delim, keepblank);
}
template<typename CharT>
inline auto Split(const std::basic_string<CharT>& src, const CharT delim, const bool keepblank = true)
{
    return Split(src.data(), src.length(), delim, keepblank);
}
template<typename CharT>
inline auto Split(const std::vector<CharT>& src, const CharT delim, const bool keepblank = true)
{
    return Split(src.data(), src.length(), delim, keepblank);
}
template<typename CharT, size_t N>
inline auto Split(const CharT(&src)[N], const CharT delim, const bool keepblank = true)
{
    return Split(src, N - 1, delim, keepblank);
}


template<typename charT>
inline bool IsBeginWith(const std::basic_string<charT>& src, const charT *prefix, const size_t objlen)
{
    const size_t srclen = src.length();
    if (srclen < objlen)
        return false;
    return src.compare(0, objlen, prefix, objlen) == 0;
}
template<typename charT>
inline bool IsBeginWith(const std::basic_string_view<charT>& src, const charT *prefix, const size_t objlen)
{
    const size_t srclen = src.length();
    if (srclen < objlen)
        return false;
    return src.compare(0, objlen, prefix, objlen) == 0;
}
template<typename charT>
inline bool IsBeginWith(const std::basic_string<charT>& src, const std::basic_string_view<charT>& prefix)
{
    return IsBeginWith(src, prefix.data(), prefix.length());
}
template<typename charT>
inline bool IsBeginWith(const std::basic_string_view<charT>& src, const std::basic_string_view<charT>& prefix)
{
    return IsBeginWith(src, prefix.data(), prefix.length());
}
template<typename charT>
inline bool IsBeginWith(const std::basic_string<charT>& src, const std::basic_string<charT>& prefix)
{
    return IsBeginWith(src, prefix.data(), prefix.length());
}
template<typename charT>
inline bool IsBeginWith(const std::basic_string_view<charT>& src, const std::basic_string<charT>& prefix)
{
    return IsBeginWith(src, prefix.data(), prefix.length());
}
template<typename charT, size_t N>
inline bool IsBeginWith(const std::basic_string<charT>& src, const charT(&prefix)[N])
{
    return IsBeginWith(src, prefix, N - 1);
}
template<typename charT, size_t N>
inline bool IsBeginWith(const std::basic_string_view<charT>& src, const charT(&prefix)[N])
{
    return IsBeginWith(src, prefix, N - 1);
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
inline void appendstr(std::basic_string<CharT>& obj)
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
template<class T, typename charT = T::value_type, class itT = T::const_iterator>
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
template<class T, typename charT = T::value_type, class itT = T::const_iterator, size_t N>
inline std::optional<itT> ifind_first(const T& src, const charT(&obj)[N])
{
    return ifind_first(src, std::basic_string<charT>(obj));
}
}


}


