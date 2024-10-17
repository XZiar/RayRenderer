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
    uint16_t FmtLen     = 0;
    uint8_t TypeExtra   = 0;
    Align Alignment     : 2;
    Sign SignFlag       : 2;
    bool AlterForm      : 2;
    bool ZeroPad        : 2;
    constexpr FormatSpec() noexcept : 
        Alignment(Align::None), SignFlag(Sign::None), AlterForm(false), ZeroPad(false) {}
};


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4324)
#endif
struct OpaqueFormatSpecBase
{
    uint8_t Dummy[11] = { 0 };
};
struct OpaqueFormatSpecFill
{
    uint8_t Fill[4] = { 0 };
    uint8_t Count = 0;
};
struct alignas(4) OpaqueFormatSpecReal
{
    OpaqueFormatSpecBase Base;
    OpaqueFormatSpecFill Fill;
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
struct SpecReader;
struct SYSCOMMONAPI FormatterExecutor
{
    friend FormatterBase;
public:
    enum class StringType : uint8_t { UTF8, UTF16, UTF32 };

    struct Context { };
    virtual void OnBrace(Context& context, bool isLeft);
    virtual void OnColor(Context& context, ScreenColor color);

    virtual void PutString(Context& context, const void* str, size_t len, StringType type, const OpaqueFormatSpec& spec) = 0;
    virtual void PutInteger(Context& context, uint32_t val, bool isSigned, const OpaqueFormatSpec& spec) = 0;
    virtual void PutInteger(Context& context, uint64_t val, bool isSigned, const OpaqueFormatSpec& spec) = 0;
    virtual void PutFloat  (Context& context, float  val, const OpaqueFormatSpec& spec) = 0;
    virtual void PutFloat  (Context& context, double val, const OpaqueFormatSpec& spec) = 0;
    virtual void PutPointer(Context& context, uintptr_t val, const OpaqueFormatSpec& spec) = 0;

    virtual void PutString(Context& context, const void* str, size_t len, StringType type, const FormatSpec* spec) = 0;
    virtual void PutInteger(Context& context, uint32_t val, bool isSigned, const FormatSpec* spec) = 0;
    virtual void PutInteger(Context& context, uint64_t val, bool isSigned, const FormatSpec* spec) = 0;
    virtual void PutFloat  (Context& context, float  val, const FormatSpec* spec) = 0;
    virtual void PutFloat  (Context& context, double val, const FormatSpec* spec) = 0;
    virtual void PutPointer(Context& context, uintptr_t val, const FormatSpec* spec) = 0;
    virtual void PutDate   (Context& context, ::std::string_view fmtStr, const DateStructure& date) = 0;


    forceinline void PutString(Context& context, ::std::string_view str, const OpaqueFormatSpec& spec)
    {
        PutString(context, str.data(), str.size(), FormatterExecutor::StringType::UTF8, spec);
    }
    forceinline void PutString(Context& context, ::std::wstring_view str, const OpaqueFormatSpec& spec)
    {
        PutString(context, str.data(), str.size(), sizeof(wchar_t) == sizeof(char16_t) ? FormatterExecutor::StringType::UTF16 : FormatterExecutor::StringType::UTF32, spec);
    }
    forceinline void PutString(Context& context, ::std::u16string_view str, const OpaqueFormatSpec& spec)
    {
        PutString(context, str.data(), str.size(), FormatterExecutor::StringType::UTF16, spec);
    }
    forceinline void PutString(Context& context, ::std::u32string_view str, const OpaqueFormatSpec& spec)
    {
        PutString(context, str.data(), str.size(), FormatterExecutor::StringType::UTF32, spec);
    }
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    forceinline void PutString(Context& context, ::std::u8string_view str, const OpaqueFormatSpec& spec)
    {
        PutString(context, str.data(), str.size(), FormatterExecutor::StringType::UTF8, spec);
    }
#endif

    forceinline void PutString(Context& context, ::std::string_view str, const FormatSpec* spec)
    {
        PutString(context, str.data(), str.size(), FormatterExecutor::StringType::UTF8, spec);
    }
    forceinline void PutString(Context& context, ::std::wstring_view str, const FormatSpec* spec)
    {
        PutString(context, str.data(), str.size(), sizeof(wchar_t) == sizeof(char16_t) ? FormatterExecutor::StringType::UTF16 : FormatterExecutor::StringType::UTF32, spec);
    }
    forceinline void PutString(Context& context, ::std::u16string_view str, const FormatSpec* spec)
    {
        PutString(context, str.data(), str.size(), FormatterExecutor::StringType::UTF16, spec);
    }
    forceinline void PutString(Context& context, ::std::u32string_view str, const FormatSpec* spec)
    {
        PutString(context, str.data(), str.size(), FormatterExecutor::StringType::UTF32, spec);
    }
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    forceinline void PutString(Context& context, ::std::u8string_view str, const FormatSpec* spec)
    {
        PutString(context, str.data(), str.size(), FormatterExecutor::StringType::UTF8, spec);
    }
#endif

    // context irrelevant
    static bool ConvertSpec(OpaqueFormatSpec& dst, const FormatSpec* src, ArgRealType real, ArgDispType disp) noexcept;
    static bool ConvertSpec(OpaqueFormatSpec& dst, std::u32string_view spectxt, ArgRealType real) noexcept;
protected:
    virtual void OnFmtStr(Context& context, uint32_t offset, uint32_t length) = 0;
    virtual void OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader) = 0;
};


template<typename T>
inline auto FormatAs(const T& arg);

template<typename T>
inline auto FormatWith(const T& arg, FormatterExecutor& executor, FormatterExecutor::Context& context, const FormatSpec* spec);

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

template<typename T, typename U = std::decay_t<T>, typename = std::enable_if_t<std::is_integral_v<U> || std::is_floating_point_v<U>>>
inline void FormatWith(const span<T>& arg, FormatterExecutor& executor, FormatterExecutor::Context& context, const FormatSpec* spec)
{
    executor.PutString(context, "[", nullptr);
    static_assert(sizeof(U) <= sizeof(uint64_t));
    for (size_t i = 0; i < arg.size(); ++i)
    {
        if (i > 0)
        {
            executor.PutString(context, ", ", nullptr);
        }
        if constexpr (std::is_floating_point_v<U>)
        {
            if constexpr (std::is_same_v<U, double>)
            {
                executor.PutFloat(context, arg[i], spec);
            }
            else
            {
                executor.PutFloat(context, static_cast<float>(arg[i]), spec);
            }
        }
        else
        {
            if constexpr (sizeof(U) == sizeof(uint64_t))
            {
                executor.PutInteger(context, arg[i], !std::is_unsigned_v<U>, spec);
            }
            else
            {
                executor.PutInteger(context, static_cast<uint32_t>(arg[i]), !std::is_unsigned_v<U>, spec);
            }
        }
    }
    executor.PutString(context, "]", nullptr);
}


}