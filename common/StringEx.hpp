#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <type_traits>
#include <codecvt>
#include <cctype>
#include <cwctype>

namespace common
{


/** @brief calculate simple hash for string, used for switch-string
** @param str std-string_view for the text
** @return uint64_t the hash
**/
template<class T, class = typename std::enable_if<std::is_same<T, std::string>::value || std::is_same<T, std::string_view>::value>::type>
inline uint64_t hash_(const T& str)
{
	uint64_t hash = 0;
	for (size_t a = 0, len = str.length(); a < len; ++a)
		hash = hash * 33 + str[a];
	return hash;
}
/** @brief calculate simple hash for string, used for switch-string
** @param str std-string for the text
** @return uint64_t the hash
**/
constexpr inline uint64_t hash_(const char *str)
{
	uint64_t hash = 0;
	for (;*str != '\0'; ++str)
		hash = hash * 33 + *str;
	return hash;
}
/** @brief calculate simple hash for string, used for switch-string
** @return uint64_t the hash
**/
constexpr inline uint64_t operator "" _hash(const char *str, size_t)
{
	return hash_(str);
}


namespace str
{

template<class T, class charT = T::value_type>
inline auto split(const T& src, const charT delim, const bool keepblank = true)
{
	using namespace std;
	vector<basic_string_view<charT>> ret;
	size_t cur = 0, last = 0, len = src.length();
	for (; cur < len; cur++)
	{
		if (src[cur] == delim)
		{
			if (keepblank || cur != last)
				ret.push_back(basic_string_view<charT>(&src[last], cur - last));
			last = cur + 1;
		}
	}
	if (keepblank || cur != last)
		ret.push_back(basic_string_view<charT>(&src[last], cur - last));
	return ret;
}

template<class charT, size_t N>
inline auto split(const charT(&src)[N], const charT delim, const bool keepblank = true)
{
	using namespace std;
	vector<basic_string_view<charT>> ret;
	size_t cur = 0, last = 0, len = N - 1;
	for (; cur < len; cur++)
	{
		if (src[cur] == delim)
		{
			if (keepblank || cur != last)
				ret.push_back(basic_string_view<charT>(&src[last], cur - last));
			last = cur + 1;
		}
	}
	if (keepblank || cur != last)
		ret.push_back(basic_string_view<charT>(&src[last], cur - last));
	return ret;
}

template<class T, class charT = T::value_type>
inline bool begin_with(const T& src, const std::basic_string<charT>& prefix)
{
	const size_t srclen = src.length(), objlen = prefix.length();
	if (srclen < objlen)
		return false;
	return src.compare(0, objlen, prefix) == 0;
}

template<class T, class charT = T::value_type, size_t N>
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

namespace detail
{
inline char toUpper(const char ch)
{
	return (char)std::toupper(ch);
}
inline wchar_t toUpper(const wchar_t ch)
{
	return (wchar_t)std::towupper(ch);
}
inline std::string toUpper(const std::string& src)
{
	std::string ret;
	ret.reserve(src.length());
	for (const auto ch : src)
		ret.push_back((char)std::toupper(ch));
	return ret;
}
inline std::wstring toUpper(const std::wstring& src)
{
	std::wstring ret;
	ret.reserve(src.length());
	for (const auto ch : src)
		ret.push_back((wchar_t)std::towupper(ch));
	return ret;
}
inline bool upperComp(const char ch1, const char ch2)
{
	return (char)std::toupper(ch1) == ch2;
}
inline bool upperComp(const wchar_t ch1, const wchar_t ch2)
{
	return (wchar_t)std::towupper(ch1) == ch2;
}
}
template<class T, class charT = T::value_type, class itT = T::const_iterator>
inline std::optional<itT> ifind_first(T src, const std::basic_string<charT>& obj)
{
	const size_t srclen = src.length(), objlen = obj.length();
	if (srclen < objlen)
		return {};
	const size_t morelen = srclen - objlen;
	for (size_t a = 0; a < objlen; ++a)
		src[a] = detail::toUpper(src[a]);
	auto upobj = detail::toUpper(obj);
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
		src[objlen + a] = detail::toUpper(src[objlen + a]);
	}
	return {};
}
template<class T, class charT = T::value_type, class itT = T::const_iterator, size_t N>
inline std::optional<itT> ifind_first(const T& src, const charT(&obj)[N])
{
	return ifind_first(src, std::basic_string<charT>(obj));
}
}

enum class Charset { ASCII, GB18030, UTF8, UTF16LE, UTF16BE, UTF32 };

inline Charset toCharset(const std::string& chset)
{
	switch (hash_(chset))
	{
	case "GB18030"_hash:
		return Charset::GB18030;
	case "UTF-8"_hash:
		return Charset::UTF8;
	case "UTF-16LE"_hash:
		return Charset::UTF16LE;
	case "UTF-16BE"_hash:
		return Charset::UTF16BE;
	case "error"_hash:
		return Charset::ASCII;
	default:
		return Charset::ASCII;
	}
}

inline std::wstring getCharsetWName(const Charset chset)
{
	switch (chset)
	{
	case Charset::ASCII:
		return L"ASCII";
	case Charset::GB18030:
		return L"GB18030";
	case Charset::UTF8:
		return L"UTF-8";
	case Charset::UTF16BE:
		return L"UTF-16BE";
	case Charset::UTF16LE:
		return L"UTF-16LE";
	case Charset::UTF32:
		return L"UTF-32";
	default:
		return L"error";
	}
}

template<class T, class = typename std::enable_if<std::is_integral<T>::value>::type>
inline std::wstring to_wstring(const T val)
{
	return std::to_wstring(val);
}

inline std::wstring to_wstring(const std::string& str, const Charset chset = Charset::ASCII)
{
	switch (chset)
	{
	case Charset::ASCII:
		return std::wstring(str.begin(), str.end());
	case Charset::GB18030:
		{
			std::wstring_convert<std::codecvt_byname<wchar_t, char, mbstate_t>> gbk_utf16_cvt(new std::codecvt_byname<wchar_t, char, mbstate_t>(".936"));
			return gbk_utf16_cvt.from_bytes(str);
		}
	case Charset::UTF8:
		{ 
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf8_utf16_cvt;
			return utf8_utf16_cvt.from_bytes(str);
		}
	}
	return L"";
}

inline std::wstring to_wstring(const char* const str, const Charset chset = Charset::ASCII)
{
	if (chset == Charset::ASCII)
		return std::wstring(str, str + strlen(str));
	return to_wstring(std::string(str), chset);
}


}