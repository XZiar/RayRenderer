#pragma once

//Bypass C++17 codecvt deprecation
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1

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
** @brief split source using judger, putting slice into container
** @param src source
** @param judger a function that accepts one element and return (bool) whether it is delim
** @param container a container that allows push_back to put a slice of std::basyc_string_view<CharT>
** @param keepblank whether should keep blank splice
** @return container
**/
template<class T, typename CharT = T::value_type, class Judger, class Container>
inline auto split(const T& src, const Judger judger, Container& container, const bool keepblank = true)
{
	using namespace std;
	size_t cur = 0, last = 0, len = src.length();
	for (; cur < len; cur++)
	{
		if (judger(src[cur]))
		{
			if (keepblank || cur != last)
				container.push_back(basic_string_view<CharT>(&src[last], cur - last));
			last = cur + 1;
		}
	}
	if (keepblank || cur != last)
		container.push_back(basic_string_view<CharT>(&src[last], cur - last));
	return container;
}

/**
** @brief split source using delim, do something for each slice
** @param src source
** @param delim a delim that split source
** @param consumer a function that accepts start pos AND length for each slice and do something
** @param keepblank whether should keep blank splice
**/
template<class T, typename CharT = T::value_type, class Func>
inline void SplitAndDo(const T& src, const CharT delim, const Func consumer, const bool keepblank = true)
{
	size_t cur = 0, last = 0, len = src.length();
	for (; cur < len; cur++)
	{
		if (src[cur] == delim)
		{
			if (keepblank || cur != last)
				consumer(&src[last], cur - last);
			last = cur + 1;
		}
	}
	if (keepblank || cur != last)
		consumer(&src[last], cur - last);
}

template<class T, typename CharT = T::value_type>
inline auto split(const T& src, const CharT delim, const bool keepblank = true)
{
	using namespace std;
	vector<basic_string_view<CharT>> ret;
	SplitAndDo(src, delim, [&ret](const CharT *pos, const size_t len) { ret.push_back(basic_string_view<CharT>(pos, len)); }, keepblank);
	return ret;
}

template<typename CharT, size_t N>
inline auto split(const CharT(&src)[N], const CharT delim, const bool keepblank = true)
{
	return split(std::basic_string_view<CharT>(src, N - 1), delim, keepblank);
}

template<class T, typename charT = T::value_type>
inline bool begin_with(const T& src, const std::basic_string<charT>& prefix)
{
	const size_t srclen = src.length(), objlen = prefix.length();
	if (srclen < objlen)
		return false;
	return src.compare(0, objlen, prefix) == 0;
}

template<class T, typename charT = T::value_type, size_t N>
inline bool begin_with(const T& src, const charT(&prefix)[N])
{
	constexpr size_t objlen = N - 1;
	const size_t srclen = src.length();
	if (srclen < objlen)
		return false;
	return src.compare(0, objlen, prefix, objlen) == 0;
}

namespace detail
{
template<class T>
inline size_t getsize(const size_t size)
{
	return size;
}
template<class T, class... ARGS>
inline size_t getsize(const size_t size, const std::basic_string_view<T>& str, ARGS... args)
{
	return getsize<T>(size + str.length(), args...);
}
template<class T, size_t N, class... ARGS>
inline size_t getsize(const size_t size, const T(&str)[N], ARGS... args)
{
	return getsize<T>(size + N - 1, args...);
}
template<class T>
inline void appendstr(std::basic_string<T>& obj)
{
	return;
}
template<class T, class... ARGS>
inline void appendstr(std::basic_string<T>& obj, const std::basic_string_view<T>& str, ARGS... args)
{
	obj.append(str);
	appendstr<T>(obj, args...);
}
template<class T, size_t N, class... ARGS>
inline void appendstr(std::basic_string<T>& obj, const T(&str)[N], ARGS... args)
{
	obj.append(str, N - 1);
	appendstr<T>(obj, args...);
}
}
template<class T, class... ARGS>
inline std::basic_string<T> concat(ARGS... args)
{
	std::basic_string<T> ret;
	size_t size = detail::getsize<T>(0, args...);
	ret.reserve(size);
	detail::appendstr<T>(ret, args...);
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


