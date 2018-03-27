#pragma once
#include "format.h"
#include "common/StrCharset.hpp"

namespace fmt
{
template<typename Char>
using BasicFormatterExt = BasicFormatter<Char>;

namespace internal
{
    constexpr inline size_t SizeTag = size_t(1) << (sizeof(size_t) * 8 - 1);
    constexpr inline size_t SizeMask = ~SizeTag;
    
    inline uint64_t make_type2() { return 0; }
    template<typename T>
    inline uint64_t make_type2(const T &arg) { return make_type(arg); }
    inline uint64_t make_type2(const char* arg) { return Arg::STRING; }
    inline uint64_t make_type2(const char16_t* arg) { return Arg::STRING; }
    inline uint64_t make_type2(const wchar_t* arg) { return Arg::WSTRING; }
    inline uint64_t make_type2(const char32_t* arg) { return Arg::WSTRING; }
    template<typename T, typename A>
    inline uint64_t make_type2(const std::basic_string<char, T, A> &arg) { return Arg::STRING; }
    template<typename T, typename A>
    inline uint64_t make_type2(const std::basic_string<char16_t, T, A> &arg) { return Arg::STRING; }
    template<typename T, typename A>
    inline uint64_t make_type2(const std::basic_string<wchar_t, T, A> &arg) { return Arg::WSTRING; }
    template<typename T, typename A>
    inline uint64_t make_type2(const std::basic_string<char32_t, T, A> &arg) { return Arg::WSTRING; }
    template<typename T>
    inline uint64_t make_type2(const std::basic_string_view<char, T> &arg) { return Arg::STRING; }
    template<typename T>
    inline uint64_t make_type2(const std::basic_string_view<char16_t, T> &arg) { return Arg::STRING; }
    template<typename T>
    inline uint64_t make_type2(const std::basic_string_view<wchar_t, T> &arg) { return Arg::WSTRING; }
    template<typename T>
    inline uint64_t make_type2(const std::basic_string_view<char32_t, T> &arg) { return Arg::WSTRING; }

    template <typename Arg, typename... Args>
    inline uint64_t make_type2(const Arg &first, const Args & ... tail) 
    {
        return make_type2(first) | (make_type2(tail...) << 4);
    }

    template <std::size_t N, bool = (N < ArgList::MAX_PACKED_ARGS)>
    struct ArgArray2;

    template <std::size_t N>
    struct ArgArray2<N, true/*IsPacked*/> : public ArgArray<N, true>
    {
        static Value FromStr(const BasicStringRef<char> &value)
        {
            Value ret;
            ret.string.value = value.data();
            ret.string.size = value.size();
            return ret;
        }
        static Value FromStr(const BasicStringRef<char16_t> &value)
        {
            Value ret;
            ret.string.value = reinterpret_cast<const char*>(value.data());
            ret.string.size = value.size() | SizeTag;
            return ret;
        }
        static Value FromStr(const BasicStringRef<wchar_t> &value)
        {
            Value ret;
            ret.wstring.value = value.data();
            ret.wstring.size = value.size();
            return ret;
        }
        static Value FromStr(const BasicStringRef<char32_t> &value)
        {
            Value ret;
            ret.wstring.value = reinterpret_cast<const wchar_t*>(value.data());
            ret.wstring.size = value.size() | SizeTag;
            return ret;
        }

        template <typename T>
        static Value make(const T &value)
        {
        #ifdef __clang__
            Value result = MakeValue<Formatter>(value);
            // Workaround a bug in Apple LLVM version 4.2 (clang-425.0.28) of clang:
            // https://github.com/fmtlib/fmt/issues/276
            (void)result.custom.format;
            return result;
        #else
            return MakeValue<BasicFormatter<char>>(value);
        #endif
        }
        static Value make(const char* value) { return FromStr(value); }
        static Value make(const char16_t* value) { return FromStr(value); }
        static Value make(const char32_t* value) { return FromStr(value); }
        static Value make(const wchar_t* value) { return FromStr(value); }
        template<typename Char>
        static Value make(const std::basic_string<Char> &value) { return FromStr(value); }
        template<typename Char>
        static Value make(const std::basic_string_view<Char> &value) { return FromStr(value); }
    };

