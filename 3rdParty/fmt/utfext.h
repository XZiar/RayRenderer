#pragma once
#include "format.h"
#include "common/CommonRely.hpp"




FMT_BEGIN_NAMESPACE


namespace internal
{




//template <>
//struct char_traits<char16_t>
//{
//    // Formats a floating-point number.
//    template <typename T>
//    FMT_API static int format_float(char16_t *buffer, std::size_t size, const char16_t *format, int precision, T value);
//};
//template <>
//struct char_traits<char32_t>
//{
//    // Formats a floating-point number.
//    template <typename T>
//    FMT_API static int format_float(char32_t *buffer, std::size_t size, const char32_t *format, int precision, T value);
//};



//template<typename Context>
//struct MakeValueProxy
//{
//    template<typename T>
//    static FMT_CONSTEXPR auto map(const T& val)
//    {
//        return arg_mapper<Context>().map(val);
//    }
//};



template <typename Context>
struct UTFMakeValueProxy
{
    using Char = typename Context::char_type;
    /*template<typename T>
    static FMT_CONSTEXPR bool IsChar()
    {
        return std::is_same_v<T, char> || std::is_same_v<T, char16_t> || std::is_same_v<T, wchar_t> || std::is_same_v<T, char32_t>;
    }*/
    template<typename T> 
    static FMT_CONSTEXPR bool IsTransChar()
    {
        return is_char<T>::value && !std::is_same_v<T, Char>;
    }

    template<typename T>
    static constexpr basic_string_view<Char> ToStringValue(const T* ptr, const size_t size)
    { 
        if constexpr (std::is_same_v<T, char>)
            return basic_string_view<Char>(reinterpret_cast<const Char*>(ptr), (size & SizeMask) | CharTag);
        else if constexpr (std::is_same_v<T, char8_t>)
            return basic_string_view<Char>(reinterpret_cast<const Char*>(ptr), (size & SizeMask) | Char8Tag);
        else if constexpr (std::is_same_v<T, char16_t>)
            return basic_string_view<Char>(reinterpret_cast<const Char*>(ptr), (size & SizeMask) | Char16Tag);
        else if constexpr (std::is_same_v<T, char32_t>)
            return basic_string_view<Char>(reinterpret_cast<const Char*>(ptr), (size & SizeMask) | Char32Tag);
        else if constexpr (std::is_same_v<T, wchar_t>)
            return basic_string_view<Char>(reinterpret_cast<const Char*>(ptr), (size & SizeMask) | WCharTag);
        else
            static_assert(!common::AlwaysTrue<T>(), "Non-char type enter here");
    }
    template<typename T>
    static constexpr auto ToStringValue(const T* ptr) { return ToStringValue(ptr, std::char_traits<T>::length(ptr)); }

    template<typename T>
    static FMT_CONSTEXPR auto map(T* val)
    {
        using RawT = std::remove_cv_t<T>;
        //static_assert(IsChar<RawT>(), "non-ch pointer enter here");
        if constexpr (IsTransChar<RawT>())
            return ToStringValue(val);
        else
            return arg_mapper<Context>().map(val);
    }

    template<typename T>
    static FMT_CONSTEXPR auto map(const T& val)
    {
        using common::is_specialization;
        if constexpr (is_specialization<T, basic_string_view>::value
            || is_specialization<T, std::basic_string_view>::value
            || is_specialization<T, std::basic_string>::value)
            return ToStringValue(val.data(), val.size());
        else if constexpr (is_char<T>::value)
            return ToStringValue(&val, 1);
        else if constexpr (std::is_convertible_v<const T&, const std::string_view&>)
            return map(static_cast<const std::string_view&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::basic_string_view<char8_t>&>)
            return map(static_cast<const std::basic_string_view<char8_t>&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::u16string_view&>)
            return map(static_cast<const std::u16string_view&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::u32string_view&>)
            return map(static_cast<const std::u32string_view&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::wstring_view&>)
            return map(static_cast<const std::wstring_view&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::string&>)
            return map(static_cast<const std::string&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::basic_string<char8_t>&>)
            return map(static_cast<const std::basic_string<char8_t>&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::u16string&>)
            return map(static_cast<const std::u16string&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::u32string&>)
            return map(static_cast<const std::u32string&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::wstring&>)
            return map(static_cast<const std::wstring&>(val));
        else
            return arg_mapper<Context>().map(val);
    }

};
template<> struct MakeValueProxy<u16format_context> : public UTFMakeValueProxy<u16format_context> {};
template<> struct MakeValueProxy<u32format_context> : public UTFMakeValueProxy<u32format_context> {};


}

FMT_END_NAMESPACE
