#pragma once

#include <string>
#include <type_traits>
#include <codecvt>

namespace common
{


template<class T, class = typename std::enable_if<std::is_integral<T>::value>::type>
inline std::wstring to_wstring(const T val)
{
	return std::to_wstring(val);
}

enum class StringFormat { ASCII, GB18030, UTF8, UTF16LE, UTF16BE, UTF32 };

inline std::wstring to_wstring(const std::string& str, const StringFormat format = StringFormat::ASCII)
{
	switch (format)
	{
	case StringFormat::ASCII:
		return std::wstring(str.begin(), str.end());
	case StringFormat::GB18030:
		{
			std::wstring_convert<std::codecvt_byname<wchar_t, char, mbstate_t>> gbk_utf16_cvt(new std::codecvt_byname<wchar_t, char, mbstate_t>(".936"));
			return gbk_utf16_cvt.from_bytes(str);
		}
	case StringFormat::UTF8:
		{ 
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf8_utf16_cvt;
			return utf8_utf16_cvt.from_bytes(str);
		}
	}
	return L"";
}

inline std::wstring to_wstring(const char* const str, const StringFormat format = StringFormat::ASCII)
{
	if (format == StringFormat::ASCII)
		return std::wstring(str, str + strlen(str));
	return to_wstring(std::string(str), format);
}


}
