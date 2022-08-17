#pragma once

#include "SystemCommonRely.h"
#include "common/StrBase.hpp"
#include <ctime>

namespace common::str
{


enum class ArgDispType : uint8_t
{
    Any = 0, String, Char, Integer, Float, Pointer, Date, Time, Numeric, Custom,
};
//- Any
//    - String
//    - Date
//    - Time
//    - Custom
//    - Numeric
//        - Integer
//            - Char
//            - Pointer
//        - Float
MAKE_ENUM_BITFIELD(ArgDispType)


enum class ArgRealType : uint8_t
{
    BaseTypeMask = 0xf0, SizeMask8 = 0b111, SizeMask4 = 0b11, SpanBit = 0b1000,
    Error = 0x00, Custom = 0x01, Ptr = 0x02, PtrVoid = 0x03, Date = 0x04, DateDelta = 0x05, Bool = 0x06,
    TypeSpecial = 0x00, SpecialMax = Bool,
    SInt    = 0x10,
    UInt    = 0x20,
    Float   = 0x30,
    Char    = 0x40,
    String  = 0x50, StrPtrBit = 0b100,
    Empty   = 0x0,
};
MAKE_ENUM_BITFIELD(ArgRealType)
MAKE_ENUM_RANGE(ArgRealType)


struct FormatSpec
{
    enum class Align : uint8_t { None, Left, Right, Middle };
    enum class Sign  : uint8_t { None, Pos, Neg, Space };
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


struct OpaqueFormatSpecBase
{
    uint8_t Dummy[11] = { 0 };
};
struct OpaqueFormatSpec
{
    char32_t Fill32[1] = { 0 };
    char16_t Fill16[2] = { 0 };
    char     Fill8 [4] = { 0 };
    uint8_t Count32 : 2;
    uint8_t Count16 : 2;
    uint8_t Count8  : 2;
    OpaqueFormatSpecBase Base;
    constexpr OpaqueFormatSpec() noexcept : Count32(0), Count16(0), Count8(0) {}
};


struct FormatterBase;
struct SYSCOMMONAPI FormatterExecutor
{
    friend FormatterBase;
public:
    struct Context { };
    virtual void OnBrace(Context& context, bool isLeft);
    virtual void OnColor(Context& context, ScreenColor color);

    virtual void PutString(Context& context, ::std::   string_view str, const OpaqueFormatSpec& spec) = 0;
    virtual void PutString(Context& context, ::std::u16string_view str, const OpaqueFormatSpec& spec) = 0;
    virtual void PutString(Context& context, ::std::u32string_view str, const OpaqueFormatSpec& spec) = 0;
    virtual void PutInteger(Context& context, uint32_t val, bool isSigned, const OpaqueFormatSpec& spec) = 0;
    virtual void PutInteger(Context& context, uint64_t val, bool isSigned, const OpaqueFormatSpec& spec) = 0;
    virtual void PutFloat  (Context& context, float  val, const OpaqueFormatSpec& spec) = 0;
    virtual void PutFloat  (Context& context, double val, const OpaqueFormatSpec& spec) = 0;
    virtual void PutPointer(Context& context, uintptr_t val, const OpaqueFormatSpec& spec) = 0;

    virtual void PutString(Context& context, ::std::   string_view str, const FormatSpec* spec) = 0;
    virtual void PutString(Context& context, ::std::u16string_view str, const FormatSpec* spec) = 0;
    virtual void PutString(Context& context, ::std::u32string_view str, const FormatSpec* spec) = 0;
    virtual void PutInteger(Context& context, uint32_t val, bool isSigned, const FormatSpec* spec) = 0;
    virtual void PutInteger(Context& context, uint64_t val, bool isSigned, const FormatSpec* spec) = 0;
    virtual void PutFloat  (Context& context, float  val, const FormatSpec* spec) = 0;
    virtual void PutFloat  (Context& context, double val, const FormatSpec* spec) = 0;
    virtual void PutPointer(Context& context, uintptr_t val, const FormatSpec* spec) = 0;
    virtual void PutDate   (Context& context, ::std::string_view fmtStr, const ::std::tm& date) = 0;

    forceinline void PutString(Context& context, ::std::wstring_view str, const OpaqueFormatSpec& spec)
    {
        using Char = ::std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t>;
        PutString(context, ::std::basic_string_view<Char>{ reinterpret_cast<const Char*>(str.data()), str.size() }, spec);
    }
    forceinline void PutString(Context& context, ::std::wstring_view str, const FormatSpec* spec)
    {
        using Char = ::std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t>;
        PutString(context, ::std::basic_string_view<Char>{ reinterpret_cast<const Char*>(str.data()), str.size() }, spec);
    }
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    forceinline void PutString(Context& context, ::std::u8string_view str, const OpaqueFormatSpec& spec)
    {
        PutString(context, ::std::string_view{ reinterpret_cast<const char*>(str.data()), str.size() }, spec);
    }
    forceinline void PutString(Context& context, ::std::u8string_view str, const FormatSpec* spec)
    {
        PutString(context, ::std::string_view{ reinterpret_cast<const char*>(str.data()), str.size() }, spec);
    }
#endif
    // context irrelevant
    static bool ConvertSpec(OpaqueFormatSpec& dst, const FormatSpec* src, ArgRealType real, ArgDispType disp) noexcept;
    static bool ConvertSpec(OpaqueFormatSpec& dst, std::u32string_view spectxt, ArgRealType real) noexcept;
protected:
    virtual void OnFmtStr(Context& context, uint32_t offset, uint32_t length) = 0;
    virtual void OnArg(Context& context, uint8_t argIdx, bool isNamed, const FormatSpec* spec) = 0;
};


}