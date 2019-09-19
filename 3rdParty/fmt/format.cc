// Formatting library for C++
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "utfext.h"
#include "format-inl.h"

FMT_BEGIN_NAMESPACE
template struct FMT_API internal::basic_data<void>;

// Workaround a bug in MSVC2013 that prevents instantiation of grisu_format.
bool (*instantiate_grisu_format)(double, internal::buffer<char>&, int, unsigned,
                                 int&) = internal::grisu_format;

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
template FMT_API internal::locale_ref::locale_ref(const std::locale& loc);
template FMT_API std::locale internal::locale_ref::get<std::locale>() const;
#endif

// Explicit instantiations for char.

template FMT_API char internal::thousands_sep_impl(locale_ref);
template FMT_API char internal::decimal_point_impl(locale_ref);

template FMT_API void internal::buffer<char>::append(const char*, const char*);

template FMT_API void internal::arg_map<format_context>::init(
    const basic_format_args<format_context>& args);

template FMT_API std::string internal::vformat<char>(
    string_view, basic_format_args<format_context>);

template FMT_API format_context::iterator internal::vformat_to(
    internal::buffer<char>&, string_view, basic_format_args<format_context>);

template FMT_API char* internal::sprintf_format(double, internal::buffer<char>&,
                                                sprintf_specs);
template FMT_API char* internal::sprintf_format(long double,
                                                internal::buffer<char>&,
                                                sprintf_specs);

// Explicit instantiations for wchar_t.

template FMT_API wchar_t internal::thousands_sep_impl(locale_ref);
template FMT_API wchar_t internal::decimal_point_impl(locale_ref);

template FMT_API void internal::buffer<wchar_t>::append(const wchar_t*,
                                                        const wchar_t*);

template FMT_API void internal::arg_map<wformat_context>::init(
    const basic_format_args<wformat_context>&);

template FMT_API std::wstring internal::vformat<wchar_t>(
    wstring_view, basic_format_args<wformat_context>);

    
// ++UTF++
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
