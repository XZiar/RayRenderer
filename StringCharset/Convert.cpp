#include "Convert.h"
#include "common/StrCharset.hpp"

namespace common::strchset
{

namespace detail
{

template<typename Char, typename Conv>
[[nodiscard]] forceinline std::basic_string<Char> ConvertString(const common::span<const std::byte> data, const Charset inchset)
{
    using namespace common::str::detail;
    const std::basic_string_view<uint8_t> str(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    switch (inchset)
    {
    case Charset::ASCII:
        return Transform(GetDecoder<UTF7>   (str), GetEncoder<Conv, Char>());
    case Charset::UTF8:
        return Transform(GetDecoder<UTF8>   (str), GetEncoder<Conv, Char>());
    case Charset::UTF16LE:
        return Transform(GetDecoder<UTF16LE>(str), GetEncoder<Conv, Char>());
    case Charset::UTF16BE:
        return Transform(GetDecoder<UTF16BE>(str), GetEncoder<Conv, Char>());
    case Charset::UTF32LE:
        return Transform(GetDecoder<UTF32LE>(str), GetEncoder<Conv, Char>());
    case Charset::UTF32BE:
        return Transform(GetDecoder<UTF32BE>(str), GetEncoder<Conv, Char>());
    case Charset::GB18030:
        return Transform(GetDecoder<GB18030>(str), GetEncoder<Conv, Char>());
    default: // should not enter, to please compiler
        COMMON_THROW(BaseException, u"unknown charset", inchset);
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
        COMMON_THROW(BaseException, u"unknown charset", outchset);
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
STRCHSETAPI std::basic_string<Char> ToUEng(const std::basic_string_view<Char> str, const Charset inchset)
{
    using namespace common::str::detail;
    return DirectConv<Char>(str, inchset, EngUpper);

}
template STRCHSETAPI std::basic_string<char>     ToUEng(const std::basic_string_view<char>     str, const Charset inchset);
template STRCHSETAPI std::basic_string<wchar_t>  ToUEng(const std::basic_string_view<wchar_t>  str, const Charset inchset);
template STRCHSETAPI std::basic_string<char16_t> ToUEng(const std::basic_string_view<char16_t> str, const Charset inchset);
template STRCHSETAPI std::basic_string<char32_t> ToUEng(const std::basic_string_view<char32_t> str, const Charset inchset);

template<typename Char>
STRCHSETAPI std::basic_string<Char> ToLEng(const std::basic_string_view<Char> str, const Charset inchset)
{
    using namespace common::str::detail;
    return DirectConv<Char>(str, inchset, EngLower);
}
template STRCHSETAPI std::basic_string<char>     ToLEng(const std::basic_string_view<char>     str, const Charset inchset);
template STRCHSETAPI std::basic_string<wchar_t>  ToLEng(const std::basic_string_view<wchar_t>  str, const Charset inchset);
template STRCHSETAPI std::basic_string<char16_t> ToLEng(const std::basic_string_view<char16_t> str, const Charset inchset);
template STRCHSETAPI std::basic_string<char32_t> ToLEng(const std::basic_string_view<char32_t> str, const Charset inchset);

}

}


