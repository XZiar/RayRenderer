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

template<typename Conv, typename Char>
[[nodiscard]] static forcenoinline bool ConvertString(const common::span<const std::byte> data, const Encoding inchset, std::basic_string<Char>& dst) noexcept
{
    using namespace common::str::charset::detail;
    const auto srcptr = reinterpret_cast<const uint8_t*>(data.data());
    const size_t count = data.size();
    switch (inchset)
    {
    case Encoding::ASCII:
        Transform2<UTF7,    Conv>(srcptr, count, dst); break;
    case Encoding::UTF8:
        Transform2<UTF8,    Conv>(srcptr, count, dst); break;
    case Encoding::URI:
        Transform2<URI,     Conv>(srcptr, count, dst); break;
    case Encoding::UTF16LE:
        Transform2<UTF16LE, Conv>(srcptr, count, dst); break;
    case Encoding::UTF16BE:
        Transform2<UTF16BE, Conv>(srcptr, count, dst); break;
    case Encoding::UTF32LE:
        Transform2<UTF32LE, Conv>(srcptr, count, dst); break;
    case Encoding::UTF32BE:
        Transform2<UTF32BE, Conv>(srcptr, count, dst); break;
    case Encoding::GB18030:
        Transform2<GB18030, Conv>(srcptr, count, dst); break;
    default:
        return false;
    }
    return true;
}

std::string to_string(const common::span<const std::byte> data, const Encoding outchset, const Encoding inchset)
{
    using namespace common::str::charset::detail;
    std::basic_string<char> ret;
    bool valid = false;
    switch (outchset)
    {
    case Encoding::ASCII:
        valid = ConvertString<UTF7   >(data, inchset, ret); break;
    case Encoding::UTF8:
        valid = ConvertString<UTF8   >(data, inchset, ret); break;
    case Encoding::URI:
        valid = ConvertString<URI    >(data, inchset, ret); break;
    case Encoding::UTF16LE:
        valid = ConvertString<UTF16LE>(data, inchset, ret); break;
    case Encoding::UTF16BE:
        valid = ConvertString<UTF16BE>(data, inchset, ret); break;
    case Encoding::UTF32LE:
        valid = ConvertString<UTF32LE>(data, inchset, ret); break;
    case Encoding::UTF32BE:
        valid = ConvertString<UTF32BE>(data, inchset, ret); break;
    case Encoding::GB18030:
        valid = ConvertString<GB18030>(data, inchset, ret); break;
    default: break;
    }
    if (!valid)
        COMMON_THROW(BaseException, u"unknown charset");
    return ret;
}

std::u32string  to_u32string(const common::span<const std::byte> data, const Encoding inchset)
{
    std::basic_string<char32_t> ret;
    if (!ConvertString<common::str::charset::detail::UTF32LE>(data, inchset, ret))
        COMMON_THROW(BaseException, u"unknown charset");
    return ret;
}
std::u16string  to_u16string(const common::span<const std::byte> data, const Encoding inchset)
{
    std::basic_string<char16_t> ret;
    if (!ConvertString<common::str::charset::detail::UTF16LE>(data, inchset, ret))
        COMMON_THROW(BaseException, u"unknown charset");
    return ret;
}
str::u8string   to_u8string (const common::span<const std::byte> data, const Encoding inchset)
{
    std::basic_string<u8ch_t> ret;
    if (!ConvertString<common::str::charset::detail::UTF8   >(data, inchset, ret))
        COMMON_THROW(BaseException, u"unknown charset");
    return ret;
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


