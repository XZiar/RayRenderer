#pragma once

#include "SystemCommonRely.h"
#include "common/StrBase.hpp"
#include <ctime>

namespace common::str
{


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

struct FormatterBase;
struct SYSCOMMONAPI FormatterExecutor
{
    friend FormatterBase;
public:
    struct Context { };
    virtual void OnBrace(Context& context, bool isLeft);
    virtual void OnColor(Context& context, ScreenColor color);
    virtual void PutString(Context& context, std::   string_view str, const FormatSpec* spec) = 0;
    virtual void PutString(Context& context, std::u16string_view str, const FormatSpec* spec) = 0;
    virtual void PutString(Context& context, std::u32string_view str, const FormatSpec* spec) = 0;
    virtual void PutInteger(Context& context, uint32_t val, bool isSigned, const FormatSpec* spec) = 0;
    virtual void PutInteger(Context& context, uint64_t val, bool isSigned, const FormatSpec* spec) = 0;
    virtual void PutFloat  (Context& context, float  val, const FormatSpec* spec) = 0;
    virtual void PutFloat  (Context& context, double val, const FormatSpec* spec) = 0;
    virtual void PutPointer(Context& context, uintptr_t val, const FormatSpec* spec) = 0;
    virtual void PutDate   (Context& context, std::string_view fmtStr, const std::tm& date) = 0;
    forceinline void PutString(Context& context, std::wstring_view str, const FormatSpec* spec)
    {
        using Char = std::conditional_t<sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t>;
        PutString(context, std::basic_string_view<Char>{ reinterpret_cast<const Char*>(str.data()), str.size() }, spec);
    }
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    forceinline void PutString(Context& context, std::u8string_view str, const FormatSpec* spec)
    {
        PutString(context, std::string_view{ reinterpret_cast<const char*>(str.data()), str.size() }, spec);
    }
#endif
protected:
    virtual void OnFmtStr(Context& context, uint32_t offset, uint32_t length) = 0;
    virtual void OnArg(Context& context, uint8_t argIdx, bool isNamed, const FormatSpec* spec) = 0;
};


}