#pragma once

#include <string>
#include <type_traits>
#include <codecvt>

namespace common
{


/** @brief calculate simple hash for string, used for switch-string
** @param str std-string for the text
** @return uint64_t the hash
**/
inline uint64_t hash_(const std::string& str)
{
	uint64_t hash = 0;
	for (size_t a = 0; a < str.length(); ++a)
		hash = hash * 33 + str[a];
	return hash;
}
/** @brief calculate simple hash for string, used for switch-string
** @param str std-string for the text
** @return uint64_t the hash
**/
constexpr uint64_t hash_(const char *str, const uint64_t last = 0)
{
	return *str == '\0' ? last : hash_(str + 1, *str + last * 33);
}
/** @brief calculate simple hash for string, used for switch-string
** @return uint64_t the hash
**/
constexpr uint64_t operator "" _hash(const char *str, size_t)
{
	return hash_(str);
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
