#pragma once
#include "format.h"
#include "common/CommonRely.hpp"




FMT_BEGIN_NAMESPACE

typedef basic_string_view<char16_t> u16string_view;
typedef basic_string_view<char32_t> u32string_view;
typedef buffer_context<char16_t>::type u16format_context;
typedef buffer_context<char32_t>::type u32format_context;
typedef basic_memory_buffer<char16_t> u16memory_buffer;
typedef basic_memory_buffer<char32_t> u32memory_buffer;

namespace internal
{

typedef basic_buffer<char16_t> u16buffer;
typedef basic_buffer<char32_t> u32buffer;


template <>
struct char_traits<char16_t>
{
    // Formats a floating-point number.
    template <typename T>
    FMT_API static int format_float(char16_t *buffer, std::size_t size, const char16_t *format, int precision, T value);
};
template <>
struct char_traits<char32_t>
{
    // Formats a floating-point number.
    template <typename T>
    FMT_API static int format_float(char32_t *buffer, std::size_t size, const char32_t *format, int precision, T value);
};


constexpr inline size_t SizeTag   = size_t(0b11) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t CharTag   = size_t(0b00) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t Char16Tag = size_t(0b01) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t Char32Tag = size_t(0b10) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t WCharTag  = sizeof(wchar_t) == sizeof(char16_t) ? Char16Tag : Char32Tag;
constexpr inline size_t SizeMask  = ~SizeTag;


template <typename Char>
struct UTFMakeValueProxy
{
    using Context = typename buffer_context<Char>::type;
    template<typename T>
    static FMT_CONSTEXPR bool IsChar()
    {
        return std::is_same_v<T, char> || std::is_same_v<T, char16_t> || std::is_same_v<T, wchar_t> || std::is_same_v<T, char32_t>;
    }
    template<typename T> 
    static FMT_CONSTEXPR bool IsTransChar()
    {
        return IsChar<T>() && !std::is_same_v<T, Char>;
    }

