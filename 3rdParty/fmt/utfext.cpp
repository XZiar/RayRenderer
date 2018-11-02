#include "utfext.h"
#include "common/StrCharset.hpp"

FMT_BEGIN_NAMESPACE

namespace internal
{

template <typename Src, typename Char, typename T>
static int format_float_(Char *buffer, std::size_t size, const Char *format, int precision, T value)
{
    if constexpr(sizeof(Char) == sizeof(wchar_t))
    {
        return char_traits<wchar_t>::format_float((wchar_t*)buffer, size, (const wchar_t*)format, precision, value);
    }
    else
    {
        char chbuffer[256];
        char chformat[256] = { 0 };
        for (size_t i = 0, j = 0; j < size && j < 255;)
        {
            const auto[ch, len] = Src::FromBytes(reinterpret_cast<const char*>(format) + i, 4, true);
            if (ch == 0)
                break;
            i += len;
            if (ch == common::str::detail::InvalidChar)
                continue;
            j += common::str::detail::UTF7::To(ch, 255 - j, &chformat[j]);
        }
        const auto cnt = char_traits<char>::format_float(chbuffer, 255, chformat, precision, value);
        for (int i = 0; i < cnt; ++i)
            buffer[i] = chbuffer[i];
        return cnt;
    }
}

template<>
FMT_API int char_traits<char16_t>::format_float(char16_t *buffer, std::size_t size, const char16_t *format, int precision, double value)
{
    return format_float_<common::str::detail::UTF16>(buffer, size, format, precision, value);
}

template<>
FMT_API int char_traits<char16_t>::format_float(char16_t *buffer, std::size_t size, const char16_t *format, int precision, long double value)
{
    return format_float_<common::str::detail::UTF16>(buffer, size, format, precision, value);
}

template<>
FMT_API int char_traits<char32_t>::format_float(char32_t *buffer, std::size_t size, const char32_t *format, int precision, double value)
{
    return format_float_<common::str::detail::UTF32>(buffer, size, format, precision, value);
}

template<>
FMT_API int char_traits<char32_t>::format_float(char32_t *buffer, std::size_t size, const char32_t *format, int precision, long double value)
{
    return format_float_<common::str::detail::UTF32>(buffer, size, format, precision, value);
}


std::size_t strftime(char16_t *str, std::size_t count, const char16_t *format, const std::tm *time)
{
    using namespace common::str::detail;
    static thread_local std::string buffer;
    buffer.resize(count);
    const auto u7format = CharsetConvertor<UTF16, UTF7, char16_t, char>::Convert(format, std::char_traits<char16_t>::length(format), true, true);
    const auto ret = std::strftime(buffer.data(), count, u7format.c_str(), time);
    for (size_t idx = 0; idx < ret;)
        *str++ = buffer[idx++];
    return ret;
}

std::size_t strftime(char32_t *str, std::size_t count, const char32_t *format, const std::tm *time)
{
    using namespace common::str::detail;
    static thread_local std::string buffer;
    buffer.resize(count);
    const auto u7format = CharsetConvertor<UTF32, UTF7, char32_t, char>::Convert(format, std::char_traits<char32_t>::length(format), true, true);
    const auto ret = std::strftime(buffer.data(), count, u7format.c_str(), time);
    for (size_t idx = 0; idx < ret;)
        *str++ = buffer[idx++];
    return ret;
}



}

#if defined(_WIN32)
using CharCharSet = common::str::detail::GB18030;
#else
using CharCharSet = common::str::detail::UTF8;
#endif

template<> template<>
std::basic_string<char16_t> utf_formatter<back_insert_range<internal::u16buffer>>::ConvertStr(const char* str, const size_t size)
{
    using namespace common::str::detail;
    return CharsetConvertor<CharCharSet, UTF16, char, char16_t>::Convert(str, size, true, true);
}
template<> template<>
std::basic_string<char16_t> utf_formatter<back_insert_range<internal::u16buffer>>::ConvertStr(const char32_t* str, const size_t size)
{
    using namespace common::str::detail;
    return CharsetConvertor<UTF32, UTF16, char32_t, char16_t>::Convert(str, size, true, true);
}

template<> template<>
std::basic_string<char32_t> utf_formatter<back_insert_range<internal::u32buffer>>::ConvertStr(const char* str, const size_t size)
{
    using namespace common::str::detail;
    return CharsetConvertor<CharCharSet, UTF32, char, char32_t>::Convert(str, size, true, true);
}
template<> template<>
std::basic_string<char32_t> utf_formatter<back_insert_range<internal::u32buffer>>::ConvertStr(const char16_t* str, const size_t size)
{
    using namespace common::str::detail;
    return CharsetConvertor<UTF16, UTF32, char16_t, char32_t>::Convert(str, size, true, true);
}


FMT_END_NAMESPACE