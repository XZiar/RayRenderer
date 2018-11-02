#include "Convert.h"
#include "common/StrCharset.hpp"

namespace common::strchset
{

namespace detail
{

template<typename Char, typename Dest>
static std::basic_string<Char> ConvertString(const char *str, const size_t size, const Charset inchset, const bool toLE = true)
{
    using namespace common::str::detail;
    switch (inchset)
    {
    case Charset::ASCII:
        return std::basic_string<Char>(str, str + size);
    case Charset::UTF8:
        return CharsetConvertor<UTF8, Dest, char, Char>::Convert(str, size, true, toLE);
    case Charset::UTF16LE:
        return CharsetConvertor<UTF16, Dest, char, Char>::Convert(str, size, true, toLE);
    case Charset::UTF16BE:
        return CharsetConvertor<UTF16, Dest, char, Char>::Convert(str, size, false, toLE);
    case Charset::UTF32:
        return CharsetConvertor<UTF32, Dest, char, Char>::Convert(str, size, true, toLE);
    case Charset::GB18030:
        return CharsetConvertor<GB18030, Dest, char, Char>::Convert(str, size, true, toLE);
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
        return ConvertString<char, UTF16>(str, size, inchset);
    case Charset::UTF16BE:
        return ConvertString<char, UTF16>(str, size, inchset, false);
    case Charset::UTF32:
        return ConvertString<char, UTF32>(str, size, inchset);
    case Charset::GB18030:
        return ConvertString<char, GB18030>(str, size, inchset);
    default:
        return {};
    }
}

std::u32string to_u32string(const char * str, const size_t size, const Charset inchset)
{
    return ConvertString<char32_t, common::str::detail::UTF32>(str, size, inchset);
}
std::u16string to_u16string(const char * str, const size_t size, const Charset inchset)
{
    return ConvertString<char16_t, common::str::detail::UTF16>(str, size, inchset);
}
std::string to_u8string(const char * str, const size_t size, const Charset inchset)
{
    return ConvertString<char, common::str::detail::UTF8>(str, size, inchset);
}

}

}


