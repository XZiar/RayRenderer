#include "u16ext.h"
#include "common/StrCharset.hpp"
#include "3rdParty/dtoa_milo.hpp"

namespace fmt::internal 
{
    template<size_t SizeWchar>
    char16_t ToChar16(const wchar_t ch)
    {
        static_assert(false, "sizeof(wchar_t) mismatch char16_t nor char32_t");
    }
    template<>
    char16_t ToChar16<sizeof(char16_t)>(const wchar_t ch) { return ch; }
    template<>
    char16_t ToChar16<sizeof(char32_t)>(const wchar_t ch)
    { 
        char16_t tmp[2];
        auto cnt = common::str::detail::UTF16::ToBytes((char32_t)ch, 2, true, (char*)&tmp[0]);
        if (cnt == 1)
            return tmp[0];
        else
            return 0xfffd; //unrecognizable
    }

    char16_t CharTraits<char16_t>::convert(wchar_t ch)
    {
        return ToChar16<sizeof(wchar_t)>(ch);
    }
    
    template <typename Char, typename T>
    int format_float(Char *buffer, std::size_t size, const Char *format, unsigned width, int precision, T value)
    {
        if constexpr(sizeof(Char) == sizeof(wchar_t))
        {
            return CharTraits<wchar_t>::format_float((wchar_t*)buffer, size, (const wchar_t*)format, width, precision, value);
        }
        else
        {
            char chbuffer[256];
            char chformat[256] = { 0 };
            for (int i = 0, j = 0; j < 255;)
            {
                uint8_t len = 0;
                const auto ch = common::str::detail::UTF16::From(format[i], 4, len);
                if (ch == 0)
                    break;
                i += len;
                if (ch == -1)
                    continue;
                j += common::str::detail::UTF7::To(ch, 255 - j, &chformat[j]);
            }
            const auto cnt = CharTraits<char>::format_float(chbuffer, 255, chformat, width, precision, value);
            for (int i = 0; i < cnt; ++i)
                buffer[i] = chbuffer[i];
            return cnt;
        }
    }
    
    template<>
    FMT_API int CharTraits<char16_t>::format_float(char16_t *buffer, std::size_t size, const char16_t *format, unsigned width, int precision, double value)
    {
        return format_float(buffer, size, format, width, precision, value);
    }

    template<>
    FMT_API int CharTraits<char16_t>::format_float(char16_t *buffer, std::size_t size, const char16_t *format, unsigned width, int precision, long double value)
    {
        return format_float(buffer, size, format, width, precision, value);
    }

    template<>
    FMT_API int CharTraits<char32_t>::format_float(char32_t *buffer, std::size_t size, const char32_t *format, unsigned width, int precision, double value)
    {
        return format_float(buffer, size, format, width, precision, value);
    }

    template<>
    FMT_API int CharTraits<char32_t>::format_float(char32_t *buffer, std::size_t size, const char32_t *format, unsigned width, int precision, long double value)
    {
        return format_float(buffer, size, format, width, precision, value);
    }


    template<size_t SizeWchar>
    std::u16string ToChar16(const std::wstring& str)
    {
        static_assert(false, "sizeof(wchar_t) mismatch char16_t nor char32_t");
    }
    template<>
    std::u16string ToChar16<4>(const std::wstring& str)
    {
        return common::str::to_u16string(str, common::str::Charset::UTF32);
    }
    void format_arg(fmt::BasicFormatter<char16_t> &f, const char16_t *&format_str, const std::wstring& s)
    {
        if constexpr(sizeof(wchar_t) == sizeof(char16_t))
        {
            f.writer().write(*(const std::u16string*)&s);
        }
        else
            f.writer().write(ToChar16<sizeof(wchar_t)>(s));
    }
}
