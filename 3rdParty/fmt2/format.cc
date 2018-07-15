// Formatting library for C++
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "format-inl.h"
#include "utfext.h"

FMT_BEGIN_NAMESPACE
namespace internal {
// Force linking of inline functions into the library.
std::string (*vformat_ref)(string_view, format_args) = vformat;
std::wstring (*vformat_wref)(wstring_view, wformat_args) = vformat;
std::u16string(*vformat_u16ref)(u16string_view, u16format_args) = vformat;
std::u32string(*vformat_u32ref)(u32string_view, u32format_args) = vformat;
}

template struct internal::basic_data<void>;

// Explicit instantiations for char.

template FMT_API char internal::thousands_sep(locale_provider *lp);

template void basic_fixed_buffer<char>::grow(std::size_t);

template void internal::arg_map<format_context>::init(
    const basic_format_args<format_context> &args);

template FMT_API int internal::char_traits<char>::format_float(
    char *buffer, std::size_t size, const char *format, int precision,
    double value);

template FMT_API int internal::char_traits<char>::format_float(
    char *buffer, std::size_t size, const char *format, int precision,
    long double value);

// Explicit instantiations for wchar_t.

template FMT_API wchar_t internal::thousands_sep(locale_provider *lp);

template void basic_fixed_buffer<wchar_t>::grow(std::size_t);

template void internal::arg_map<wformat_context>::init(
    const basic_format_args<wformat_context> &args);

template FMT_API int internal::char_traits<wchar_t>::format_float(
    wchar_t *buffer, std::size_t size, const wchar_t *format,
    int precision, double value);

template FMT_API int internal::char_traits<wchar_t>::format_float(
    wchar_t *buffer, std::size_t size, const wchar_t *format,
    int precision, long double value);


// Explicit instantiations for char16_t and char32_t.

template<> FMT_API char16_t internal::thousands_sep<char16_t>(locale_provider *lp)
{
    return (char16_t)internal::thousands_sep<char>(lp);
}

template void basic_fixed_buffer<char16_t>::grow(std::size_t);

template void internal::arg_map<u16format_context>::init(const basic_format_args<u16format_context> &args);

template<> FMT_API char32_t internal::thousands_sep<char32_t>(locale_provider *lp)
{
    return (char32_t)internal::thousands_sep<char>(lp);
}

template void basic_fixed_buffer<char32_t>::grow(std::size_t);

template void internal::arg_map<u32format_context>::init(const basic_format_args<u32format_context> &args);






FMT_END_NAMESPACE
