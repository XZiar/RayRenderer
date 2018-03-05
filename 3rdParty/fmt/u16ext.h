#pragma once
#include "format.h"

namespace fmt
{

namespace internal
{
    template <>
    class CharTraits<char16_t> : public BasicCharTraits<char16_t>
    {
    public:
        static char16_t convert(wchar_t);
        static char16_t convert(char value) { return value; }

        // Formats a floating-point number.
        template <typename T>
        FMT_API static int format_float(char16_t *buffer, std::size_t size,
            const char16_t *format, unsigned width, int precision, T value);
    };
    template <>
    class CharTraits<char32_t> : public BasicCharTraits<char32_t>
    {
    public:
        static char32_t convert(wchar_t);
        static char32_t convert(char value) { return value; }

        // Formats a floating-point number.
        template <typename T>
        FMT_API static int format_float(char32_t *buffer, std::size_t size,
            const char32_t *format, unsigned width, int precision, T value);
    };

    inline void format_arg(fmt::BasicFormatter<char16_t> &f, const char16_t *&format_str, const std::string& s)
    {
        const std::u16string tmp(s.cbegin(), s.cend());
        f.writer().write(tmp);
    }
    void format_arg(fmt::BasicFormatter<char16_t> &f, const char16_t *&format_str, const std::wstring& s);
    inline void format_arg(fmt::BasicFormatter<char16_t> &f, const char16_t *&format_str, const std::u16string& s)
    {
        f.writer().write(s);
    }
}

using u16CStringRef = fmt::BasicCStringRef<char16_t>;
inline std::u16string format(u16CStringRef format_str, ArgList args)
{
    fmt::BasicMemoryWriter<char16_t> w;
    w.write(format_str, args);
    return w.str();
}


# define FMT_ASSIGN_char16_t(n) \
    arr[n] = fmt::internal::MakeValue< fmt::BasicFormatter<char16_t> >(v##n)
}

#define FMT_VARIADIC_U16(ReturnType, func, ...) \
  FMT_VARIADIC_(, char16_t, ReturnType, func, return func, __VA_ARGS__)

#define FMT_VARIADIC_CONST_U16(ReturnType, func, ...) \
  FMT_VARIADIC_(const, char16_t, ReturnType, func, return func, __VA_ARGS__)

#define FMT_CAPTURE_ARG_U16_(id, index) ::fmt::arg(u###id, id)
#define FMT_CAPTURE_U16(...) FMT_FOR_EACH(FMT_CAPTURE_ARG_U16_, __VA_ARGS__)

namespace fmt
{
FMT_VARIADIC_U16(std::u16string, format, u16CStringRef)

}