    template<typename T>
    static constexpr init<typename buffer_context<Char>::type, basic_string_view<Char>, string_type> ToStringValue(const T* ptr, const size_t size)
    { 
        if constexpr (std::is_same_v<T, char>)
            return basic_string_view<Char>(reinterpret_cast<const Char*>(ptr), (size & SizeMask) | CharTag);
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
    static FMT_CONSTEXPR auto make(T* val)
    {
        using RawT = std::remove_cv_t<T>;
        //static_assert(IsChar<RawT>(), "non-ch pointer enter here");
        if constexpr (IsTransChar<RawT>())
            return ToStringValue(val);
        else
            return make_value<Context>(val);
    }

    template<typename T>
    static FMT_CONSTEXPR auto make(const T& val)
    {
        using common::is_specialization;
        if constexpr (is_specialization<T, basic_string_view>::value
            || is_specialization<T, std::basic_string_view>::value
            || is_specialization<T, std::basic_string>::value)
            return ToStringValue(val.data(), val.size());
        else if constexpr (IsChar<T>())
            return ToStringValue(&val, 1);
        else if constexpr (std::is_convertible_v<const T&, const std::string_view&>)
            return make(static_cast<const std::string_view&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::u16string_view&>)
            return make(static_cast<const std::u16string_view&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::u32string_view&>)
            return make(static_cast<const std::u32string_view&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::wstring_view&>)
            return make(static_cast<const std::wstring_view&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::string&>)
            return make(static_cast<const std::string&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::u16string&>)
            return make(static_cast<const std::u16string&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::u32string&>)
            return make(static_cast<const std::u32string&>(val));
        else if constexpr (std::is_convertible_v<const T&, const std::wstring&>)
            return make(static_cast<const std::wstring&>(val));
        else
            return make_value<Context>(val);
    }

};
template<> struct MakeValueProxy<u16format_context> : public UTFMakeValueProxy<char16_t> {};
template<> struct MakeValueProxy<u32format_context> : public UTFMakeValueProxy<char32_t> {};


}


template <typename Range>
class utf_formatter :
    public internal::function<typename internal::arg_formatter_base<Range>::iterator>,
    public internal::arg_formatter_base<Range> 
{
private:
    typedef typename Range::value_type char_type;
    typedef internal::arg_formatter_base<Range> base;
    typedef basic_format_context<typename base::iterator, char_type> context_type;

    context_type &ctx_;
    
    template<typename SrcChar>
    std::basic_string<char_type> ConvertStr(const SrcChar* str, const size_t size);
public:
    typedef Range range;
    typedef typename base::iterator iterator;
    typedef typename base::format_specs format_specs;

    /**
    \rst
    Constructs an argument formatter object.
    *ctx* is a reference to the formatting context,
    *spec* contains format specifier information for standard argument types.
    \endrst
    */
    explicit utf_formatter(context_type &ctx, format_specs *spec = {})
        : base(Range(ctx.out()), spec), ctx_(ctx) {}

    // Deprecated.
    utf_formatter(context_type &ctx, format_specs &spec)
        : base(Range(ctx.out()), &spec), ctx_(ctx) {}

    using base::operator();

    /** Formats an argument of a user-defined type. */
    iterator operator()(typename basic_format_arg<context_type>::handle handle) 
    {
        handle.format(ctx_);
        return this->out();
    }

    iterator operator()(basic_string_view<char_type> value)
    {
        const auto realSize = value.size() & internal::SizeMask;
        switch (value.size() & internal::SizeTag)
        {
        case internal::CharTag: 
            return internal::arg_formatter_base<Range>::operator()(ConvertStr(reinterpret_cast<const char*>(value.data()), realSize));
        case internal::Char16Tag:
            if constexpr (sizeof(char_type) == 2)
                return internal::arg_formatter_base<Range>::operator()(basic_string_view<char_type>(value.data(), realSize));
            else // UTF16 -> UTF32
                return internal::arg_formatter_base<Range>::operator()(ConvertStr(reinterpret_cast<const char16_t*>(value.data()), realSize));
        case internal::Char32Tag:
            if constexpr (sizeof(char_type) == 4)
                return internal::arg_formatter_base<Range>::operator()(basic_string_view<char_type>(value.data(), realSize));
            else // UTF32 -> UTF16
                return internal::arg_formatter_base<Range>::operator()(ConvertStr(reinterpret_cast<const char32_t*>(value.data()), realSize));
        default: // simple passthrough
            return this->out();
        }
    }
};


template <typename T>
inline internal::named_arg<T, char16_t> arg(u16string_view name, const T &arg)
{
    return internal::named_arg<T, char16_t>(name, arg);
}
template <typename T>
inline internal::named_arg<T, char32_t> arg(u32string_view name, const T &arg)
{
    return internal::named_arg<T, char32_t>(name, arg);
}

struct u16format_args : basic_format_args<u16format_context>
{
    template <typename ...Args>
    u16format_args(Args && ... arg)
        : basic_format_args<u16format_context>(std::forward<Args>(arg)...) {}
};
struct u32format_args : basic_format_args<u32format_context>
{
    template <typename ...Args>
    u32format_args(Args && ... arg)
        : basic_format_args<u32format_context>(std::forward<Args>(arg)...) {}
};

inline u16format_context::iterator vformat_to(internal::u16buffer &buf, u16string_view format_str, u16format_args args)
{
    typedef back_insert_range<internal::u16buffer> range;
    return vformat_to<utf_formatter<range>>(buf, format_str, args);
}

inline u32format_context::iterator vformat_to(internal::u32buffer &buf, u32string_view format_str, u32format_args args)
{
    typedef back_insert_range<internal::u32buffer> range;
    return vformat_to<utf_formatter<range>>(buf, format_str, args);
}


FMT_END_NAMESPACE
