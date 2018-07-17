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


constexpr inline size_t SizeTag = size_t(0b11) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t CharTag = size_t(0b00) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t Char16Tag = size_t(0b01) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t WCharTag = size_t(0b10) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t Char32Tag = size_t(0b11) << (sizeof(size_t) * 8 - 2);
constexpr inline size_t SizeMask = ~SizeTag;

template<typename Char>
struct StrConv
{
    union
    {
        const Char* const Ptr;
        const char* const PtrChar;
        const char16_t* const PtrChar16;
        const wchar_t* const PtrWChar;
        const char32_t* const PtrChar32;
    };
    const size_t Size;
    constexpr StrConv(const char* ptr) : StrConv(ptr, std::char_traits<char>::length(ptr)) {}
    constexpr StrConv(const char16_t* ptr) : StrConv(ptr, std::char_traits<char16_t>::length(ptr)) {}
    constexpr StrConv(const wchar_t* ptr) : StrConv(ptr, std::char_traits<wchar_t>::length(ptr)) {}
    constexpr StrConv(const char32_t* ptr) : StrConv(ptr, std::char_traits<char32_t>::length(ptr)) {}
    constexpr StrConv(const char* ptr, const size_t size) : PtrChar(ptr), Size((size & SizeMask) | CharTag) {}
    constexpr StrConv(const char16_t* ptr, const size_t size) : PtrChar16(ptr), Size((size & SizeMask) | Char16Tag) {}
    constexpr StrConv(const wchar_t* ptr, const size_t size) : PtrWChar(ptr), Size((size & SizeMask) | WCharTag) {}
    constexpr StrConv(const char32_t* ptr, const size_t size) : PtrChar32(ptr), Size((size & SizeMask) | Char32Tag) {}
    constexpr basic_string_view<Char> Result() const { return basic_string_view<Char>(Ptr, Size); }
    constexpr typed_value<typename buffer_context<Char>::type, string_type> Value() const { return basic_string_view<Char>(Ptr, Size); }
};


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
    static FMT_CONSTEXPR auto make(const T& val)
    {
        if constexpr(IsChar<T>())
            return StrConv<Char>(&val, 1).Value();
        else
            return make_value<Context>(val);
    }

    template<typename T>
    static FMT_CONSTEXPR auto make(const T* val)
    {
        if constexpr(IsTransChar<T>())
            return StrConv<Char>(val).Value();
        else
            return make_value<Context>(val);
    }
    template<typename T>
    static FMT_CONSTEXPR auto make(T* val)
    {
        if constexpr(IsTransChar<T>())
            return StrConv<Char>(val).Value();
        else
            return make_value<Context>(val);
    }

    template<typename T>
    static FMT_CONSTEXPR auto make(const basic_string_view<T>& val)
    {
        return StrConv<Char>(val.data(), val.size()).Value();
    }
    template<typename T>
    static FMT_CONSTEXPR auto make(const std::basic_string_view<T>& val)
    {
        return StrConv<Char>(val.data(), val.size()).Value();
    }
    template<typename T>
    static FMT_CONSTEXPR auto make(const std::basic_string<T>& val)
    {
        return StrConv<Char>(val.data(), val.size()).Value();
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
    utf_formatter(context_type &ctx, format_specs &spec)
        : base(Range(ctx.out()), spec), ctx_(ctx) {}

    using base::operator();

    /** Formats an argument of a user-defined type. */
    iterator operator()(typename basic_format_arg<context_type>::handle handle) 
    {
        handle.format(ctx_);
        return this->out();
    }

    iterator operator()(basic_string_view<char_type> value)
    {
        internal::check_string_type_spec(this->spec().type(), internal::error_handler());
        uint8_t charSize = 0;
        switch (value.size() & internal::SizeTag)
        {
        case internal::CharTag: charSize = 1; break;
        case internal::Char16Tag: charSize = 2; break;
        case internal::Char32Tag: charSize = 4; break;
        case internal::WCharTag: charSize = sizeof(wchar_t); break;
        }
        const auto realSize = value.size() & internal::SizeMask;
        switch (charSize)
        {
        case 1:
            return internal::arg_formatter_base<Range>::operator()(
                std::basic_string<char_type>(reinterpret_cast<const char*>(value.data()), reinterpret_cast<const char*>(value.data()) + realSize));
        case 2:
            if constexpr (sizeof(char_type) == 2)
                return internal::arg_formatter_base<Range>::operator()(basic_string_view<char_type>(value.data(), realSize));
            else // UTF32
                return internal::arg_formatter_base<Range>::operator()(ConvertStr(reinterpret_cast<const char16_t*>(value.data()), realSize));
        case 4:
            if constexpr (sizeof(char_type) == 4)
                return internal::arg_formatter_base<Range>::operator()(basic_string_view<char_type>(value.data(), realSize));
            else // UTF16
                return internal::arg_formatter_base<Range>::operator()(ConvertStr(reinterpret_cast<const char32_t*>(value.data()), realSize));
        }
        return this->out();
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


template <typename... Args, std::size_t SIZE = inline_buffer_size>
inline u16format_context::iterator format_to(basic_memory_buffer<char16_t, SIZE> &buf, u16string_view format_str, const Args & ... args)
{
    return vformat_to(buf, format_str, make_format_args<u16format_context>(args...));
}

template <typename... Args, std::size_t SIZE = inline_buffer_size>
inline u32format_context::iterator format_to(basic_memory_buffer<char32_t, SIZE> &buf, u32string_view format_str, const Args & ... args)
{
    return vformat_to(buf, format_str, make_format_args<u32format_context>(args...));
}

inline std::u16string vformat(u16string_view format_str, u16format_args args) 
{
    u16memory_buffer buffer;
    vformat_to(buffer, format_str, args);
    return fmt::to_string(buffer);
}

inline std::u32string vformat(u32string_view format_str, u32format_args args) 
{
    u32memory_buffer buffer;
    vformat_to(buffer, format_str, args);
    return to_string(buffer);
}


FMT_END_NAMESPACE
