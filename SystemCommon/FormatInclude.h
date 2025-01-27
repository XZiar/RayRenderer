#pragma once

#include "SystemCommonRely.h"
#include "common/StrBase.hpp"
#include <ctime>

static_assert(common::detail::is_little_endian, "Format only support Little Endian");

namespace common::str
{


enum class ArgDispType : uint8_t
{
    Any = 0, String, Char, Integer, Float, Pointer, Date, Time, Color, Numeric, Custom,
};
//- Any
//    - String
//    - Date
//    - Time
//    - Color
//    - Custom
//    - Numeric
//        - Integer
//            - Char
//                - Bool
//            - Pointer
//        - Float
MAKE_ENUM_BITFIELD(ArgDispType)


enum class ArgRealType : uint8_t
{
    Error = 0x0, SInt = 0x1, UInt = 0x2, Char = 0x3, Float = 0x4, String = 0x5, Bool = 0x6, Ptr = 0x7, 
    Date = 0x8, Color = 0x9, Custom = 0xf,
    SpanBit = 0x80, EmptyBit = 0x00, StrPtrBit = 0x40, PtrVoidBit = 0x10, DateDeltaBit = 0x10, DateZoneBit = 0x20,
    BaseTypeMask = 0x0f, TypeSizeMask = 0x70,
};
MAKE_ENUM_BITFIELD(ArgRealType)


struct FormatSpec
{
    enum class Align : uint8_t { None, Left, Right, Middle };
    enum class Sign  : uint8_t { None, Neg, Pos, Space };
    uint32_t Fill       = ' ';
    uint32_t Precision  = 0;
    uint16_t Width      = 0;
    uint16_t FmtOffset  = 0;
    uint8_t FmtLen      = 0;
    uint8_t TypeExtra   = 0;
    Align Alignment     : 2;
    Sign SignFlag       : 2;
    bool AlterForm      : 1;
    bool ZeroPad        : 1;
    forceinline constexpr FormatSpec() noexcept : 
        Alignment(Align::None), SignFlag(Sign::None), AlterForm(false), ZeroPad(false) {}
};


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4324)
#endif
struct alignas(4) OpaqueFormatSpecReal
{
    uint8_t Data[4] = { 0 };
    uint8_t Fill[4] = { 0 };
    uint8_t Extra[8] = { 0 };
};
struct alignas(4) OpaqueFormatSpec
{
    OpaqueFormatSpecReal Real;
    char32_t Fill32[1] = { 0 };
    char16_t Fill16[2] = { 0 };
};
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


namespace detail
{
template<typename T>
using HasGMTOffCheck = decltype(T::tm_gmtoff);
template<typename T>
using HasZoneCheck = decltype(T::tm_zone);
template<typename T>
inline constexpr bool has_gmtoff_v = is_detected_v<HasGMTOffCheck, T>;
template<typename T>
inline constexpr bool has_zone_v = is_detected_v<HasZoneCheck, T>;
inline constexpr bool tm_has_zone_info = has_gmtoff_v<std::tm> || has_gmtoff_v<std::tm>;
inline constexpr bool tm_has_all_zone_info = has_gmtoff_v<std::tm> && has_gmtoff_v<std::tm>;
}

struct DateStructure
{
    std::string Zone;
    std::tm Base;
    uint32_t MicroSeconds = UINT32_MAX;
    int32_t GMTOffset = 0;
};

struct FormatterBase;
struct FormatterContext {};
struct SYSCOMMONAPI FormatterOpExecutor
{
    using Context = FormatterContext;
    virtual ~FormatterOpExecutor() = 0;
    virtual void OnBrace(Context& context, bool isLeft) = 0;
    virtual void OnColor(Context& context, ScreenColor color) = 0;
    virtual void OnFmtStr(Context& context, uint32_t offset, uint32_t length) = 0;
    virtual void OnArg(Context& context, uint8_t argIdx, bool isNamed, const uint8_t* spec) = 0;
protected:
    uint32_t Execute(span<const uint8_t>& opcodes, Context& context, uint32_t instCount = UINT32_MAX);
};

enum class StringType : uint8_t { UTF8, UTF16, UTF32 };
template<typename T, typename Context>
struct StringFormatter
{
    template<typename Spec>
    forceinline void PutString(Context& context, ::std::string_view str, const Spec& spec)
    {
        static_cast<T*>(this)->PutString(context, str.data(), str.size(), StringType::UTF8, spec);
    }
    template<typename Spec>
    forceinline void PutString(Context& context, ::std::wstring_view str, const Spec& spec)
    {
        static_cast<T*>(this)->PutString(context, str.data(), str.size(), sizeof(wchar_t) == sizeof(char16_t) ? StringType::UTF16 : StringType::UTF32, spec);
    }
    template<typename Spec>
    forceinline void PutString(Context& context, ::std::u16string_view str, const Spec& spec)
    {
        static_cast<T*>(this)->PutString(context, str.data(), str.size(), StringType::UTF16, spec);
    }
    template<typename Spec>
    forceinline void PutString(Context& context, ::std::u32string_view str, const Spec& spec)
    {
        static_cast<T*>(this)->PutString(context, str.data(), str.size(), StringType::UTF32, spec);
    }
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    template<typename Spec>
    forceinline void PutString(Context& context, ::std::u8string_view str, const Spec& spec)
    {
        static_cast<T*>(this)->PutString(context, str.data(), str.size(), StringType::UTF8, spec);
    }
#endif
};
template<typename T, typename Context>
struct DateFormatter
{
    forceinline void PutDate(Context& context, ::std::string_view str, const DateStructure& date)
    {
        static_cast<T*>(this)->PutDate(context, str.data(), str.size(), StringType::UTF8, date);
    }
    forceinline void PutDate(Context& context, ::std::wstring_view str, const DateStructure& date)
    {
        static_cast<T*>(this)->PutDate(context, str.data(), str.size(), sizeof(wchar_t) == sizeof(char16_t) ? StringType::UTF16 : StringType::UTF32, date);
    }
    forceinline void PutDate(Context& context, ::std::u16string_view str, const DateStructure& date)
    {
        static_cast<T*>(this)->PutDate(context, str.data(), str.size(), StringType::UTF16, date);
    }
    forceinline void PutDate(Context& context, ::std::u32string_view str, const DateStructure& date)
    {
        static_cast<T*>(this)->PutDate(context, str.data(), str.size(), StringType::UTF32, date);
    }
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    forceinline void PutDate(Context& context, ::std::u8string_view str, const DateStructure& date)
    {
        static_cast<T*>(this)->PutDate(context, str.data(), str.size(), StringType::UTF8, date);
    }
#endif
};

