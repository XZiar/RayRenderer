#pragma once
#ifdef FMT_FORMAT_H_
#  error "Don't include format.h before utfext.h"
#endif
#define FMT_USE_GRISU 1
#ifndef FMT_EXPORT
#  define FMT_SHARED
#endif
#include "ranges.h"
#include "chrono.h"
#include "format.h"




FMT_BEGIN_NAMESPACE


namespace detail
{


namespace temp
{
template <class T, template <typename...> class Template>
struct is_specialization : std::false_type {};
template <template <typename...> class Template, typename... Ts>
struct is_specialization<Template<Ts...>, Template> : std::true_type {};

template<typename T>
inline constexpr bool AlwaysTrue = true;

template<typename T>
inline constexpr bool StringHack1 =
    is_specialization<T, std::basic_string>::value ||
    is_specialization<T, std::basic_string_view>::value ||
    is_specialization<T, basic_string_view>::value;

template<typename T>
inline constexpr bool StringHack2 = is_char<T>::value ||
#ifdef __cpp_char8_t
    std::is_convertible_v<const T&, const std::u8string_view&> ||
    std::is_convertible_v<const T&, const std::u8string&> ||
#endif
    std::is_convertible_v<const T&, const std::string_view&> ||
    std::is_convertible_v<const T&, const std::wstring_view&> ||
    std::is_convertible_v<const T&, const std::u16string_view&> ||
    std::is_convertible_v<const T&, const std::u32string_view&> ||
    std::is_convertible_v<const T&, const std::string&> ||
    std::is_convertible_v<const T&, const std::wstring&> ||
    std::is_convertible_v<const T&, const std::u16string&> ||
    std::is_convertible_v<const T&, const std::u32string&>;

}

template <typename Context>
struct UTFMakeValueProxy
{
    using Char = typename Context::char_type;

    template<typename T>
    constexpr basic_string_view<Char> ToStringValue(const T* ptr, const size_t size)
    {
        if constexpr (std::is_same_v<T, char>)
            return basic_string_view<Char>(reinterpret_cast<const Char*>(ptr), (size & SizeMask) | CharTag);
#ifdef __cpp_char8_t
        else if constexpr (std::is_same_v<T, char8_t>)
            return basic_string_view<Char>(reinterpret_cast<const Char*>(ptr), (size & SizeMask) | Char8Tag);
#endif
        else if constexpr (std::is_same_v<T, char16_t>)
            return basic_string_view<Char>(reinterpret_cast<const Char*>(ptr), (size & SizeMask) | Char16Tag);
        else if constexpr (std::is_same_v<T, char32_t>)
            return basic_string_view<Char>(reinterpret_cast<const Char*>(ptr), (size & SizeMask) | Char32Tag);
        else if constexpr (std::is_same_v<T, wchar_t>)
            return basic_string_view<Char>(reinterpret_cast<const Char*>(ptr), (size & SizeMask) | WCharTag);
        else
            static_assert(!temp::AlwaysTrue<T>, "Non-char type enter here");
    }
    template<typename T>
    constexpr auto ToStringValue(const T* ptr) { return ToStringValue(ptr, std::char_traits<T>::length(ptr)); }

    template<typename T, typename RawT = std::remove_cv_t<T>,
        typename = std::enable_if_t<is_char<RawT>::value && !std::is_same_v<RawT, Char>>>
        FMT_CONSTEXPR basic_string_view<Char> map(const T* val)
    {
        return ToStringValue(val);
    }

    template<typename T>
    FMT_CONSTEXPR auto map(const T& val) -> typename std::enable_if_t<temp::StringHack1<T>, basic_string_view<Char>>
    {
        return ToStringValue(val.data(), val.size());
    }
    template<typename T>
    FMT_CONSTEXPR auto map(const T& val) -> typename std::enable_if_t<!temp::StringHack1<T> && temp::StringHack2<T>, basic_string_view<Char>>
    {
        if constexpr (is_char<T>::value)
            return ToStringValue(&val, 1);
        else if constexpr (std::is_convertible_v<const T&, const std::string_view&>)
            return map(static_cast<const std::string_view&>(val));
#ifdef __cpp_char8_t
        else if constexpr (std::is_convertible_v<const T&, const std::basic_string_view<char8_t>&>)
            return map(static_cast<const std::basic_string_view<char8_t>&>(val));
#endif
        else if constexpr (std::is_convertible_v<const T&, const std::u16string_view&>)
            return map(static_cast<const std::u16string_view&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::u32string_view&>)
            return map(static_cast<const std::u32string_view&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::wstring_view&>)
            return map(static_cast<const std::wstring_view&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::string&>)
            return map(static_cast<const std::string&>(val));
#ifdef __cpp_char8_t
        else if constexpr (std::is_convertible_v<const T&, const std::basic_string<char8_t>&>)
            return map(static_cast<const std::basic_string<char8_t>&>(val));
#endif
        else if constexpr (std::is_convertible_v<const T&, const std::u16string&>)
            return map(static_cast<const std::u16string&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::u32string&>)
            return map(static_cast<const std::u32string&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::wstring&>)
            return map(static_cast<const std::wstring&>(val));
        else
            static_assert(!temp::AlwaysTrue<T>, "should not enter here");
    }
    template<typename T>
    FMT_CONSTEXPR auto map(const T& val) -> typename std::enable_if_t<!temp::StringHack1<T> && !temp::StringHack2<T>, decltype(arg_mapper<Context>().map(val))>
    {
        return arg_mapper<Context>().map(val);
    }

};


using u16memory_buffer = basic_memory_buffer<char16_t>;
using u32memory_buffer = basic_memory_buffer<char32_t>;

template<> struct MakeValueProxy<u16format_context> : public UTFMakeValueProxy<u16format_context> {};
template<> struct MakeValueProxy<u32format_context> : public UTFMakeValueProxy<u32format_context> {};


}

FMT_END_NAMESPACE
