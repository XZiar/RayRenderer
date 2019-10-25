#include "Convert.h"
#include "common/StrCharset.hpp"

namespace common::strchset
{

namespace detail
{

template<typename Char, typename Dest>
static std::basic_string<Char> ConvertString(const common::span<const std::byte> data, const Charset inchset)
{
    using namespace common::str::detail;
    const auto ptr = reinterpret_cast<const uint8_t*>(data.data());
    const auto size = data.size();
    switch (inchset)
    {
    case Charset::ASCII:
        return std::basic_string<Char>(ptr, ptr + size);
    case Charset::UTF8:
        return CharsetConvertor<UTF8   , Dest, uint8_t, Char>::Convert(ptr, size);
    case Charset::UTF16LE:
        return CharsetConvertor<UTF16LE, Dest, uint8_t, Char>::Convert(ptr, size);
    case Charset::UTF16BE:
        return CharsetConvertor<UTF16BE, Dest, uint8_t, Char>::Convert(ptr, size);
    case Charset::UTF32LE:
        return CharsetConvertor<UTF32LE, Dest, uint8_t, Char>::Convert(ptr, size);
    case Charset::UTF32BE:
        return CharsetConvertor<UTF32BE, Dest, uint8_t, Char>::Convert(ptr, size);
    case Charset::GB18030:
        return CharsetConvertor<GB18030, Dest, uint8_t, Char>::Convert(ptr, size);
    default:
        return {};
    }
}

std::string to_string(const common::span<const std::byte> data, const Charset outchset, const Charset inchset)
{
    using namespace common::str::detail;
    switch (outchset)
    {
    case Charset::ASCII:
        return ConvertString<char, UTF7   >(data, inchset);
    case Charset::UTF8:
        return ConvertString<char, UTF8   >(data, inchset);
    case Charset::UTF16LE:
        return ConvertString<char, UTF16LE>(data, inchset);
    case Charset::UTF16BE:
        return ConvertString<char, UTF16BE>(data, inchset);
    case Charset::UTF32LE:
        return ConvertString<char, UTF32LE>(data, inchset);
    case Charset::UTF32BE:
        return ConvertString<char, UTF32BE>(data, inchset);
    case Charset::GB18030:
        return ConvertString<char, GB18030>(data, inchset);
    default:
        return {};
    }
}

std::u32string  to_u32string(const common::span<const std::byte> data, const Charset inchset)
{
    return ConvertString<char32_t, common::str::detail::UTF32LE>(data, inchset);
}
std::u16string  to_u16string(const common::span<const std::byte> data, const Charset inchset)
{
    return ConvertString<char16_t, common::str::detail::UTF16LE>(data, inchset);
}
std::string     to_u8string (const common::span<const std::byte> data, const Charset inchset)
{
    return ConvertString<char    , common::str::detail::UTF8   >(data, inchset);
}


template<typename Char>
STRCHSETAPI std::basic_string<Char> ToULEng(const std::basic_string_view<Char> str, const Charset inchset, const bool isUpper)
{
    return isUpper ? common::str::ToUpperEng(str, inchset) : common::str::ToLowerEng(str, inchset);
}
template STRCHSETAPI std::basic_string<char>     ToULEng(const std::basic_string_view<char>     str, const Charset inchset, const bool isUpper);
template STRCHSETAPI std::basic_string<wchar_t>  ToULEng(const std::basic_string_view<wchar_t>  str, const Charset inchset, const bool isUpper);
template STRCHSETAPI std::basic_string<char16_t> ToULEng(const std::basic_string_view<char16_t> str, const Charset inchset, const bool isUpper);
template STRCHSETAPI std::basic_string<char32_t> ToULEng(const std::basic_string_view<char32_t> str, const Charset inchset, const bool isUpper);

}

}


