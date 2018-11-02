#include "Convert.h"
#include "common/StrCharset.hpp"

namespace common::strchset
{

namespace detail
{

template<typename Char, typename Dest>
static std::basic_string<Char> ConvertString(const char *str, const size_t size, const Charset inchset)
{
    using namespace common::str::detail;
    switch (inchset)
    {
    case Charset::ASCII:
        return std::basic_string<Char>(str, str + size);
    case Charset::UTF8:
        return CharsetConvertor<UTF8, Dest, char, Char>::Convert(str, size);
    case Charset::UTF16LE:
        return CharsetConvertor<UTF16LE, Dest, char, Char>::Convert(str, size);
    case Charset::UTF16BE:
        return CharsetConvertor<UTF16BE, Dest, char, Char>::Convert(str, size);
    case Charset::UTF32LE:
        return CharsetConvertor<UTF32LE, Dest, char, Char>::Convert(str, size);
    case Charset::UTF32BE:
        return CharsetConvertor<UTF32BE, Dest, char, Char>::Convert(str, size);
    case Charset::GB18030:
        return CharsetConvertor<GB18030, Dest, char, Char>::Convert(str, size);
    default:
        return {};
    }
}

std::string to_string(const char *str, const size_t size, const Charset outchset, const Charset inchset)
{
    using namespace common::str::detail;
    switch (outchset)
    {
    case Charset::ASCII:
        return ConvertString<char, UTF7>(str, size, inchset);
    case Charset::UTF8:
        return ConvertString<char, UTF8>(str, size, inchset);
    case Charset::UTF16LE:
        return ConvertString<char, UTF16LE>(str, size, inchset);
    case Charset::UTF16BE:
        return ConvertString<char, UTF16BE>(str, size, inchset);
    case Charset::UTF32LE:
        return ConvertString<char, UTF32LE>(str, size, inchset);
    case Charset::UTF32BE:
        return ConvertString<char, UTF32BE>(str, size, inchset);
    case Charset::GB18030:
        return ConvertString<char, GB18030>(str, size, inchset);
    default:
        return {};
    }
}

std::u32string to_u32string(const char * str, const size_t size, const Charset inchset)
{
    return ConvertString<char32_t, common::str::detail::UTF32LE>(str, size, inchset);
}
std::u16string to_u16string(const char * str, const size_t size, const Charset inchset)
{
    return ConvertString<char16_t, common::str::detail::UTF16LE>(str, size, inchset);
}
std::string to_u8string(const char * str, const size_t size, const Charset inchset)
{
    return ConvertString<char, common::str::detail::UTF8>(str, size, inchset);
}


template<typename Char>
STRCHSETAPI std::basic_string<Char> ToULEng(const Char *str, const size_t size, const Charset inchset, const bool isUpper)
{
    return isUpper ? common::str::ToUpperEng(str, size, inchset) : common::str::ToLowerEng(str, size, inchset);
}
template STRCHSETAPI std::basic_string<char> ToULEng(const char *str, const size_t size, const Charset inchset, const bool isUpper);
template STRCHSETAPI std::basic_string<wchar_t> ToULEng(const wchar_t *str, const size_t size, const Charset inchset, const bool isUpper);
template STRCHSETAPI std::basic_string<char16_t> ToULEng(const char16_t *str, const size_t size, const Charset inchset, const bool isUpper);
template STRCHSETAPI std::basic_string<char32_t> ToULEng(const char32_t *str, const size_t size, const Charset inchset, const bool isUpper);

}

}