struct SYSCOMMONAPI FormatterHost : public StringFormatter<FormatterHost, FormatterContext>, public DateFormatter<FormatterHost, FormatterContext>
{
public:
    using Context = FormatterContext;
    virtual ~FormatterHost() = 0;

    virtual void PutString(Context& context, const void* str, size_t len, StringType type, const OpaqueFormatSpec& spec) = 0;
    virtual void PutInteger(Context& context, uint32_t val, bool isSigned, const OpaqueFormatSpec& spec) = 0;
    virtual void PutInteger(Context& context, uint64_t val, bool isSigned, const OpaqueFormatSpec& spec) = 0;
    virtual void PutFloat(Context& context, float  val, const OpaqueFormatSpec& spec) = 0;
    virtual void PutFloat(Context& context, double val, const OpaqueFormatSpec& spec) = 0;
    virtual void PutPointer(Context& context, uintptr_t val, const OpaqueFormatSpec& spec) = 0;

    virtual void PutString(Context& context, const void* str, size_t len, StringType type, const FormatSpec* spec) = 0;
    virtual void PutInteger(Context& context, uint32_t val, bool isSigned, const FormatSpec* spec) = 0;
    virtual void PutInteger(Context& context, uint64_t val, bool isSigned, const FormatSpec* spec) = 0;
    virtual void PutFloat(Context& context, float  val, const FormatSpec* spec) = 0;
    virtual void PutFloat(Context& context, double val, const FormatSpec* spec) = 0;
    virtual void PutPointer(Context& context, uintptr_t val, const FormatSpec* spec) = 0;

    virtual void PutColor(Context& context, ScreenColor color);
    virtual void PutDate(Context& context, const void* fmtStr, size_t len, StringType type, const DateStructure& date);

    using StringFormatter<FormatterHost, FormatterContext>::PutString;
    using DateFormatter  <FormatterHost, FormatterContext>::PutDate;

    // context irrelevant
    static bool ConvertSpec(OpaqueFormatSpec& dst, const FormatSpec* src, ArgRealType real, ArgDispType disp) noexcept;
    static bool ConvertSpec(OpaqueFormatSpec& dst, std::u32string_view spectxt, ArgRealType real) noexcept;
};


template<typename T>
inline auto FormatAs(const T& arg);

template<typename T>
inline auto FormatWith(const T& arg, FormatterHost& executor, FormatterContext& context, const FormatSpec* spec);

template<typename Char>
inline auto FormatAs(const StrVariant<Char>& arg)
{
    return arg.View;
}
template<typename Char>
inline auto FormatAs(const HashedStrView<Char>& arg)
{
    return arg.View;
}

template<typename T> concept BasicFormatable = std::is_integral_v<std::decay_t<T>> || std::is_floating_point_v<std::decay_t<T>> ||
common::is_specialization<std::decay_t<T>, std::basic_string>::value || common::is_specialization<std::decay_t<T>, std::basic_string_view>::value;
template<BasicFormatable T>
inline void FormatWith(const span<T>& arg, FormatterHost& host, FormatterContext& context, const FormatSpec* spec)
{
    using U = std::decay_t<T>;
    host.PutString(context, "[", nullptr);
    for (size_t i = 0; i < arg.size(); ++i)
    {
        if (i > 0)
        {
            host.PutString(context, ", ", nullptr);
        }
        if constexpr (std::is_floating_point_v<U>)
        {
            static_assert(sizeof(U) <= sizeof(double));
            if constexpr (sizeof(U) == sizeof(double))
            {
                host.PutFloat(context, static_cast<double>(arg[i]), spec);
            }
            else
            {
                host.PutFloat(context, static_cast<float>(arg[i]), spec);
            }
        }
        else if constexpr (std::is_integral_v<U>)
        {
            static_assert(sizeof(U) <= sizeof(uint64_t));
            if constexpr (sizeof(U) == sizeof(uint64_t))
            {
                host.PutInteger(context, static_cast<uint64_t>(arg[i]), !std::is_unsigned_v<U>, spec);
            }
            else
            {
                host.PutInteger(context, static_cast<uint32_t>(arg[i]), !std::is_unsigned_v<U>, spec);
            }
        }
        else
            host.PutString(context, arg[i], spec);
    }
    host.PutString(context, "]", nullptr);
}


}