#pragma once

#include "StringUtilRely.h"
#include "Convert.h"

#ifdef FMT_FORMAT_H_
#  error "Don't include format.h before Format.h"
#endif
#define FMT_USE_GRISU 1
#ifdef STRCHSET_EXPORT
#  define FMT_EXPORT
#else
#  define FMT_SHARED
#endif

#include "3rdParty/fmt/ranges.h"
#include "3rdParty/fmt/chrono.h"
#include "3rdParty/fmt/format.h"

FMT_BEGIN_NAMESPACE

namespace detail
{

namespace temp
{

template<typename T>
inline constexpr bool StringHack1 =
common::is_specialization<T, std::basic_string>::value ||
common::is_specialization<T, std::basic_string_view>::value ||
common::is_specialization<T, basic_string_view>::value;

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


template<typename DstChar, typename SrcChar>
std::basic_string<DstChar> ConvertStr(const SrcChar* str, const size_t size);
template<typename DstChar>
std::basic_string<DstChar> ConvertU8Str(const char* str, const size_t size);
using Charset = common::str::Charset;

template<>
inline std::basic_string<char16_t> ConvertStr(const char* str, const size_t size)
{
    return common::str::to_u16string(str, size, Charset::UTF7);
}
template<>
inline std::basic_string<char16_t> ConvertU8Str(const char* str, const size_t size)
{
    return common::str::to_u16string(str, size, Charset::UTF8);
}
template<>
inline std::basic_string<char16_t> ConvertStr(const char32_t* str, const size_t size)
{
    return common::str::to_u16string(str, size, Charset::UTF32);
}

template<>
inline std::basic_string<char32_t> ConvertStr(const char* str, const size_t size)
{
    return common::str::to_u32string(str, size, Charset::UTF7);
}
template<>
inline std::basic_string<char32_t> ConvertU8Str(const char* str, const size_t size)
{
    return common::str::to_u32string(str, size, Charset::UTF8);
}
template<>
inline std::basic_string<char32_t> ConvertStr(const char16_t* str, const size_t size)
{
    return common::str::to_u32string(str, size, Charset::UTF16);
}

[[nodiscard]] STRCHSETAPI std::string& GetLocalString();

}

inline std::size_t strftime(char16_t* str, std::size_t count, const char16_t* format, const std::tm* time)
{
    auto& buffer = temp::GetLocalString();
    buffer.resize(count);
    const auto u7format = common::str::to_string(std::u16string_view(format), temp::Charset::UTF7);
    const auto ret = std::strftime(buffer.data(), count, u7format.c_str(), time);
    for (size_t idx = 0; idx < ret;)
        *str++ = buffer[idx++];
    return ret;
}

inline std::size_t strftime(char32_t* str, std::size_t count, const char32_t* format, const std::tm* time)
{
    auto& buffer = temp::GetLocalString();
    buffer.resize(count);
    const auto u7format = common::str::to_string(std::u32string_view(format), temp::Charset::UTF7);
    const auto ret = std::strftime(buffer.data(), count, u7format.c_str(), time);
    for (size_t idx = 0; idx < ret;)
        *str++ = buffer[idx++];
    return ret;
}


constexpr inline size_t SizeTag   = size_t(0b11) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t CharTag   = size_t(0b00) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t Char8Tag  = size_t(0b01) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t Char16Tag = size_t(0b10) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t Char32Tag = size_t(0b11) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t WCharTag  = sizeof(wchar_t) == sizeof(char16_t) ? Char16Tag : Char32Tag;
constexpr inline size_t SizeMask  = ~SizeTag;

template <typename Context>
struct UTFArgMapperProxy
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
            static_assert(!common::AlwaysTrue<T>, "Non-char type enter here");
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
    FMT_CONSTEXPR auto map(const T& val) -> typename std::enable_if_t<!temp::StringHack1<T>&& temp::StringHack2<T>, basic_string_view<Char>>
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
            static_assert(!common::AlwaysTrue<T>, "should not enter here");
    }
    template<typename T>
    FMT_CONSTEXPR auto map(const T& val) -> typename std::enable_if_t<!temp::StringHack1<T> && !temp::StringHack2<T>, 
        decltype(arg_mapper_<Context>().map(val))>
    {
        return arg_mapper_<Context>().map(val);
    }

};


template<typename Char>
struct StringHacker
{
    template<typename Ret, typename Func>
    static forceinline Ret HandleString(const basic_string_view<Char> str, Func&& func)
    {
        const auto realSize = str.size() & SizeMask;
        switch (str.size() & SizeTag)
        {
        case CharTag:
            return func(temp::ConvertStr<Char>(reinterpret_cast<const char*>(str.data()), realSize));
        case Char8Tag:
            return func(temp::ConvertU8Str<Char>(reinterpret_cast<const char*>(str.data()), realSize));
        case Char16Tag:
            if constexpr (sizeof(Char) == 2)
                return func(std::basic_string_view<Char>(str.data(), realSize));
            else // UTF16 -> UTF32
                return func(temp::ConvertStr<Char>(reinterpret_cast<const char16_t*>(str.data()), realSize));
        case Char32Tag:
            if constexpr (sizeof(Char) == 4)
                return func(std::basic_string_view<Char>(str.data(), realSize));
            else // UTF32 -> UTF16
                return func(temp::ConvertStr<Char>(reinterpret_cast<const char32_t*>(str.data()), realSize));
        default: // simple passthrough, should not enter
            return func(std::basic_string_view<Char>(str.data(), realSize));
        }
    }
};

template<> struct StringProcess<char16_t> : public StringHacker<char16_t> {};
template<> struct StringProcess<char32_t> : public StringHacker<char32_t> {};


using u16memory_buffer = basic_memory_buffer<char16_t>;
using u32memory_buffer = basic_memory_buffer<char32_t>;
template<> struct arg_mapper<buffer_context<char16_t>> : public UTFArgMapperProxy<buffer_context<char16_t>> {};
template<> struct arg_mapper<buffer_context<char32_t>> : public UTFArgMapperProxy<buffer_context<char32_t>> {};
}

using u16format_context = buffer_context<char16_t>;
using u32format_context = buffer_context<char32_t>;

FMT_END_NAMESPACE

