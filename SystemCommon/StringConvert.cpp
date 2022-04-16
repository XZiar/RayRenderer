#include "SystemCommonPch.h"
#include "StringConvert.h"
#include "StrEncoding.hpp"


#if COMMON_COMPILER_MSVC
#   define SYSCOMMONTPL SYSCOMMONAPI
#else
#   define SYSCOMMONTPL
#endif


namespace common::str
{

namespace detail
{

template<typename Char, typename Conv>
[[nodiscard]] forceinline std::basic_string<Char> ConvertString(const common::span<const std::byte> data, const Encoding inchset)
{
    using namespace common::str::charset::detail;
    const std::basic_string_view<uint8_t> str(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    switch (inchset)
    {
    case Encoding::ASCII:
        return Transform(GetDecoder<UTF7>   (str), GetEncoder<Conv, Char>());
    case Encoding::UTF8:
        return Transform(GetDecoder<UTF8>   (str), GetEncoder<Conv, Char>());
    case Encoding::URI:
        return Transform(GetDecoder<URI>    (str), GetEncoder<Conv, Char>());
    case Encoding::UTF16LE:
        return Transform(GetDecoder<UTF16LE>(str), GetEncoder<Conv, Char>());
    case Encoding::UTF16BE:
        return Transform(GetDecoder<UTF16BE>(str), GetEncoder<Conv, Char>());
    case Encoding::UTF32LE:
        return Transform(GetDecoder<UTF32LE>(str), GetEncoder<Conv, Char>());
    case Encoding::UTF32BE:
        return Transform(GetDecoder<UTF32BE>(str), GetEncoder<Conv, Char>());
    case Encoding::GB18030:
        return Transform(GetDecoder<GB18030>(str), GetEncoder<Conv, Char>());
    default: // should not enter, to please compiler
        COMMON_THROW(BaseException, u"unknown charset");
    }
}

std::string to_string(const common::span<const std::byte> data, const Encoding outchset, const Encoding inchset)
{
    using namespace common::str::charset::detail;
    switch (outchset)
    {
    case Encoding::ASCII:
        return ConvertString<char, UTF7   >(data, inchset);
    case Encoding::UTF8:
        return ConvertString<char, UTF8   >(data, inchset);
    case Encoding::URI:
        return ConvertString<char, URI    >(data, inchset);
    case Encoding::UTF16LE:
        return ConvertString<char, UTF16LE>(data, inchset);
    case Encoding::UTF16BE:
        return ConvertString<char, UTF16BE>(data, inchset);
    case Encoding::UTF32LE:
        return ConvertString<char, UTF32LE>(data, inchset);
    case Encoding::UTF32BE:
        return ConvertString<char, UTF32BE>(data, inchset);
    case Encoding::GB18030:
        return ConvertString<char, GB18030>(data, inchset);
    default:
        COMMON_THROW(BaseException, u"unknown charset");
    }
}

std::u32string  to_u32string(const common::span<const std::byte> data, const Encoding inchset)
{
    return ConvertString<char32_t, common::str::charset::detail::UTF32LE>(data, inchset);
}
std::u16string  to_u16string(const common::span<const std::byte> data, const Encoding inchset)
{
    return ConvertString<char16_t, common::str::charset::detail::UTF16LE>(data, inchset);
}
str::u8string   to_u8string (const common::span<const std::byte> data, const Encoding inchset)
{
    return ConvertString<u8ch_t  , common::str::charset::detail::UTF8   >(data, inchset);
}


template<typename Char>
SYSCOMMONAPI std::basic_string<Char> ToUEng(const std::basic_string_view<Char> str, const Encoding inchset)
{
    using namespace common::str::charset::detail;
    return DirectConv<Char>(str, inchset, EngUpper);
}
template SYSCOMMONTPL std::basic_string<char>     ToUEng(const std::basic_string_view<char>     str, const Encoding inchset);
template SYSCOMMONTPL std::basic_string<wchar_t>  ToUEng(const std::basic_string_view<wchar_t>  str, const Encoding inchset);
template SYSCOMMONTPL std::basic_string<char16_t> ToUEng(const std::basic_string_view<char16_t> str, const Encoding inchset);
template SYSCOMMONTPL std::basic_string<char32_t> ToUEng(const std::basic_string_view<char32_t> str, const Encoding inchset);

template<typename Char>
SYSCOMMONAPI std::basic_string<Char> ToLEng(const std::basic_string_view<Char> str, const Encoding inchset)
{
    using namespace common::str::charset::detail;
    return DirectConv<Char>(str, inchset, EngLower);
}
template SYSCOMMONTPL std::basic_string<char>     ToLEng(const std::basic_string_view<char>     str, const Encoding inchset);
template SYSCOMMONTPL std::basic_string<wchar_t>  ToLEng(const std::basic_string_view<wchar_t>  str, const Encoding inchset);
template SYSCOMMONTPL std::basic_string<char16_t> ToLEng(const std::basic_string_view<char16_t> str, const Encoding inchset);
template SYSCOMMONTPL std::basic_string<char32_t> ToLEng(const std::basic_string_view<char32_t> str, const Encoding inchset);


#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template SYSCOMMONTPL std::basic_string<u8ch_t>   ToUEng(const std::basic_string_view<u8ch_t>   str, const Encoding inchset);
template SYSCOMMONTPL std::basic_string<u8ch_t>   ToLEng(const std::basic_string_view<u8ch_t>   str, const Encoding inchset);
#endif
}

}


