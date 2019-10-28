#include "utfext.h"
#include "StringCharset/Convert.h"
#include <ctime>


using common::str::Charset;

FMT_BEGIN_NAMESPACE

namespace internal
{


FMT_API std::size_t strftime(char16_t *str, std::size_t count, const char16_t *format, const std::tm *time)
{
    static thread_local std::string buffer;
    buffer.resize(count);
    const auto u7format = common::strchset::to_string(std::u16string_view(format), Charset::UTF7, Charset::UTF16LE);
    const auto ret = std::strftime(buffer.data(), count, u7format.c_str(), time);
    for (size_t idx = 0; idx < ret;)
        *str++ = buffer[idx++];
    return ret;
}

FMT_API std::size_t strftime(char32_t *str, std::size_t count, const char32_t *format, const std::tm *time)
{
    static thread_local std::string buffer;
    buffer.resize(count);
    const auto u7format = common::strchset::to_string(std::u32string_view(format), Charset::UTF7, Charset::UTF32LE);
    const auto ret = std::strftime(buffer.data(), count, u7format.c_str(), time);
    for (size_t idx = 0; idx < ret;)
        *str++ = buffer[idx++];
    return ret;
}



}


template<>
FMT_API std::basic_string<char16_t> internal::UTFFormatterSupport::ConvertStr(const char* str, const size_t size)
{
    return common::strchset::to_u16string(str, size, Charset::UTF7);
}
template<>
FMT_API std::basic_string<char16_t> internal::UTFFormatterSupport::ConvertU8Str(const char* str, const size_t size)
{
    return common::strchset::to_u16string(str, size, Charset::UTF8);
}
template<>
FMT_API std::basic_string<char16_t> internal::UTFFormatterSupport::ConvertStr(const char32_t* str, const size_t size)
{
    return common::strchset::to_u16string(str, size, Charset::UTF32LE);
}

template<>
FMT_API std::basic_string<char32_t> internal::UTFFormatterSupport::ConvertStr(const char* str, const size_t size)
{
    return common::strchset::to_u32string(str, size, Charset::UTF7);
}
template<>
FMT_API std::basic_string<char32_t> internal::UTFFormatterSupport::ConvertU8Str(const char* str, const size_t size)
{
    return common::strchset::to_u32string(str, size, Charset::UTF8);
}
template<>
FMT_API std::basic_string<char32_t> internal::UTFFormatterSupport::ConvertStr(const char16_t* str, const size_t size)
{
    return common::strchset::to_u32string(str, size, Charset::UTF16LE);
}



using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;

//===== Below mapping from format.cc =====

// Explicit instantiations for char16_t.

template<> FMT_API char16_t internal::thousands_sep_impl(locale_ref loc)
{
    return (char16_t)internal::thousands_sep<char>(loc);
}
template<> FMT_API char16_t internal::decimal_point_impl(locale_ref loc)
{
    return (char16_t)internal::decimal_point_impl<char>(loc);
}

template FMT_API void internal::buffer<char16_t>::append(const char16_t*,
    const char16_t*);

template FMT_API void internal::arg_map<u16format_context>::init(
    const basic_format_args<u16format_context>&);

template FMT_API std::u16string internal::vformat<char16_t>(
    u16string_view, basic_format_args<u16format_context>);

// Explicit instantiations for char32_t.

template<> FMT_API char32_t internal::thousands_sep_impl(locale_ref loc)
{
    return (char32_t)internal::thousands_sep<char>(loc);
}
template<> FMT_API char32_t internal::decimal_point_impl(locale_ref loc)
{
    return (char32_t)internal::decimal_point_impl<char>(loc);
}

template FMT_API void internal::buffer<char32_t>::append(const char32_t*,
    const char32_t*);

template FMT_API void internal::arg_map<u32format_context>::init(
    const basic_format_args<u32format_context>&);

template FMT_API std::u32string internal::vformat<char32_t>(
    u32string_view, basic_format_args<u32format_context>);


FMT_END_NAMESPACE