    template <std::size_t N>
    struct ArgArray2<N, false/*IsPacked*/>  : public ArgArray<N, false>
    {
        template <typename T>
        static Arg make(const T &value) 
        { 
            return Arg{ ArgArray2<N, true>::make_crc_table(value), make_type2(value) };
        }
    };
}

# define FMT_ASSIGN_char16_t(n) \
  arr[n] = fmt::internal::MakeValue< fmt::BasicFormatterExt<char16_t> >(u##n)
# define FMT_ASSIGN_char32_t(n) \
  arr[n] = fmt::internal::MakeValue< fmt::BasicFormatterExt<char32_t> >(U##n)

# define FMT_VARIADIC_VOID_EXT(func, arg_type) \
  template <typename... Args> \
  void func(arg_type arg0, const Args & ... args) { \
    typedef fmt::internal::ArgArray<sizeof...(Args)> ArgArray; \
    typedef fmt::internal::ArgArray2<sizeof...(Args)> ArgArrayExt; \
    typename ArgArray::Type array{ \
      ArgArrayExt::make(args)...}; \
    func(arg0, fmt::ArgList(fmt::internal::make_type2(args...), array)); \
  }
# define FMT_VARIADIC_EXT_(Const, Char, ReturnType, func, call, ...) \
  template <typename... Args> \
  ReturnType func(FMT_FOR_EACH(FMT_ADD_ARG_NAME, __VA_ARGS__), \
      const Args & ... args) Const { \
    typedef fmt::internal::ArgArray<sizeof...(Args)> ArgArray; \
    typedef fmt::internal::ArgArray2<sizeof...(Args)> ArgArrayExt; \
    typename ArgArray::Type array{ \
      ArgArrayExt::make(args)...}; \
    call(FMT_FOR_EACH(FMT_GET_ARG_NAME, __VA_ARGS__), \
      fmt::ArgList(fmt::internal::make_type2(args...), array)); \
  }

namespace internal
{
    template <>
    class CharTraits<char16_t> : public BasicCharTraits<char16_t>
    {
    public:
        static char16_t convert(char16_t value) { return value; }

        // Formats a floating-point number.
        template <typename T>
        FMT_API static int format_float(char16_t *buffer, std::size_t size,
            const char16_t *format, unsigned width, int precision, T value);
    };
    template <>
    class CharTraits<char32_t> : public BasicCharTraits<char32_t>
    {
    public:
        static char32_t convert(char32_t value) { return value; }

        // Formats a floating-point number.
        template <typename T>
        FMT_API static int format_float(char32_t *buffer, std::size_t size,
            const char32_t *format, unsigned width, int precision, T value);
    };


    template <typename T>
    struct WCharHelper<T, char16_t> 
    {
        typedef T Supported;
        typedef Null<T> Unsupported;
    };
    template <typename T>
    struct WCharHelper<T, char32_t>
    {
        typedef T Supported;
        typedef Null<T> Unsupported;
    };

}


template<typename Char, typename Spec = fmt::FormatSpec>
class UTFArgFormatter : public BasicArgFormatter<UTFArgFormatter<Char>, Char, Spec>
{
private:
public:
    UTFArgFormatter(BasicFormatter<Char, UTFArgFormatter<Char>> &f, fmt::FormatSpec &s, const Char *fmt)
        : BasicArgFormatter<UTFArgFormatter<Char>, Char, Spec>(f, s, fmt) {}

    void visit_string(internal::Arg::StringValue<char> value);

    void visit_wstring(internal::Arg::StringValue<wchar_t> value) 
    {
        if (value.size & internal::SizeTag) //u32string
        {
            value.size &= internal::SizeMask;
            visit_wstring(*reinterpret_cast<internal::Arg::StringValue<char32_t>*>(&value));
        }
        else //wstring
        {
            if constexpr(sizeof(wchar_t) == sizeof(char16_t))
                visit_wstring(*reinterpret_cast<internal::Arg::StringValue<char16_t>*>(&value));
            else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
                visit_wstring(*reinterpret_cast<internal::Arg::StringValue<char32_t>*>(&value));
        }
    }

    void visit_wstring(internal::Arg::StringValue<char16_t> value);
    void visit_wstring(internal::Arg::StringValue<char32_t> value);
};
template class UTFArgFormatter<char16_t>;
template class UTFArgFormatter<char32_t>;

template<>
inline void UTFArgFormatter<char16_t>::visit_string(internal::Arg::StringValue<char> value)
{
    if (value.size & internal::SizeTag) //u16string
    {
        value.size &= internal::SizeMask;
        visit_wstring(*reinterpret_cast<internal::Arg::StringValue<char16_t>*>(&value));
    }
    else //string
    {
        const auto u16str = common::str::to_u16string(value.value, value.size, common::str::Charset::UTF7);
        BasicArgFormatter<UTFArgFormatter<char16_t>, char16_t>::visit_wstring(internal::Arg::StringValue<char16_t>{ u16str.data(), u16str.size() });
    }
}
template<>
inline void UTFArgFormatter<char16_t>::visit_wstring(internal::Arg::StringValue<char16_t> value)
{
    BasicArgFormatter<UTFArgFormatter<char16_t>, char16_t>::visit_wstring(value);
}
template<>
inline void UTFArgFormatter<char16_t>::visit_wstring(internal::Arg::StringValue<char32_t> value)
{
    const auto u16str = common::str::to_u16string(value.value, value.size, common::str::Charset::UTF32);
    BasicArgFormatter<UTFArgFormatter<char16_t>, char16_t>::visit_wstring(internal::Arg::StringValue<char16_t>{ u16str.data(), u16str.size() });
}
template<>
inline void UTFArgFormatter<char32_t>::visit_string(internal::Arg::StringValue<char> value)
{
    if (value.size & internal::SizeTag) //u16string
    {
        value.size &= internal::SizeMask;
        visit_wstring(*reinterpret_cast<internal::Arg::StringValue<char16_t>*>(&value));
    }
    else //string
    {
        const auto u32str = common::str::to_u32string(value.value, value.size, common::str::Charset::UTF7);
        BasicArgFormatter<UTFArgFormatter<char32_t>, char32_t>::visit_wstring(internal::Arg::StringValue<char32_t>{ u32str.data(), u32str.size() });
    }
}
template<>
inline void UTFArgFormatter<char32_t>::visit_wstring(internal::Arg::StringValue<char32_t> value)
{
    BasicArgFormatter<UTFArgFormatter<char32_t>, char32_t>::visit_wstring(value);
}
template<>
inline void UTFArgFormatter<char32_t>::visit_wstring(internal::Arg::StringValue<char16_t> value)
{
    const auto u32str = common::str::to_u32string(value.value, value.size, common::str::Charset::UTF16LE);
    BasicArgFormatter<UTFArgFormatter<char32_t>, char32_t>::visit_wstring(internal::Arg::StringValue<char32_t>{ u32str.data(), u32str.size() });
}


template <typename Char, typename Allocator = std::allocator<Char> >
class UTFMemoryWriter : public BasicMemoryWriter<Char, Allocator> 
{
    static_assert(std::is_same_v<Char, char16_t> || std::is_same_v<Char, char32_t>, "only char16_t and char32_t allowed!");
public:
    explicit UTFMemoryWriter(const Allocator& alloc = Allocator()) : BasicMemoryWriter<Char, Allocator>(alloc) 
    { }

#if FMT_USE_RVALUE_REFERENCES
    UTFMemoryWriter(UTFMemoryWriter &&other) : BasicMemoryWriter<Char, Allocator>(std::move(other))
    { }

    UTFMemoryWriter &operator=(UTFMemoryWriter &&other) 
    {
        *(BasicMemoryWriter<Char, Allocator>*)this = std::move(other);
        return *this;
    }
#endif

    UTFMemoryWriter & operator<<(const char value) 
    { 
        static_cast<BasicWriter<Char>*>(this)->buffer().push_back(value);
        return *this;
    }
    UTFMemoryWriter & operator<<(const char* value)
    {
        return operator<<(BasicStringRef<char>(value));
    }
    UTFMemoryWriter & operator<<(const wchar_t value);
    UTFMemoryWriter & operator<<(const wchar_t* value)
    {
        return operator<<(BasicStringRef<wchar_t>(value));
    }
    UTFMemoryWriter & operator<<(const char16_t value);
    UTFMemoryWriter & operator<<(const char16_t* value)
    {
        return operator<<(BasicStringRef<char16_t>(value));
    }
    UTFMemoryWriter & operator<<(const char32_t value);
    UTFMemoryWriter & operator<<(const char32_t* value)
    {
        return operator<<(BasicStringRef<char32_t>(value));
    }


    template<typename Char2>
    UTFMemoryWriter &operator<<(fmt::BasicStringRef<Char2> value);
    template<>
    UTFMemoryWriter &operator<< <Char>(fmt::BasicStringRef<Char> value)
    {
        const Char *str = value.data();
        static_cast<BasicWriter<Char>*>(this)->buffer().append(str, str + value.size());
        return *this;
    }

    void write(BasicCStringRef<Char> format, ArgList args) 
    {
        BasicFormatter<Char, UTFArgFormatter<Char>>(args, *this).format(format);
    }
    //FMT_VARIADIC_VOID_EXT(write, BasicCStringRef<Char>)

    ///*
    template <typename... Args>
    void write(BasicCStringRef<Char> arg0, const Args& ... args)
    {
        typedef fmt::internal::ArgArray<sizeof...(Args)> ArgArray;
        typedef fmt::internal::ArgArray2<sizeof...(Args)> ArgArrayExt;
        typename ArgArray::Type array{ ArgArrayExt::make(args)... };
        write(arg0, fmt::ArgList(fmt::internal::make_type2(args...), array));
    }
    //*/
};
template class UTFMemoryWriter<char16_t>;
template class UTFMemoryWriter<char32_t>;

template<>
inline UTFMemoryWriter<char16_t>& UTFMemoryWriter<char16_t>::operator<<(const char16_t value)
{
    static_cast<BasicWriter<char16_t>*>(this)->buffer().push_back(value);
    return *this;
}
template<>
inline UTFMemoryWriter<char16_t>& UTFMemoryWriter<char16_t>::operator<<(const char32_t value)
{
    char16_t tmp[2];
    const auto cnt = common::str::detail::UTF16::To(value, 2, tmp);
    if(cnt > 0)
        static_cast<BasicWriter<char16_t>*>(this)->buffer().append(tmp, tmp + cnt);
    return *this;
}
template<>
inline UTFMemoryWriter<char16_t>& UTFMemoryWriter<char16_t>::operator<<(const wchar_t value)
{
    if constexpr(sizeof(wchar_t) == sizeof(char16_t))
        return operator<<(*reinterpret_cast<const char16_t*>(&value));
    else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
        return operator<<(*reinterpret_cast<const char32_t*>(&value));
    else
        return *this;
}

template<>
inline UTFMemoryWriter<char32_t>& UTFMemoryWriter<char32_t>::operator<<(const char32_t value)
{
    static_cast<BasicWriter<char32_t>*>(this)->buffer().push_back(value);
    return *this;
}
template<>
inline UTFMemoryWriter<char32_t>& UTFMemoryWriter<char32_t>::operator<<(const char16_t value)
{
    uint8_t cnt;
    const auto ch = common::str::detail::UTF16::From(&value, 1, cnt);
    if (cnt > 0)
        static_cast<BasicWriter<char32_t>*>(this)->buffer().push_back(ch);
    return *this;
}
template<>
inline UTFMemoryWriter<char32_t>& UTFMemoryWriter<char32_t>::operator<<(const wchar_t value)
{
    if constexpr(sizeof(wchar_t) == sizeof(char16_t))
        return operator<<(*reinterpret_cast<const char16_t*>(&value));
    else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
        return operator<<(*reinterpret_cast<const char32_t*>(&value));
    else
        return *this;
}


template<>
template<>
inline UTFMemoryWriter<char16_t>& UTFMemoryWriter<char16_t>::operator<< <char>(fmt::BasicStringRef<char> value)
{
    const auto u16str = common::str::to_u16string(value.data(), value.size(), common::str::Charset::UTF8);
    static_cast<BasicWriter<char16_t>*>(this)->buffer().append(u16str.data(), u16str.data() + u16str.size());
    return *this;
}
template<>
template<>
inline UTFMemoryWriter<char16_t>& UTFMemoryWriter<char16_t>::operator<< <char32_t>(fmt::BasicStringRef<char32_t> value)
{
    const auto u16str = common::str::to_u16string(value.data(), value.size(), common::str::Charset::UTF32);
    static_cast<BasicWriter<char16_t>*>(this)->buffer().append(u16str.data(), u16str.data() + u16str.size());
    return *this;
}
template<>
template<>
inline UTFMemoryWriter<char16_t>& UTFMemoryWriter<char16_t>::operator<< <wchar_t>(fmt::BasicStringRef<wchar_t> value)
{
    if constexpr(sizeof(wchar_t) == sizeof(char16_t))
        return operator<<(*reinterpret_cast<fmt::BasicStringRef<char16_t>*>(&value));
    else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
        return operator<<(*reinterpret_cast<fmt::BasicStringRef<char32_t>*>(&value));
    else
        return *this;
}

template<>
template<>
inline UTFMemoryWriter<char32_t>& UTFMemoryWriter<char32_t>::operator<< <char>(fmt::BasicStringRef<char> value)
{
    const auto u32str = common::str::to_u32string(value.data(), value.size(), common::str::Charset::UTF8);
    static_cast<BasicWriter<char32_t>*>(this)->buffer().append(u32str.data(), u32str.data() + u32str.size());
    return *this;
}
template<>
template<>
inline UTFMemoryWriter<char32_t>& UTFMemoryWriter<char32_t>::operator<< <char16_t>(fmt::BasicStringRef<char16_t> value)
{
    const auto u32str = common::str::to_u32string(value.data(), value.size(), common::str::Charset::UTF16LE);
    static_cast<BasicWriter<char32_t>*>(this)->buffer().append(u32str.data(), u32str.data() + u32str.size());
    return *this;
}
template<>
template<>
inline UTFMemoryWriter<char32_t>& UTFMemoryWriter<char32_t>::operator<< <wchar_t>(fmt::BasicStringRef<wchar_t> value)
{
    if constexpr(sizeof(wchar_t) == sizeof(char16_t))
        return operator<<(*reinterpret_cast<fmt::BasicStringRef<char16_t>*>(&value));
    else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
        return operator<<(*reinterpret_cast<fmt::BasicStringRef<char32_t>*>(&value));
    else
        return *this;
}


using u16CStringRef = fmt::BasicCStringRef<char16_t>;
inline std::u16string format(u16CStringRef format_str, ArgList args)
{
    UTFMemoryWriter<char16_t> writer;
    writer.write(format_str, args);
    return writer.str();
}

using u32CStringRef = fmt::BasicCStringRef<char32_t>;
inline std::u32string format(u32CStringRef format_str, ArgList args)
{
    UTFMemoryWriter<char32_t> w;
    w.write(format_str, args);
    return w.str();
}

}



#define FMT_VARIADIC_U16(ReturnType, func, ...) \
  FMT_VARIADIC_EXT_(, char16_t, ReturnType, func, return func, __VA_ARGS__)

#define FMT_VARIADIC_CONST_U16(ReturnType, func, ...) \
  FMT_VARIADIC_EXT_(const, char16_t, ReturnType, func, return func, __VA_ARGS__)

#define FMT_VARIADIC_U32(ReturnType, func, ...) \
  FMT_VARIADIC_EXT_(, char32_t, ReturnType, func, return func, __VA_ARGS__)

#define FMT_VARIADIC_CONST_U32(ReturnType, func, ...) \
  FMT_VARIADIC_EXT_(const, char32_t, ReturnType, func, return func, __VA_ARGS__)

#define FMT_CAPTURE_ARG_U16_(id, index) ::fmt::arg(u###id, id)

#define FMT_CAPTURE_ARG_U32_(id, index) ::fmt::arg(U###id, id)

#define FMT_CAPTURE_U16(...) FMT_FOR_EACH(FMT_CAPTURE_ARG_U16_, __VA_ARGS__)

#define FMT_CAPTURE_U32(...) FMT_FOR_EACH(FMT_CAPTURE_ARG_U32_, __VA_ARGS__)

namespace fmt
{

FMT_VARIADIC_U16(std::u16string, format, u16CStringRef)

FMT_VARIADIC_U32(std::u32string, format, u32CStringRef)


//
/*
template <typename... Args>
auto test0(const Args& ... args)
{
    typedef fmt::internal::ArgArray<sizeof...(Args)> ArgArray;
    typedef fmt::internal::ArgArray2<sizeof...(Args)> ArgArrayExt;
    typename ArgArray::Type array{ ArgArrayExt::make(args)... };
    return array;
}

static inline auto t0res = test0(u"0", U"1", L"2");
*/
}