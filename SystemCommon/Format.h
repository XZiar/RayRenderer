#pragma once

#include "SystemCommonRely.h"
#include "FormatInclude.h"
#include "common/StrBase.hpp"
#include "common/RefHolder.hpp"
#include "common/SharedString.hpp"
#include <optional>
#include <chrono>
#include <ctime>

#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wassume"
#endif
#if COMMON_COMPILER_GCC & COMMON_GCC_VER < 100000
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wattributes"
#endif
namespace common::str
{


template<typename Char>
struct StrArgInfoCh;

struct ParseResultBase
{
    uint16_t ErrorPos = UINT16_MAX, ErrorNum = 0;
    enum class ErrorCode : uint16_t
    {
        Error = 0xff00, FmtTooLong, TooManyIdxArg, TooManyNamedArg, TooManyOp,
        MissingLeftBrace, MissingRightBrace, MissingFmtSpec,
        InvalidColor, MissingColorFGBG, InvalidCommonColor, Invalid8BitColor, Invalid24BitColor,
        InvalidArgIdx, ArgIdxTooLarge, InvalidArgName, ArgNameTooLong,
        WidthTooLarge, InvalidPrecision, PrecisionTooLarge,
        InvalidType, ExtraFmtSpec, IncompDateSpec, IncompTimeSpec, IncompColorSpec, IncompNumSpec, IncompType, InvalidFmt
    };
#define SCSTR_HANDLE_PARSE_ERROR(handler)\
    handler(FmtTooLong,         "Format string too long"); \
    handler(TooManyIdxArg,      "Too many IdxArg"); \
    handler(TooManyNamedArg,    "Too many NamedArg"); \
    handler(TooManyOp,          "Format string too complex with too many generated OP"); \
    handler(MissingLeftBrace,   "Missing left brace"); \
    handler(MissingRightBrace,  "Missing right brace"); \
    handler(MissingFmtSpec,     "Missing format spec"); \
    handler(InvalidColor,       "Invalid color format"); \
    handler(MissingColorFGBG,   "Missing color foreground/background identifier"); \
    handler(InvalidCommonColor, "Invalid common color"); \
    handler(Invalid8BitColor,   "Invalid 8bit color"); \
    handler(Invalid24BitColor,  "Invalid 24bit color"); \
    handler(InvalidArgIdx,      "Invalid index for IdxArg"); \
    handler(ArgIdxTooLarge,     "Arg index is too large"); \
    handler(InvalidArgName,     "Invalid name for NamedArg"); \
    handler(ArgNameTooLong,     "Arg name is too long"); \
    handler(WidthTooLarge,      "Width is too large"); \
    handler(InvalidPrecision,   "Invalid format for precision"); \
    handler(PrecisionTooLarge,  "Precision is too large"); \
    handler(InvalidType,        "Invalid type specified for arg"); \
    handler(ExtraFmtSpec,       "Unknown extra format spec left at the end"); \
    handler(IncompDateSpec,     "Extra spec applied on date type"); \
    handler(IncompTimeSpec,     "Extra spec applied on time type"); \
    handler(IncompColorSpec,    "Extra spec applied on color type"); \
    handler(IncompNumSpec,      "Numeric spec applied on non-numeric type"); \
    handler(IncompType,         "In-compatible type being specified for the arg"); \
    handler(InvalidFmt,         "Invalid format string");

    template<uint16_t ErrorPos, uint16_t ErrorNum>
    static constexpr bool CheckErrorCompile() noexcept
    {
        if constexpr (ErrorPos != UINT16_MAX)
        {
            constexpr auto err = static_cast<ErrorCode>(ErrorNum);
#define CHECK_ERROR_MSG(e, msg) static_assert(err != ErrorCode::e, msg)
            SCSTR_HANDLE_PARSE_ERROR(CHECK_ERROR_MSG)
#undef CHECK_ERROR_MSG
                static_assert(!AlwaysTrue2<ErrorNum>, "Unknown internal error");
            return false;
        }
        return true;
    }
    SYSCOMMONAPI static void CheckErrorRuntime(uint16_t errorPos, uint16_t errorNum);

    constexpr void SetError(size_t pos, ErrorCode err) noexcept
    {
        ErrorPos = static_cast<uint16_t>(pos);
        ErrorNum = enum_cast(err);
    }

    static constexpr std::optional<ArgDispType> CheckCompatible(ArgDispType prev, ArgDispType cur) noexcept
    {
        if (prev == cur) return prev;
        if (cur == ArgDispType::Any) return prev;
        if (prev == ArgDispType::Any) return cur;
        if (prev == ArgDispType::Integer) // can cast to detail type
        {
            if (cur == ArgDispType::Char || cur == ArgDispType::Pointer) return cur;
        }
        else if (prev == ArgDispType::Numeric)
        {
            if (cur == ArgDispType::Char || cur == ArgDispType::Pointer || cur == ArgDispType::Integer || cur == ArgDispType::Float) return cur;
        }
        return {}; // incompatible
    }
};

struct ParseResultCommon : public ParseResultBase
{
    static constexpr uint32_t IdxArgSlots = 64;
    static constexpr uint32_t NamedArgSlots = 16;
    struct NamedArgType
    {
        uint16_t Offset = 0;
        uint8_t Length = 0;
        ArgDispType Type = ArgDispType::Any;
    };
    NamedArgType NamedTypes[NamedArgSlots] = {};
    ArgDispType IndexTypes[IdxArgSlots] = { ArgDispType::Any };
    uint8_t NamedArgCount = 0, IdxArgCount = 0, NextArgIdx = 0;

};

template<uint16_t Size = 1024>
struct ParseResult : public ParseResultCommon
{
    static constexpr uint32_t OpSlots = Size;
    uint8_t Opcodes[OpSlots] = { 0 };
    uint16_t OpCount = 0;
    forceinline constexpr uint8_t* ReserveSpace(size_t offset, uint32_t count) noexcept
    {
        IF_UNLIKELY(OpCount + count > OpSlots)
        {
            SetError(offset, ParseResultBase::ErrorCode::TooManyOp);
            return nullptr;
        }
        auto ptr = Opcodes + OpCount;
        OpCount = static_cast<uint16_t>(OpCount + count);
        return ptr;
    }

    template<typename Char>
    constexpr StrArgInfoCh<Char> ToInfo(const std::basic_string_view<Char> str) const noexcept;
};


struct FormatterParser
{
    static constexpr uint8_t OpIdMask = 0b11000000;
    static constexpr uint8_t OpFieldMask = 0b00110000;
    static constexpr uint8_t OpDataMask = 0b00001111;

    struct FormatSpec
    {
        using Align = str::FormatSpec::Align;
        using Sign  = str::FormatSpec::Sign;
        struct TypeIdentifier
        {
            ArgDispType Type  = ArgDispType::Any;
            uint8_t Extra = 0;
            constexpr TypeIdentifier() noexcept {}
            forceinline constexpr TypeIdentifier(char ch) noexcept
            {
                static_assert(std::string_view::npos == SIZE_MAX);
                // type        ::=  "a" | "A" | "b" | "B" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "o" | "p" | "s" | "x" | "X" | "T" | "@"
                constexpr std::string_view types{ "gGaAeEfFdbBoxXcpsT@" };
                const auto typeidx = static_cast<uint32_t>(types.find_first_of(ch)); // hack on x86 to avoid using r64
                if (typeidx <= 7) // gGaAeEfF
                {
                    Type = ArgDispType::Float;
                    Extra = static_cast<uint8_t>(typeidx);
                }
                else if (typeidx <= 13) // dbBoxX
                {
                    Type = ArgDispType::Integer;
                    Extra = static_cast<uint8_t>(typeidx - 8);
                }
                else if (typeidx == 14) // c
                {
                    Type = ArgDispType::Char;
                    Extra = 0x00;
                }
                else if (typeidx == 15) // p
                {
                    Type = ArgDispType::Pointer;
                    Extra = 0x00;
                }
                else if (typeidx == 16) // s
                {
                    Type = ArgDispType::String;
                    Extra = 0x00;
                }
                else if (typeidx == 17) // T
                {
                    Type = ArgDispType::Date;
                    Extra = 0x00;
                }
                else if (typeidx == 18) // T
                {
                    Type = ArgDispType::Color;
                    Extra = 0x00;
                }
                ELSE_UNLIKELY
                {
                    Extra = 0xff;
                }
            }
        };
        uint32_t Fill       = ' ';
        uint32_t Precision  = 0;
        uint16_t Width      = 0;
        uint16_t FmtOffset  = 0;
        uint16_t FmtLen     = 0;
        TypeIdentifier Type;
        Align Alignment     = Align::None;
        Sign SignFlag       = Sign::None;
        bool AlterForm      = false;
        bool ZeroPad        = false;
    };
    
    struct BuiltinOp
    {
        static constexpr uint8_t Length[2] = { 1,5 };
        static constexpr uint8_t Op = 0x00;
        static constexpr uint8_t FieldFmtStr = 0x00;
        static constexpr uint8_t FieldBrace  = 0x10;
        static constexpr uint8_t DataOffset16 = 0x01;
        static constexpr uint8_t DataLength16 = 0x02;
        template<typename T>
        static forceinline constexpr bool EmitFmtStr(T& result, size_t offset, size_t length) noexcept
        {
            const auto isOffset16 = offset >= UINT8_MAX;
            const auto isLength16 = length >= UINT8_MAX;
            const auto opcnt = 1 + 1 + 1 + (isOffset16 ? 1 : 0) + (isLength16 ? 1 : 0);
            auto space = result.ReserveSpace(offset, opcnt);
            IF_UNLIKELY(!space) return false;
            *space++ = Op | FieldFmtStr;
            *space++ = static_cast<uint8_t>(offset);
            if (isOffset16)
                *space++ = static_cast<uint8_t>(offset >> 8);
            *space++ = static_cast<uint8_t>(length);
            if (isLength16)
                *space++ = static_cast<uint8_t>(length >> 8);
            return true;
        }
        template<typename T>
        static forceinline constexpr bool EmitBrace(T& result, size_t offset, bool isLeft) noexcept
        {
            const auto space = result.ReserveSpace(offset, 1);
            IF_UNLIKELY(!space) return false;
            *space = Op | FieldBrace | uint8_t(isLeft ? 0x0 : 0x1);
            return true;
        }
    };
    struct ColorOp
    {
        static constexpr uint8_t Length[2] = { 1,4 };
        static constexpr uint8_t Op = 0x40;
        static constexpr uint8_t FieldCommon     = 0x00;
        static constexpr uint8_t FieldSpecial    = 0x20;
        static constexpr uint8_t FieldForeground = 0x00;
        static constexpr uint8_t FieldBackground = 0x10;
        static constexpr uint8_t DataDefault     = 0x0;
        static constexpr uint8_t DataBit8        = 0x1;
        static constexpr uint8_t DataBit24       = 0x2;
        template<typename T>
        static forceinline constexpr bool Emit(T& result, size_t offset, CommonColor color, bool isForeground) noexcept
        {
            const auto space = result.ReserveSpace(offset, 1);
            IF_UNLIKELY(!space) return false;
            *space = Op | FieldCommon | (isForeground ? FieldForeground : FieldBackground) | enum_cast(color);
            return true;
        }
        template<typename T>
        static forceinline constexpr bool EmitDefault(T& result, size_t offset, bool isForeground) noexcept
        {
            const auto space = result.ReserveSpace(offset, 1);
            IF_UNLIKELY(!space) return false;
            *space = Op | FieldSpecial | (isForeground ? FieldForeground : FieldBackground) | DataDefault;
            return true;
        }
        template<typename T>
        static forceinline constexpr bool Emit(T& result, size_t offset, uint8_t color, bool isForeground) noexcept
        {
            if (color < 16) // use common color
                return Emit(result, offset, static_cast<CommonColor>(color), isForeground);
            auto space = result.ReserveSpace(offset, 2);
            IF_UNLIKELY(!space) return false;
            *space++ = Op | FieldSpecial | (isForeground ? FieldForeground : FieldBackground) | DataBit8;
            *space++ = color;
            return true;
        }
        template<typename T>
        static forceinline constexpr bool Emit(T& result, size_t offset, uint8_t red, uint8_t green, uint8_t blue, bool isForeground) noexcept
        {
            auto space = result.ReserveSpace(offset, 4);
            IF_UNLIKELY(!space) return false;
            *space++ = Op | FieldSpecial | (isForeground ? FieldForeground : FieldBackground) | DataBit24;
            *space++ = red;
            *space++ = green;
            *space++ = blue;
            return true;
        }
    };
    struct ArgOp
    {
        static constexpr uint8_t SpecLength[2] = { 2,16 };
        static constexpr uint8_t Length[2] = { 2,18 };
        static constexpr uint8_t Op = 0x80;
        static constexpr uint8_t FieldIndexed = 0x00;
        static constexpr uint8_t FieldNamed   = 0x20;
        static constexpr uint8_t FieldHasSpec = 0x10;
        static forceinline constexpr uint8_t EncodeValLen(uint32_t val) noexcept
        {
            if (val <= UINT8_MAX)  return (uint8_t)1;
            if (val <= UINT16_MAX) return (uint8_t)2;
            return (uint8_t)3;
        }
        static forceinline constexpr uint32_t EncodeSpec(const FormatSpec& spec, uint8_t(&output)[SpecLength[1]]) noexcept
        {
            uint8_t spec0 = 0, spec1 = 0;
            /*struct FormatSpec
            {
                uint32_t Fill;//2
                enum class Align : uint8_t { None, Left, Right, Middle };
                enum class Sign : uint8_t { None, Pos, Neg, Space };
                uint32_t Precision = 0;//2
                uint16_t Width = 0;//2
                uint16_t FmtOffset = 0, FmtLen = 0;//1
                char Type = '\0';//3
                Align Alignment = Align::None;//2
                Sign SignFlag = Sign::None;//2
                bool AlterForm = false;//1
                bool ZeroPad = false;//1
            };*/
            spec0 |= static_cast<uint8_t>(spec.Type.Extra << 5);
            spec0 |= static_cast<uint8_t>((enum_cast(spec.Alignment) & 0x3) << 2);
            spec0 |= static_cast<uint8_t>((enum_cast(spec.SignFlag)  & 0x3) << 0);
            uint32_t idx = 2;
            IF_UNLIKELY(spec.Fill != ' ') // + 0~4
            {
                const auto val = EncodeValLen(spec.Fill);
                output[idx++] = static_cast<uint8_t>(spec.Fill);
                if (val > 1)
                {
                    output[idx++] = static_cast<uint8_t>(spec.Fill >> 8);
                }
                if (val > 2)
                {
                    output[idx++] = static_cast<uint8_t>(spec.Fill >> 16);
                    output[idx++] = static_cast<uint8_t>(spec.Fill >> 24);
                }
                spec1 |= static_cast<uint8_t>(val << 6);
            }
            IF_UNLIKELY(spec.Precision != 0) // + 0~4
            {
                const auto val = EncodeValLen(spec.Precision);
                output[idx++] = static_cast<uint8_t>(spec.Precision);
                if (val > 1)
                {
                    output[idx++] = static_cast<uint8_t>(spec.Precision >> 8);
                }
                if (val > 2)
                {
                    output[idx++] = static_cast<uint8_t>(spec.Precision >> 16);
                    output[idx++] = static_cast<uint8_t>(spec.Precision >> 24);
                }
                spec1 |= static_cast<uint8_t>(val << 4);
            }
            IF_UNLIKELY(spec.Width != 0) // + 0~2
            {
                const auto val = EncodeValLen(spec.Width);
                output[idx++] = static_cast<uint8_t>(spec.Width);
                if (val > 1)
                {
                    output[idx++] = static_cast<uint8_t>(spec.Width >> 8);
                }
                spec1 |= static_cast<uint8_t>(val << 2);
            }
            IF_UNLIKELY(spec.FmtLen > 0) // + 0~4
            {
                spec0 |= static_cast<uint8_t>(0x10u);
                output[idx++] = static_cast<uint8_t>(spec.FmtOffset);
                if (spec.FmtOffset > UINT8_MAX)
                {
                    output[idx++] = static_cast<uint8_t>(spec.FmtOffset >> 8);
                    spec0 |= static_cast<uint8_t>(0x80u);
                }
                output[idx++] = static_cast<uint8_t>(spec.FmtLen);
                if (spec.FmtLen > UINT8_MAX)
                {
                    output[idx++] = static_cast<uint8_t>(spec.FmtLen >> 8);
                    spec0 |= static_cast<uint8_t>(0x40u);
                }
            }
            if (spec.AlterForm) 
                spec1 |= 0b10;
            if (spec.ZeroPad)
                spec1 |= 0b01;
            output[0] = spec0;
            output[1] = spec1;
            return idx;
        }
        template<typename T>
        static forceinline constexpr bool EmitDefault(T& result, size_t offset, const uint8_t argIdx) noexcept
        {
            auto space = result.ReserveSpace(offset, 2);
            IF_UNLIKELY(!space) return false;
            // no spec, no need to modify argType
            // auto& argType = result.Types[result.ArgIdx];
            *space++ = Op | FieldIndexed;
            *space++ = argIdx;
            return true;
        }
        template<typename T>
        static forceinline constexpr bool Emit(T* __restrict result, size_t offset, const FormatSpec* __restrict spec, ArgDispType* __restrict dstType, const uint8_t argIdx, bool isNamed) noexcept
        {
            uint8_t opcode = Op | (isNamed ? FieldNamed : FieldIndexed);
            
            if (spec) // need to check type
            {
                const auto compType = ParseResultBase::CheckCompatible(*dstType, spec->Type.Type);
                IF_UNLIKELY(!compType)
                {
                    result->SetError(offset, ParseResultBase::ErrorCode::IncompType);
                    return false;
                }
                *dstType = *compType; // update argType
                opcode |= FieldHasSpec;
            }
            // emit op
            {
                auto space = result->ReserveSpace(offset, 2);
                IF_UNLIKELY(!space) return false;
                *space++ = opcode;
                *space++ = argIdx;
            }
            if (spec)
            {
                uint8_t output[SpecLength[1]] = { 0 };
                const auto tailopcnt = EncodeSpec(*spec, output);
                CM_ASSUME(tailopcnt <= SpecLength[1]);
                auto space = result->ReserveSpace(offset, tailopcnt);
                IF_UNLIKELY(!space) return false;
                for (uint32_t i = 0; i < tailopcnt; ++i)
                    space[i] = output[i];
            }
            return true;
        }
    };

    static forceinline constexpr std::optional<uint8_t> ParseHex8bit(uint32_t hex0, uint32_t hex1) noexcept
    {
        uint32_t hex[2] = { hex0, hex1 };
        uint32_t ret = 0;
        for (uint32_t i = 0; i < 2; ++i)
        {
                 if (hex[i] >= '0' && hex[i] <= '9') ret = ret * 16 + (hex[i] - '0');
            else if (hex[i] >= 'a' && hex[i] <= 'f') ret = ret * 16 + (hex[i] - 'a' + 10);
            else if (hex[i] >= 'A' && hex[i] <= 'F') ret = ret * 16 + (hex[i] - 'A' + 10);
            else return {};
        }
        return static_cast<uint8_t>(ret);
    }

    static forceinline constexpr bool ParseSign(FormatSpec& fmtSpec, uint32_t ch) noexcept
    {
        // sign        ::=  "+" | "-" | " "
             if (ch == '+') fmtSpec.SignFlag = FormatSpec::Sign::Pos;
        else if (ch == '-') fmtSpec.SignFlag = FormatSpec::Sign::Neg;
        else if (ch == ' ') fmtSpec.SignFlag = FormatSpec::Sign::Space;
        else return false;
        return true;
    }

    static constexpr auto CommonColorMap26 = []()
    { // rgbcmykw, bcgkmrwy
        std::array<uint8_t, 26> ret = { 0 };
        ret['b' - 'a'] = static_cast<uint8_t>(0x80u | enum_cast(CommonColor::Blue));
        ret['c' - 'a'] = static_cast<uint8_t>(0x80u | enum_cast(CommonColor::Cyan));
        ret['g' - 'a'] = static_cast<uint8_t>(0x80u | enum_cast(CommonColor::Green));
        ret['k' - 'a'] = static_cast<uint8_t>(0x80u | enum_cast(CommonColor::Black));
        ret['m' - 'a'] = static_cast<uint8_t>(0x80u | enum_cast(CommonColor::Magenta));
        ret['r' - 'a'] = static_cast<uint8_t>(0x80u | enum_cast(CommonColor::Red));
        ret['w' - 'a'] = static_cast<uint8_t>(0x80u | enum_cast(CommonColor::White));
        ret['y' - 'a'] = static_cast<uint8_t>(0x80u | enum_cast(CommonColor::Yellow));
        return ret;
    }();
    
    template<typename Char, uint16_t Size = 1024>
    static constexpr ParseResult<Size> ParseString(const std::basic_string_view<Char> str) noexcept;
    template<typename Char, uint16_t Size = 1024>
    static constexpr ParseResult<Size> ParseString(const Char* str) noexcept
    {
        return ParseString<Char, Size>(std::basic_string_view<Char>(str));
    }
};



template<typename Char>
struct ParseLiterals;
template<>
struct ParseLiterals<char>
{
    static constexpr std::string_view BracePair = "{}", StrTrue = "true", StrFalse = "false";
};
template<>
struct ParseLiterals<char16_t>
{
    static constexpr std::u16string_view BracePair = u"{}", StrTrue = u"true", StrFalse = u"false";
};
template<>
struct ParseLiterals<char32_t>
{
    static constexpr std::u32string_view BracePair = U"{}", StrTrue = U"true", StrFalse = U"false";
};
template<>
struct ParseLiterals<wchar_t>
{
    static constexpr std::wstring_view BracePair = L"{}", StrTrue = L"true", StrFalse = L"false";
};
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template<>
struct ParseLiterals<char8_t>
{
    static constexpr std::u8string_view BracePair = u8"{}", StrTrue = u8"true", StrFalse = u8"false";
};
#endif

template<typename Char>
struct FormatterParserCh : public FormatterParser, public ParseLiterals<Char>
{
    using CharType = Char;
    using LCH = ParseLiterals<Char>;
    static constexpr Char Char_0 = static_cast<Char>('0'), Char_9 = static_cast<Char>('9'),
        Char_a = static_cast<Char>('a'), Char_A = static_cast<Char>('A'), 
        Char_f = static_cast<Char>('f'), Char_F = static_cast<Char>('F'), 
        Char_z = static_cast<Char>('z'), Char_Z = static_cast<Char>('Z'), 
        Char_b = static_cast<Char>('b'),
        Char_x = static_cast<Char>('x'),
        Char_LT = static_cast<Char>('<'), Char_GT = static_cast<Char>('>'), Char_UP = static_cast<Char>('^'),
        Char_LB = static_cast<Char>('{'), Char_RB = static_cast<Char>('}'), 
        Char_Plus = static_cast<Char>('+'), Char_Minus = static_cast<Char>('-'), 
        Char_Space = static_cast<Char>(' '), Char_Under = static_cast<Char>('_'), 
        Char_Dot = static_cast<Char>('.'), Char_Colon = static_cast<Char>(':'),
        Char_At = static_cast<Char>('@'), Char_NumSign = static_cast<Char>('#');

    template<uint32_t Limit>
    static forceinline constexpr std::pair<uint32_t, bool> ParseDecTail(uint32_t& val, const Char* __restrict dec, uint32_t len) noexcept
    {
        constexpr auto limit1 = Limit / 10;
        constexpr auto limitLast = Limit % 10;
        CM_ASSUME(val < 10);
        // limit make sure [i] will not overflow, also expects str len <= UINT32_MAX
        for (uint32_t i = 1; i < len; ++i)
        {
            const uint32_t ch = dec[i];
            if (ch >= '0' && ch <= '9')
            {
                const auto num = ch - '0';
                IF_UNLIKELY(val > limit1 || (val == limit1 && num > limitLast))
                {
                    return { i - 1, false };
                }
                val = val * 10 + num;
            }
            else
                return { i, true };
        }
        return { UINT32_MAX, true };
    }
    template<typename T>
    static constexpr bool ParseColor(T& result, size_t pos, const std::basic_string_view<Char>& str) noexcept
    {
        using namespace std::string_view_literals;
        IF_UNLIKELY(str.size() < 3) // at least 2 char needed + '@'
        {
            result.SetError(pos, ParseResultBase::ErrorCode::InvalidColor);
            return false;
        }
        const uint32_t fgbg = str[1];
        IF_UNLIKELY(fgbg != '<' && fgbg != '>')
        {
            result.SetError(pos, ParseResultBase::ErrorCode::MissingColorFGBG);
            return false;
        }
        const bool isFG = fgbg == '<';
        const uint32_t ch0 = str[2];
        if (ch0 == ' ')
        {
            if (str.size() == 3)
                return ColorOp::EmitDefault(result, pos, isFG);
        }
        else if (ch0 == 'x') // hex color
        {
            if (str.size() == 5) // @<xff
            {
                const auto num = ParseHex8bit(str[3], str[4]);
                IF_UNLIKELY(!num)
                {
                    result.SetError(pos, ParseResultBase::ErrorCode::Invalid8BitColor);
                    return false;
                }
                return ColorOp::Emit(result, pos, *num, isFG);
            }
            else if (str.size() == 9) // @<xffeedd
            {
                uint8_t rgb[3] = { 0, 0, 0 };
                for (uint8_t i = 0, j = 3; i < 3; ++i, j += 2)
                {
                    const auto num = ParseHex8bit(str[j], str[j + 1]);
                    IF_UNLIKELY(!num)
                    {
                        result.SetError(pos, ParseResultBase::ErrorCode::Invalid24BitColor);
                        return false;
                    }
                    rgb[i] = *num;
                }
                return ColorOp::Emit(result, pos, rgb[0], rgb[1], rgb[2], isFG);
            }
        }
        else if (str.size() == 3)
        {
            uint8_t tmp = 0;
            bool isBright = false;
            if (ch0 >= 'a' && ch0 <= 'z')
            {
                tmp = CommonColorMap26[ch0 - 'a'];
            }
            else if (ch0 >= 'A' && ch0 <= 'Z')
            {
                tmp = CommonColorMap26[ch0 - 'A'];
                isBright = true;
            }
            IF_LIKELY(tmp)
            {
                auto color = static_cast<CommonColor>(tmp & 0x7fu);
                if (isBright)
                    color |= CommonColor::BrightBlack; // make it bright
                return ColorOp::Emit(result, pos, color, isFG);
            }
        }
        result.SetError(pos, ParseResultBase::ErrorCode::InvalidColor);
        return false;
    }
    static forceinline constexpr uint32_t ParseFillAlign(FormatSpec& fmtSpec, const std::basic_string_view<Char>& str) noexcept
    {
        // [[fill]align]
        // fill        ::=  <a character other than '{' or '}'>
        // align       ::=  "<" | ">" | "^"
        switch (str[0])
        {
        case Char_LT: fmtSpec.Alignment = FormatSpec::Align::Left;   return 1;
        case Char_GT: fmtSpec.Alignment = FormatSpec::Align::Right;  return 1;
        case Char_UP: fmtSpec.Alignment = FormatSpec::Align::Middle; return 1;
        default: break;
        }
        if (str.size() >= 2)
        {
            switch (str[1])
            {
            case Char_LT: fmtSpec.Alignment = FormatSpec::Align::Left;   fmtSpec.Fill = str[0]; return 2;
            case Char_GT: fmtSpec.Alignment = FormatSpec::Align::Right;  fmtSpec.Fill = str[0]; return 2;
            case Char_UP: fmtSpec.Alignment = FormatSpec::Align::Middle; fmtSpec.Fill = str[0]; return 2;
            default: break;
            }
        }
        return 0;
    }
    static forceinline constexpr bool ParseWidth(ParseResultBase& result, FormatSpec& fmtSpec, uint32_t offset, uint32_t& idx, const std::basic_string_view<Char>& str) noexcept
    {
        // width       ::=  integer // | "{" [arg_id] "}"
        const uint32_t firstCh = str[idx];
        if (firstCh > '0' && firstCh <= '9') // find width
        {
            uint32_t width = firstCh - '0';
            const auto len = static_cast<uint32_t>(str.size());
            const auto [errIdx, inRange] = ParseDecTail<UINT16_MAX>(width, &str[idx], len - idx);
            IF_LIKELY(inRange)
            {
                fmtSpec.Width = static_cast<uint16_t>(width);
                if (errIdx == UINT32_MAX) // full finish
                    idx = len;
                else // assume successfully get a width
                    idx += errIdx;
            }
            ELSE_UNLIKELY // out of range
            {
                result.SetError(offset + idx + errIdx, ParseResultBase::ErrorCode::WidthTooLarge);
                return false;
            }
        }
        return true;
    }
    static forceinline constexpr bool ParsePrecision(ParseResultBase& result, FormatSpec& fmtSpec, uint32_t offset, uint32_t& idx, const std::basic_string_view<Char>& str) noexcept
    {
        // ["." precision]
        // precision   ::=  integer // | "{" [arg_id] "}"
        if (str[idx] == Char_Dot)
        {
            const auto len = static_cast<uint32_t>(str.size());
            IF_LIKELY(++idx < len)
            {
                const uint32_t firstCh = str[idx];
                IF_LIKELY(firstCh > '0' && firstCh <= '9') // find precision
                {
                    uint32_t precision = firstCh - '0';
                    const auto [errIdx, inRange] = ParseDecTail<UINT32_MAX>(precision, &str[idx], len - idx);
                    IF_LIKELY(inRange)
                    {
                        fmtSpec.Precision = precision;
                        if (errIdx == UINT32_MAX) // full finish
                            idx = len;
                        else // assume successfully get a width
                            idx += errIdx;
                        return true;
                    }
                    ELSE_UNLIKELY // out of range
                    {
                        result.SetError(offset + idx + errIdx, ParseResultBase::ErrorCode::WidthTooLarge);
                        return false;
                    }
                }
            }
            result.SetError(offset + idx, ParseResultBase::ErrorCode::InvalidPrecision);
            return false;
        }
        return true;
    }
    static forceinline constexpr bool ParseFormatSpec(ParseResultBase& result, FormatSpec& fmtSpec, const uint32_t offset, const std::basic_string_view<Char>& str) noexcept
    {
        // format_spec ::=  [[fill]align][sign]["#"]["0"][width]["." precision][type]
        fmtSpec = {}; // clean up
        const auto len = static_cast<uint32_t>(str.size()); // hack on x86 to avoid using r64
        uint32_t idx = ParseFillAlign(fmtSpec, str);
        if (idx == len) return true;
        if (ParseSign(fmtSpec, str[idx]))
        {
            if (fmtSpec.SignFlag != FormatSpec::Sign::None)
                fmtSpec.Type.Type = ArgDispType::Numeric;
            if (++idx == len) return true;
        }
        if (str[idx] == Char_NumSign)
        {
            fmtSpec.AlterForm = true;
            if (++idx == len) return true;
        }
        if (str[idx] == Char_0)
        {
            fmtSpec.ZeroPad = true;
            fmtSpec.Type.Type = ArgDispType::Numeric;
            if (++idx == len) return true;
        }
        IF_UNLIKELY(!ParseWidth(result, fmtSpec, offset, idx, str))
            return false;
        if (idx == len) return true;
        IF_UNLIKELY(!ParsePrecision(result, fmtSpec, offset, idx, str))
            return false;
        if (idx != len)
        {
            const uint32_t typestr = str[idx];
            IF_LIKELY(typestr <= 127)
            {
                const FormatSpec::TypeIdentifier type{ static_cast<char>(typestr) };
                IF_LIKELY(type.Extra != 0xff)
                {
                    idx++;
                    // numeric type check
                    if (fmtSpec.Type.Type == ArgDispType::Numeric && type.Type != ArgDispType::Custom)
                    {
                        const auto newType = ParseResultBase::CheckCompatible(ArgDispType::Numeric, type.Type);
                        IF_UNLIKELY(!newType) // don't replace fmtSpec.Type.Type because if it pass, it will be [type.Type]
                        {
                            result.SetError(offset + idx, ParseResultBase::ErrorCode::IncompNumSpec);
                            return false;
                        }
                    }
                    // time type check
                    if (type.Type == ArgDispType::Date)
                    {
                        // zeropad & signflag already checked
                        IF_UNLIKELY(fmtSpec.AlterForm || fmtSpec.Precision != 0 || fmtSpec.Width != 0 || fmtSpec.Fill != ' ' ||
                            fmtSpec.Alignment != FormatSpec::Align::None)
                        {
                            result.SetError(offset + idx, ParseResultBase::ErrorCode::IncompDateSpec);
                            return false;
                        }
                    }
                    else if (type.Type == ArgDispType::Color)
                    {
                        // zeropad & signflag already checked
                        IF_UNLIKELY(fmtSpec.Precision != 0 || fmtSpec.Width != 0 || fmtSpec.Fill != ' ')
                        {
                            result.SetError(offset + idx, ParseResultBase::ErrorCode::IncompColorSpec);
                            return false;
                        }
                    }
                    // extra field handling
                    if (idx != len)
                    {
                        IF_LIKELY(type.Type == ArgDispType::Date || type.Type == ArgDispType::Custom)
                        {
                            fmtSpec.FmtOffset = static_cast<uint16_t>(offset + idx);
                            fmtSpec.FmtLen = static_cast<uint16_t>(len - idx);
                        }
                        else
                        {
                            result.SetError(offset + idx, ParseResultBase::ErrorCode::ExtraFmtSpec);
                            return false;
                        }
                    }
                    fmtSpec.Type = type;
                    return true;
                }
            }
            result.SetError(offset + idx, ParseResultBase::ErrorCode::InvalidType);
            return false;
        }
        return true;
    }
    template<uint16_t Size>
    static constexpr void ParseString(ParseResult<Size>& result, const std::basic_string_view<Char> str) noexcept
    {
        using namespace std::string_view_literals;
        constexpr auto End = std::basic_string_view<Char>::npos;
        static_assert(End == SIZE_MAX);
        size_t offset = 0;
        const auto size = str.size();
        IF_UNLIKELY(size >= UINT16_MAX)
        {
            return result.SetError(0, ParseResultBase::ErrorCode::FmtTooLong);
        }
        FormatSpec fmtSpec; // resued outside of loop
        while (offset < size)
        {
            const auto occurL = str.find_first_of(Char_LB, offset), occurR = str.find_first_of(Char_RB, offset);
            const auto occur = std::min(occurL, occurR);
            bool isBrace = false;
            if (occur != End)
            {
                IF_LIKELY(occur + 1 < size)
                {
                    isBrace = str[occur + 1] == str[occur]; // "{{" or "}}", emit single '{' or '}'
                }
                IF_UNLIKELY(!isBrace && occurR < occurL) // find '}' first and it's not brace
                {
                    return result.SetError(occur, ParseResultBase::ErrorCode::MissingLeftBrace);
                }
            }
            if (occur != offset) // need emit FmtStr
            {
                const auto strLen = (occur == End ? size : occur) - offset + (isBrace ? 1 : 0);
                IF_UNLIKELY(!BuiltinOp::EmitFmtStr(result, offset, strLen))
                    return;
                if (occur == End) // finish
                    return;
                offset += strLen; // eat string
                if (isBrace)
                {
                    offset += 1; // eat extra brace
                    continue;
                }
            }
            else
            {
                if (isBrace) // need emit Brace
                {
                    IF_UNLIKELY(!BuiltinOp::EmitBrace(result, occur, Char_LB == str[occur]))
                        return;
                    offset += 2; // eat brace "{{" or "}}"
                    continue;
                }
            }
            CM_ASSUME(occur == offset);
            CM_ASSUME(occurL < UINT16_MAX);
            CM_ASSUME(occurL < occurR);
            CM_ASSUME(occurL == offset);

            // not brace and already find '{' and '}' must be after it
            IF_UNLIKELY(occurR == End) // '}' not found
            {
                return result.SetError(occur, ParseResultBase::ErrorCode::MissingRightBrace);
            }
            // occurL and occurR both exist, and should within UINT32
            CM_ASSUME(occurR < UINT16_MAX);
            const auto argfmtOffset = static_cast<uint32_t>(occurL + 1);
            const auto argfmtLen = static_cast<uint32_t>(occurR) - argfmtOffset;
            IF_LIKELY(argfmtLen == 0) // "{}"
            {
                IF_UNLIKELY(result.NextArgIdx >= ParseResultCommon::IdxArgSlots)
                {
                    return result.SetError(offset, ParseResultBase::ErrorCode::TooManyIdxArg);
                }
                const auto argIdx = result.NextArgIdx++;
                result.IdxArgCount = std::max(result.NextArgIdx, result.IdxArgCount);
                IF_UNLIKELY(!ArgOp::EmitDefault(result, offset, argIdx))
                    return;
                offset += 2; // eat "{}"
                continue;
            }

            // begin arg parsing
            CM_ASSUME(argfmtOffset + argfmtLen < str.size());
            const auto argfmt = str.substr(argfmtOffset, argfmtLen);
            const auto specSplit_ = argfmt.find_first_of(Char_Colon);
            const auto hasSpecSplit = specSplit_ != End;
            const auto specSplit = static_cast<uint32_t>(specSplit_); // hack for x86 to avoid using r64
            uint8_t argIdx = 0;
            bool isNamed = false;
            ArgDispType* dstType = nullptr;
            IF_UNLIKELY(specSplit == argfmtLen - 1)
            { // find it at the end of argfmt, specSplit == End casted to UINT32 still works
                return result.SetError(offset, ParseResultBase::ErrorCode::MissingFmtSpec);
            }
            if (specSplit > 0) // specSplit == End || specSplit > 0, has arg_id
            {
                // see https://fmt.dev/latest/syntax.html#formatspec
                // replacement_field ::=  "{" field_detail "}"
                // field_detail      ::=  color | arg
                // color             ::=  "@" "<" | ">" id_start | digit
                // arg               ::=  [arg_id] [":" (format_spec)]
                // arg_id            ::=  integer | identifier | color
                // integer           ::=  digit+
                // digit             ::=  "0"..."9"
                // identifier        ::=  id_start id_continue*
                // id_start          ::=  "a"..."z" | "A"..."Z" | "_"
                // id_continue       ::=  id_start | digit
                const uint32_t firstCh = argfmt[0]; // idPart[0]
                // specSplit == End casted to UINT32 still pick argfmtLen
                const auto idPartLen = std::min(specSplit, argfmtLen);
                // const auto idPart = argfmt.substr(0, idPartLen); // specSplit == End is acceptable
                if (firstCh == '0')
                {
                    IF_UNLIKELY(idPartLen != 1)
                    {
                        return result.SetError(offset, ParseResultBase::ErrorCode::InvalidArgIdx);
                    }
                    argIdx = 0;
                }
                else if (firstCh > '0' && firstCh <= '9')
                {
                    uint32_t id = firstCh - '0';
                    const auto [errIdx, inRange] = ParseDecTail<ParseResultCommon::IdxArgSlots>(id, &argfmt[0], idPartLen);
                    IF_UNLIKELY(errIdx != UINT32_MAX) // not full finish
                    {
                        return result.SetError(offset + errIdx, inRange ? ParseResultBase::ErrorCode::InvalidArgIdx : ParseResultBase::ErrorCode::ArgIdxTooLarge);
                    }
                    // already checked by inRange
                    // if (id >= ParseResultCommon::IdxArgSlots)
                    // {
                    //     return result.SetError(offset, ParseResultBase::ErrorCode::TooManyIdxArg);
                    // }
                    argIdx = static_cast<uint8_t>(id);
                }
                else if ((firstCh >= 'a' && firstCh <= 'z') ||
                    (firstCh >= 'A' && firstCh <= 'Z') || firstCh == '_')
                {
                    for (uint32_t i = 1; i < idPartLen; ++i)
                    {
                        const auto ch = argfmt[i];
                        IF_LIKELY((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                            (ch >= '0' && ch <= '9') || ch == '_')
                            continue;
                        ELSE_UNLIKELY
                        {
                            return result.SetError(offset + i, ParseResultBase::ErrorCode::InvalidArgName);
                        }
                    }
                    IF_UNLIKELY(idPartLen > UINT8_MAX)
                    {
                        return result.SetError(offset, ParseResultBase::ErrorCode::ArgNameTooLong);
                    }
                    isNamed = true;
                    CM_ASSUME(idPartLen <= argfmt.size());
                    const auto idPart = argfmt.substr(0, idPartLen);
                    for (uint8_t i = 0; i < result.NamedArgCount; ++i)
                    {
                        auto& target = result.NamedTypes[i];
                        if (idPartLen != target.Length) continue;
                        CM_ASSUME(target.Offset + target.Length < str.size());
                        const auto targetName = str.substr(target.Offset, target.Length);
                        if (targetName == idPart)
                        {
                            argIdx = i;
                            dstType = &target.Type;
                            break;
                        }
                    }
                    if (!dstType)
                    {
                        IF_UNLIKELY(result.NamedArgCount >= ParseResultCommon::NamedArgSlots)
                        {
                            return result.SetError(offset, ParseResultBase::ErrorCode::TooManyNamedArg);
                        }
                        argIdx = result.NamedArgCount++;
                        auto& target = result.NamedTypes[argIdx];
                        target.Offset = static_cast<uint16_t>(argfmtOffset);
                        target.Length = static_cast<uint8_t>(idPartLen);
                        dstType = &target.Type;
                    }
                }
                else if (firstCh == '@') // Color
                {
                    IF_UNLIKELY(hasSpecSplit) // has split
                    {
                        return result.SetError(offset + 1, ParseResultBase::ErrorCode::InvalidColor);
                    }
                    IF_UNLIKELY(!ParseColor(result, offset + 1, argfmt))
                        return;
                    offset += 2 + argfmtLen; // eat "{xxx}"
                    continue;
                }
                ELSE_UNLIKELY
                {
                    return result.SetError(offset, ParseResultBase::ErrorCode::InvalidArgName);
                }
            }
            else
            {
                IF_UNLIKELY(result.NextArgIdx >= ParseResultCommon::IdxArgSlots)
                {
                    return result.SetError(offset, ParseResultBase::ErrorCode::TooManyIdxArg);
                }
                argIdx = result.NextArgIdx++;
            }
            if (!isNamed) // index arg, update argcount
            {
                dstType = &result.IndexTypes[argIdx];
                result.IdxArgCount = std::max(static_cast<uint8_t>(argIdx + 1), result.IdxArgCount);
            }
            if (hasSpecSplit)
            {
                CM_ASSUME(specSplit + 1 < argfmt.size());
                IF_UNLIKELY(!ParseFormatSpec(result, fmtSpec, argfmtOffset + specSplit + 1, argfmt.substr(specSplit + 1)))
                    return;
            }

            IF_UNLIKELY(!ArgOp::Emit(&result, offset, hasSpecSplit ? &fmtSpec : nullptr, dstType, argIdx, isNamed))
                return;
            offset += 2 + argfmtLen; // eat "{xxx}"
        }
    }
};

template<typename Char, uint16_t Size>
forceinline constexpr ParseResult<Size> FormatterParser::ParseString(const std::basic_string_view<Char> str) noexcept
{
    ParseResult<Size> result;
    FormatterParserCh<Char>::ParseString(result, str);
    return result;
}


struct StrArgInfo
{
    common::span<const uint8_t> Opcodes;
    common::span<const ArgDispType> IndexTypes;
    common::span<const ParseResultCommon::NamedArgType> NamedTypes;
};
template<typename Char>
struct StrArgInfoCh : public StrArgInfo
{
    using CharType = Char;
    std::basic_string_view<Char> FormatString;
};


template<uint16_t Size>
template<typename Char>
forceinline constexpr StrArgInfoCh<Char> ParseResult<Size>::ToInfo(const std::basic_string_view<Char> str) const noexcept
{
    common::span<const ArgDispType> idxTypes;
    IF_LIKELY(IdxArgCount > 0)
        idxTypes = { IndexTypes, IdxArgCount };
    common::span<const ParseResultCommon::NamedArgType> namedTypes;
    if (NamedArgCount > 0)
        namedTypes = { NamedTypes, NamedArgCount };
    common::span<const uint8_t> opCodes = { Opcodes, OpCount };
    return { { opCodes, idxTypes, namedTypes }, str };
}


template<uint8_t IdxArgCount>
struct IdxArgLimiter
{
    ArgDispType IndexTypes[IdxArgCount] = { ArgDispType::Any };
    constexpr IdxArgLimiter(const ArgDispType* type) noexcept
    {
        for (uint8_t i = 0; i < IdxArgCount; ++i)
            IndexTypes[i] = type[i];
    }
};
template<>
struct IdxArgLimiter<0>
{ 
    constexpr IdxArgLimiter(const ArgDispType*) noexcept {}
};
template<uint8_t NamedArgCount>
struct NamedArgLimiter
{
    ParseResultCommon::NamedArgType NamedTypes[NamedArgCount] = {};
    constexpr NamedArgLimiter(const ParseResultCommon::NamedArgType* type) noexcept
    {
        for (uint8_t i = 0; i < NamedArgCount; ++i)
            NamedTypes[i] = type[i];
    }
};
template<>
struct NamedArgLimiter<0>
{ 
    constexpr NamedArgLimiter(const ParseResultCommon::NamedArgType*) noexcept {}
};
template<typename Char, uint16_t OpCount>
struct OpHolder
{
    std::basic_string_view<Char> FormatString;
    uint8_t Opcodes[OpCount] = { 0 };
    constexpr OpHolder(std::basic_string_view<Char> str, const uint8_t* op) noexcept : FormatString(str)
    {
        for (uint16_t i = 0; i < OpCount; ++i)
            Opcodes[i] = op[i];
    }
};
template<typename Char>
struct OpHolder<Char, 0>
{
    std::basic_string_view<Char> FormatString;
    constexpr OpHolder(std::basic_string_view<Char> str, const uint8_t*) noexcept : FormatString(str)
    { }
};

struct CompileTimeFormatter {};

template<typename Char, uint16_t OpCount_, uint8_t NamedArgCount_, uint8_t IdxArgCount_>
struct COMMON_EMPTY_BASES TrimedResult : public CompileTimeFormatter, public OpHolder<Char, OpCount_>, NamedArgLimiter<NamedArgCount_>, IdxArgLimiter<IdxArgCount_>
{
    using CharType = Char;
    static constexpr uint16_t OpCount = OpCount_;
    static constexpr uint8_t NamedArgCount = NamedArgCount_;
    static constexpr uint8_t IdxArgCount = IdxArgCount_;
    template<uint16_t Size>
    constexpr TrimedResult(const ParseResult<Size>& result, std::basic_string_view<Char> fmtStr) noexcept :
        OpHolder<Char, OpCount>(fmtStr, result.Opcodes),
        NamedArgLimiter<NamedArgCount>(result.NamedTypes),
        IdxArgLimiter<IdxArgCount>(result.IndexTypes)
    { }
    constexpr operator StrArgInfoCh<Char>() const noexcept
    {
        common::span<const ArgDispType> idxTypes;
        if constexpr (IdxArgCount > 0)
            idxTypes = this->IndexTypes;
        common::span<const ParseResultCommon::NamedArgType> namedTypes;
        if constexpr (NamedArgCount > 0)
            namedTypes = this->NamedTypes;
        return { { this->Opcodes, idxTypes, namedTypes }, this->FormatString };
    }
    constexpr StrArgInfoCh<Char> ToStrArgInfo() const noexcept
    {
        return *this;
    }
};


struct NamedArgTag {};
template<typename T>
struct NamedArgDynamic : public NamedArgTag
{
    std::string_view Name;
    T Data;
    template<typename U>
    constexpr NamedArgDynamic(std::string_view name, U&& data) noexcept :
        Name(name), Data(std::forward<U>(data)) { }
};
template<typename T>
inline constexpr auto WithName(std::string_view name, T&& arg) noexcept -> NamedArgDynamic<std::decay_t<T>>
{
    return { name, std::forward<T>(arg) };
}
#define NAMEARG(name) [](auto&& arg)                \
{                                                   \
    using T = decltype(arg);                        \
    using U = std::decay_t<T>;                      \
    struct NameT { std::string_view Name = name; }; \
    struct NamedArg : public NamedArgTag            \
    {                                               \
        using NameType = NameT;                     \
        NameT Name;                                 \
        U Data;                                     \
        constexpr NamedArg(T data) noexcept :       \
            Data(std::forward<T>(data)) { }         \
    };                                              \
    return NamedArg{std::forward<T>(arg)};          \
}


using NamedMapper = std::array<uint8_t, ParseResultCommon::NamedArgSlots>;

template<uint16_t N>
struct StaticArgPack
{
    std::array<uint16_t, N == 0 ? uint16_t(1) : N> ArgStore = { 0 };
    uint16_t CurrentSlot = 0;
    template<typename T>
    forceinline void Put(T arg, uint16_t idx) noexcept
    {
        constexpr auto NeedSize = sizeof(T);
        constexpr auto NeedSlot = (NeedSize + 1) / 2;
        Expects(CurrentSlot + NeedSlot <= N);
        *reinterpret_cast<T*>(&ArgStore[CurrentSlot]) = arg;
        ArgStore[idx] = CurrentSlot;
        CurrentSlot += NeedSlot;
    }
};


struct CompactDate
{
    int16_t Year;
    uint16_t MWD; // 4+3+9
    uint16_t FDH; // 4+5+5
    uint8_t Minute;
    uint8_t Second;
    static constexpr uint16_t CompactMWD(uint32_t month, uint32_t week, uint32_t day) noexcept
    {
        uint32_t ret = 0;
        ret |= day & 0x1ffu;
        ret |= (week & 0x7u) << 9;
        ret |= (month & 0xfu) << 12;
        return static_cast<uint16_t>(ret);
    }
    static constexpr uint16_t CompactFDH(int32_t dst, uint32_t day, uint32_t hour) noexcept
    {
        uint32_t ret = 0;
        ret |= hour & 0x1fu;
        ret |= (day & 0x1fu) << 5;
        if (dst < 0)
            ret |= 0x8000u;
        else if (dst > 0)
            ret |= 0x4000u;
        else
            ret |= 0x0000u;
        return static_cast<uint16_t>(ret);
    }
    constexpr CompactDate() noexcept : Year(0), MWD(0), FDH(0), Minute(0), Second(0) {}
    constexpr CompactDate(const std::tm& date) noexcept :
        Year(static_cast<int16_t>(date.tm_year)), MWD(CompactMWD(date.tm_mon, date.tm_wday, date.tm_yday)),
        FDH(CompactFDH(date.tm_isdst, date.tm_mday, date.tm_hour)), 
        Minute(static_cast<uint8_t>(date.tm_min)), Second(static_cast<uint8_t>(date.tm_sec)) {}
    constexpr operator std::tm() const noexcept
    {
        std::tm date{};
        date.tm_year = Year;
        date.tm_mon = MWD >> 12;
        date.tm_wday = (MWD >> 9) & 0x7;
        date.tm_yday = MWD & 0x1ff;
        date.tm_isdst = FDH & 0x80 ? -1 : (FDH & 0x40 ? 1 : 0);
        date.tm_mday = (FDH >> 5) & 0x1f;
        date.tm_hour = FDH & 0x1f;
        date.tm_min = Minute;
        date.tm_sec = Second;
        return date;
    }
};


template<typename Char>
inline auto FormatAs(const SharedString<Char>& arg)
{
    return static_cast<std::basic_string_view<Char>>(arg);
}


namespace detail
{
template<typename T>
using HasFormatAsMemFn = decltype(std::declval<const T&>().FormatAs());
template<typename T>
using HasFormatAsSpeFn = decltype(FormatAs(std::declval<const T&>()));
template<typename T>
using HasFormatWithMemFn = decltype(std::declval<const T&>().FormatWith(std::declval<FormatterExecutor&>(), std::declval<FormatterExecutor::Context&>(), std::declval<const FormatSpec*>()));
template<typename T>
using HasFormatWithSpeFn = decltype(FormatWith(std::declval<const T&>(), std::declval<FormatterExecutor&>(), std::declval<FormatterExecutor::Context&>(), std::declval<const FormatSpec*>()));
using FmtWithPtr = void(*)(const void*, FormatterExecutor&, FormatterExecutor::Context&, const FormatSpec*);
struct FmtWithPair
{
    const void* Data;
    FmtWithPtr Executor;
};
}


struct ArgInfo
{
    std::string_view Names[ParseResultCommon::NamedArgSlots] = {};
    ArgRealType NamedTypes[ParseResultCommon::NamedArgSlots] = { ArgRealType::Error };
    ArgRealType IndexTypes[ParseResultCommon::IdxArgSlots] = { ArgRealType::Error };
    uint8_t NamedArgCount = 0, IdxArgCount = 0;
    template<typename T>
    forceinline static constexpr ArgRealType EncodeTypeSizeData() noexcept
    {
        switch (sizeof(T))
        {
        case 1:   return static_cast<ArgRealType>(0x00);
        case 2:   return static_cast<ArgRealType>(0x10);
        case 4:   return static_cast<ArgRealType>(0x20);
        case 8:   return static_cast<ArgRealType>(0x30);
        case 16:  return static_cast<ArgRealType>(0x40);
        case 32:  return static_cast<ArgRealType>(0x50);
        case 64:  return static_cast<ArgRealType>(0x60);
        default:  return static_cast<ArgRealType>(0xff);
        }
    }
    template<typename T>
    forceinline static constexpr ArgRealType GetCharTypeData() noexcept
    {
        constexpr auto data = EncodeTypeSizeData<T>();
        static_assert(enum_cast(data) <= 0x20);
        return data;
    }
    template<typename T>
    static constexpr bool CheckCharType() noexcept
    {
        bool result = std::is_same_v<T, char> || std::is_same_v<T, char16_t> || std::is_same_v<T, wchar_t> || std::is_same_v<T, char32_t>;
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
        result |= std::is_same_v<T, char8_t>;
#endif
        return result;
    }
    template<typename T, bool AllowPair = false>
    static constexpr bool CheckColorType() noexcept
    {
        using U = std::decay_t<T>;
        bool result = std::is_same_v<U, CommonColor> || std::is_same_v<U, ScreenColor>;
        if constexpr (AllowPair && is_specialization<U, std::pair>::value)
        {
            using T1 = std::decay_t<typename U::first_type>;
            using T2 = std::decay_t<typename U::second_type>;
            result = std::is_same_v<T1, T2> && CheckColorType<T1, false>() && CheckColorType<T2, false>();
        }
        return result;
    }
    template<typename T>
    static constexpr ArgRealType GetArgType() noexcept
    {
        using U = std::decay_t<T>;
        if constexpr (common::is_specialization<U, std::basic_string_view>::value || 
            common::is_specialization<U, std::basic_string>::value)
        {
            return ArgRealType::String | GetCharTypeData<typename U::value_type>();
        }
        else if constexpr (CheckCharType<U>())
        {
            return ArgRealType::Char | GetCharTypeData<U>();
        }
        else if constexpr (std::is_pointer_v<U>)
        {
            using X = std::decay_t<std::remove_pointer_t<U>>;
            if constexpr (CheckCharType<X>())
                return ArgRealType::String | ArgRealType::StrPtrBit | GetCharTypeData<X>();
            else
                return ArgRealType::Ptr | (std::is_same_v<X, void> ? ArgRealType::PtrVoidBit : ArgRealType::EmptyBit);
        }
        else if constexpr (std::is_floating_point_v<U>)
        {
            return ArgRealType::Float | EncodeTypeSizeData<U>();
        }
        else if constexpr (std::is_same_v<U, bool>)
        {
            return ArgRealType::Bool;
        }
        else if constexpr (std::is_integral_v<U>)
        {
            return (std::is_unsigned_v<U> ? ArgRealType::UInt : ArgRealType::SInt) | EncodeTypeSizeData<U>();
        }
        else if constexpr (std::is_same_v<U, std::tm> || std::is_same_v<U, CompactDate>)
        {
            return ArgRealType::Date;
        }
        else if constexpr (is_specialization<U, std::chrono::time_point>::value)
        {
            return ArgRealType::Date | ArgRealType::DateDeltaBit;
        }
        else if constexpr (CheckColorType<U>())
        {
            // size 1,2,4,8: CommonColor, pair<CommonColor>, ScreenColor, pair<ScreenColor>
            return ArgRealType::Color | EncodeTypeSizeData<U>();
        }
        else if constexpr (is_detected_v<detail::HasFormatAsMemFn, U>)
        {
            return GetArgType<detail::HasFormatAsMemFn<U>>();
        }
        else if constexpr (is_detected_v<detail::HasFormatAsSpeFn, U>)
        {
            return GetArgType<detail::HasFormatAsSpeFn<U>>();
        }
        else if constexpr (is_detected_v<detail::HasFormatWithMemFn, U> || is_detected_v<detail::HasFormatWithSpeFn, U>)
        {
            return ArgRealType::Custom;
        }
        else
        {
            static_assert(!AlwaysTrue<T>, "unsupported type");
        }
    }
    template<typename T>
    static constexpr void ParseAnArg(ArgInfo& result) noexcept
    {
        if constexpr (std::is_base_of_v<NamedArgTag, T>)
        {
            result.NamedTypes[result.NamedArgCount] = GetArgType<decltype(std::declval<T&>().Data)>();
            if constexpr (!common::is_specialization<T, NamedArgDynamic>::value) // static
            {
                constexpr typename T::NameType Name {};
                static_assert(!Name.Name.empty(), "Arg name should not be empty");
                result.Names[result.NamedArgCount] = Name.Name;
            }
            result.NamedArgCount++;
        }
        else
        {
            result.IndexTypes[result.IdxArgCount++] = GetArgType<T>();
        }
    }
    template<typename... Args>
    static constexpr ArgInfo ParseArgs() noexcept
    {
        ArgInfo info;
        (..., ParseAnArg<Args>(info));
        return info;
    }

    template<typename T>
    forceinline static auto BoxAnArg([[maybe_unused]] T&& arg) noexcept
    {
        if constexpr (!std::is_base_of_v<NamedArgTag, T>)
        {
            using U = std::decay_t<T>;
            if constexpr (common::is_specialization<U, std::basic_string_view>::value ||
                common::is_specialization<U, std::basic_string>::value)
            {
                return std::pair{ reinterpret_cast<uintptr_t>(arg.data()), arg.size() };
            }
            else if constexpr (CheckCharType<U>())
            {
                return arg;
            }
            else if constexpr (std::is_pointer_v<U>)
            {
                return reinterpret_cast<uintptr_t>(arg);
            }
            else if constexpr (std::is_floating_point_v<U>)
            {
                return arg;
            }
            else if constexpr (std::is_same_v<U, bool>)
            {
                return static_cast<uint16_t>(arg ? 1 : 0);
            }
            else if constexpr (std::is_integral_v<U>)
            {
                return arg;
            }
            else if constexpr (std::is_same_v<U, CompactDate>)
            {
                return arg;
            }
            else if constexpr (std::is_same_v<U, std::tm>)
            {
                return CompactDate(arg);
            }
            else if constexpr (is_specialization<U, std::chrono::time_point>::value)
            {
                const auto deltaMs = std::chrono::duration_cast<std::chrono::duration<int64_t, std::milli>>(arg.time_since_epoch()).count();
                return deltaMs;
            }
            else if constexpr (CheckColorType<U>())
            {
                return arg;
            }
            else if constexpr (is_detected_v<detail::HasFormatAsMemFn, U>)
            {
                return BoxAnArg(arg.FormatAs());
            }
            else if constexpr (is_detected_v<detail::HasFormatAsSpeFn, U>)
            {
                return BoxAnArg(FormatAs(arg));
            }
            else if constexpr (is_detected_v<detail::HasFormatWithMemFn, U>)
            {
                detail::FmtWithPair data{ &arg, [](const void* data, FormatterExecutor& executor, FormatterExecutor::Context& context, const FormatSpec* spec)
                    {
                        reinterpret_cast<const U*>(data)->FormatWith(executor, context, spec);
                    } };
                return data;
            }
            else if constexpr (is_detected_v<detail::HasFormatWithSpeFn, U>)
            {
                detail::FmtWithPair data{ &arg, [](const void* data, FormatterExecutor& executor, FormatterExecutor::Context& context, const FormatSpec* spec)
                    {
                        FormatWith(*reinterpret_cast<const U*>(data), executor, context, spec);
                    } };
                return data;
            }
            else
            {
                static_assert(!AlwaysTrue<T>, "unsupported type");
            }
        }
        else
            return BoxAnArg(arg.Data);
    }
    template<typename P, typename T>
    forceinline static void PackAnArg([[maybe_unused]] P& pack, [[maybe_unused]] T&& arg, [[maybe_unused]] uint16_t& idx) noexcept
    {
        if constexpr (!std::is_base_of_v<NamedArgTag, T>)
        {
            pack.Put(BoxAnArg(std::forward<T>(arg)), idx++);
        }
    }
    template<typename P, typename T>
    forceinline static void PackAnNamedArg([[maybe_unused]] P& pack, [[maybe_unused]] T&& arg, [[maybe_unused]] uint16_t& idx) noexcept
    {
        if constexpr (std::is_base_of_v<NamedArgTag, T>)
            PackAnArg(pack, arg.Data, idx);
    }
    template<typename... Ts>
    forceinline static auto PackArgsStatic(Ts&&... args) noexcept
    {
        constexpr auto ArgCount = sizeof...(Ts);
        constexpr auto ArgSize = (((sizeof(decltype(BoxAnArg(std::forward<Ts>(args)))) + 1) / 2) + ... + size_t(0));
        StaticArgPack<ArgCount + ArgSize> pack;
        pack.CurrentSlot = ArgCount;
        [[maybe_unused]] uint16_t argIdx = 0;
        (..., PackAnArg     (pack, std::forward<Ts>(args), argIdx));
        (..., PackAnNamedArg(pack, std::forward<Ts>(args), argIdx));
        return pack;
    }
};


struct ArgChecker
{
    struct NamedCheckResult
    {
        NamedMapper Mapper{ 0 };
        std::optional<uint8_t> AskIndex;
        std::optional<uint8_t> GiveIndex;
    };
    static forceinline constexpr bool CheckCompatible(ArgDispType ask, ArgRealType give_) noexcept
    {
        const auto give = give_ & ArgRealType::BaseTypeMask; // ArgInfo::CleanRealType(give_);
        if (give == ArgRealType::Error) return false;
        if (ask == ArgDispType::Any) return true;
        const auto isInteger = give == ArgRealType::SInt || give == ArgRealType::UInt || give == ArgRealType::Char;
        switch (ask)
        {
        case ArgDispType::Integer:
            return isInteger || give == ArgRealType::Bool;
        case ArgDispType::Float:
            return give == ArgRealType::Float;
        case ArgDispType::Numeric:
            return isInteger || give == ArgRealType::Bool || give == ArgRealType::Float;
        case ArgDispType::Char:
            return give == ArgRealType::Char || give == ArgRealType::Bool;
        case ArgDispType::String:
            return give == ArgRealType::String || give == ArgRealType::Bool;
        case ArgDispType::Pointer:
            return give == ArgRealType::Ptr || give == ArgRealType::String;
        case ArgDispType::Color:
            return give == ArgRealType::Color;
        case ArgDispType::Date:
            return give == ArgRealType::Date;
        case ArgDispType::Custom:
            return give == ArgRealType::Custom;
        default:
            return false;
        }
    }
    static forceinline constexpr uint32_t GetIdxArgMismatch(const ArgDispType* ask, const ArgRealType* give, uint8_t count) noexcept
    {
        for (uint8_t i = 0; i < count; ++i)
            if (!CheckCompatible(ask[i], give[i]))
                return i;
        return ParseResultCommon::IdxArgSlots;
    }
    template<uint32_t Index>
    static constexpr void CheckIdxArgMismatch() noexcept
    {
        static_assert(Index == ParseResultCommon::IdxArgSlots, "Type mismatch");
    }
    template<typename Char>
    static constexpr NamedCheckResult GetNamedArgMismatch(const ParseResultCommon::NamedArgType* ask, const std::basic_string_view<Char> fmtStr, uint8_t askCount,
        const ArgRealType* give, const std::string_view* giveNames, uint8_t giveCount) noexcept
    {
        NamedCheckResult ret;
        for (uint8_t i = 0; i < askCount; ++i)
        {
            CM_ASSUME(ask[i].Offset + ask[i].Length < fmtStr.size());
            const auto askName = fmtStr.substr(ask[i].Offset, ask[i].Length);
            bool found = false, match = false;
            uint8_t j = 0;
            for (; j < giveCount; ++j)
            {
                const auto& giveName = giveNames[j];
                if (giveName.size() == askName.size())
                {
                    bool fullMatch = true;
                    for (size_t k = 0; k < giveName.size(); ++k)
                    {
                        if (static_cast<Char>(giveName[k]) != askName[k])
                        {
                            fullMatch = false;
                            break;
                        }
                    }
                    if (fullMatch)
                    {
                        found = true;
                        match = CheckCompatible(ask[i].Type, give[j]);
                        break;
                    }
                }
            }
            if (found && match)
            {
                ret.Mapper[i] = j;
                continue;
            }
            ret.AskIndex = i;
            if (!match)
                ret.GiveIndex = j;
            break;
        }
        return ret;
    }
    template<uint32_t AskIdx, uint32_t GiveIdx>
    static constexpr void CheckNamedArgMismatch() noexcept
    {
        static_assert(GiveIdx == ParseResultCommon::NamedArgSlots, "Named arg type mismatch at [AskIdx, GiveIdx]");
        static_assert(AskIdx  == ParseResultCommon::NamedArgSlots, "Missing named arg at [AskIdx]");
    }
    SYSCOMMONAPI static void CheckDDBasic(const StrArgInfo& strInfo, const ArgInfo& argInfo);
    template<typename Char>
    SYSCOMMONAPI static NamedMapper CheckDDNamedArg(const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo);
    template<typename Char>
    static NamedMapper CheckDD(const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo)
    {
        CheckDDBasic(strInfo, argInfo);
        return CheckDDNamedArg(strInfo, argInfo);
    }
    template<typename Char, uint16_t Op, uint8_t Na, uint8_t Ia>
    static NamedMapper CheckDD(const TrimedResult<Char, Op, Na, Ia>& res, const ArgInfo& argInfo)
    {
        return CheckDD(res.ToStrArgInfo(), argInfo);
    }
    template<typename T, typename... Args>
    static NamedMapper CheckDS(const T& strInfo, Args&&...)
    {
        return CheckDD(strInfo, ArgInfo::ParseArgs<Args...>());
    }
    template<typename StrType>
    static NamedMapper CheckSD(StrType&& strInfo, const ArgInfo& argInfo)
    {
        return CheckDD(strInfo, argInfo);
    }
    template<typename StrType, typename... Args>
    static constexpr NamedMapper CheckSS(const StrType&, Args&&...)
    {
        constexpr StrType StrInfo;
        //const TrimedResult<OpCount, NamedArgCount, IdxArgCount>& cookie
        constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
        static_assert(ArgsInfo.IdxArgCount >= StrInfo.IdxArgCount, "No enough indexed arg");
        static_assert(ArgsInfo.NamedArgCount >= StrInfo.NamedArgCount, "No enough named arg");
        if constexpr (StrInfo.IdxArgCount > 0)
        {
            constexpr auto Index = GetIdxArgMismatch(StrInfo.IndexTypes, ArgsInfo.IndexTypes, StrInfo.IdxArgCount);
            CheckIdxArgMismatch<Index>();
        }
        NamedMapper mapper = { 0 };
        if constexpr (StrInfo.NamedArgCount > 0)
        {
            constexpr auto NamedRet = GetNamedArgMismatch(StrInfo.NamedTypes, StrInfo.FormatString, StrInfo.NamedArgCount,
                ArgsInfo.NamedTypes, ArgsInfo.Names, ArgsInfo.NamedArgCount);
            CheckNamedArgMismatch<NamedRet.AskIndex ? *NamedRet.AskIndex : ParseResultCommon::NamedArgSlots,
                NamedRet.GiveIndex ? *NamedRet.GiveIndex : ParseResultCommon::NamedArgSlots>();
            mapper = NamedRet.Mapper;
        }
        return mapper;
    }
};


template<typename Char>
struct Formatter;
struct FormatterBase
{
protected:
    template<typename T>
    SYSCOMMONAPI static uint32_t Execute(span<const uint8_t>& opcodes, T& executor, typename T::Context& context, uint32_t instCount = UINT32_MAX);
    template<typename Char>
    struct StaticExecutor;
public:
    template<typename Char>
    SYSCOMMONAPI static void FormatTo(Formatter<Char>& formatter, std::basic_string<Char>& ret, const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo, span<const uint16_t> argStore, const NamedMapper& mapping);
};

struct SpecReader
{
    friend FormatterBase;
private:
    struct Checker;
    const uint8_t* Ptr = nullptr;
    FormatSpec Spec;
    uint32_t SpecSize = 0;
    forceinline constexpr uint32_t ReadLengthedVal(uint32_t lenval, uint32_t val) noexcept
    {
        switch (lenval & 0b11u)
        {
        case 1:  val = Ptr[SpecSize]; SpecSize += 1; break;
        case 2:  val = Ptr[SpecSize + 0] | (Ptr[SpecSize + 1] << 8); SpecSize += 2; break;
        case 3:  val = Ptr[SpecSize + 0] | (Ptr[SpecSize + 1] << 8) | (Ptr[SpecSize + 2] << 16) | (Ptr[SpecSize + 3] << 24); SpecSize += 4; break;
        case 0:
        default: break;
        }
        return val;
    };
    forceinline constexpr void Reset(const uint8_t* ptr) noexcept
    {
        Ptr = ptr;
        SpecSize = 0;
    }
    [[nodiscard]] forceinline constexpr uint32_t EnsureSize() noexcept;
public:
    [[nodiscard]] forceinline constexpr const FormatSpec* ReadSpec() noexcept
    {
        if (!Ptr)
            return nullptr;
        if (!SpecSize)
        {
            const auto val0 = Ptr[SpecSize++];
            const auto val1 = Ptr[SpecSize++];
            const bool hasExtraFmt = val0 & 0b10000;
            Spec.TypeExtra = static_cast<uint8_t>(val0 >> 5);
            Spec.Alignment = static_cast<FormatSpec::Align>((val0 >> 2) & 0b11);
            Spec.SignFlag  = static_cast<FormatSpec::Sign> ((val0 >> 0) & 0b11);
            Spec.AlterForm = val1 & 0b10;
            Spec.ZeroPad   = val1 & 0b01;
            if (val1 & 0b11111100)
            {
                Spec.Fill       = ReadLengthedVal(val1 >> 6, ' ');
                Spec.Precision  = ReadLengthedVal(val1 >> 4, UINT32_MAX);
                Spec.Width      = static_cast<uint16_t>(ReadLengthedVal(val1 >> 2, 0));
            }
            else
            {
                Spec.Fill = ' ';
                Spec.Precision = UINT32_MAX;
                Spec.Width = 0;
            }
            IF_UNLIKELY(hasExtraFmt)
            {
                Spec.TypeExtra &= static_cast<uint8_t>(0x1u);
                Spec.FmtOffset  = static_cast<uint16_t>(ReadLengthedVal(val0 & 0x80 ? 2 : 1, 0));
                Spec.FmtLen     = static_cast<uint16_t>(ReadLengthedVal(val0 & 0x40 ? 2 : 1, 0));
            }
            else
                Spec.FmtOffset = Spec.FmtLen = 0;
        }
        return &Spec;
    }
};

template<typename Char>
struct CommonFormatter
{
    using StrType = std::basic_string<Char>;
public:
    SYSCOMMONAPI static void PutString(StrType& ret, std::   string_view str, const OpaqueFormatSpec* spec);
    SYSCOMMONAPI static void PutString(StrType& ret, std::  wstring_view str, const OpaqueFormatSpec* spec);
    SYSCOMMONAPI static void PutString(StrType& ret, std::u16string_view str, const OpaqueFormatSpec* spec);
    SYSCOMMONAPI static void PutString(StrType& ret, std::u32string_view str, const OpaqueFormatSpec* spec);
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    forceinline static void PutString(StrType& ret, std::u8string_view str, const OpaqueFormatSpec* spec)
    {
        PutString(ret, std::string_view{ reinterpret_cast<const char*>(str.data()), str.size() }, spec);
    }
#endif
    SYSCOMMONAPI static void PutInteger(StrType& ret, uint32_t val, bool isSigned, const OpaqueFormatSpec& spec);
    SYSCOMMONAPI static void PutInteger(StrType& ret, uint64_t val, bool isSigned, const OpaqueFormatSpec& spec);
    SYSCOMMONAPI static void PutFloat(StrType& ret, float  val, const OpaqueFormatSpec& spec);
    SYSCOMMONAPI static void PutFloat(StrType& ret, double val, const OpaqueFormatSpec& spec);
    SYSCOMMONAPI static void PutPointer(StrType& ret, uintptr_t val, const OpaqueFormatSpec& spec);
};
template<typename Char>
struct Formatter : public FormatterBase
{
    friend FormatterBase;
    using StrType = std::basic_string<Char>;
public:
    SYSCOMMONAPI virtual void PutColor(StrType& ret, ScreenColor color);
    SYSCOMMONAPI         void PutColorArg(StrType& ret, ScreenColor color, const FormatSpec* spec);
    SYSCOMMONAPI virtual void PutString(StrType& ret, std::   string_view str, const FormatSpec* spec);
    SYSCOMMONAPI virtual void PutString(StrType& ret, std::  wstring_view str, const FormatSpec* spec);
    SYSCOMMONAPI virtual void PutString(StrType& ret, std::u16string_view str, const FormatSpec* spec);
    SYSCOMMONAPI virtual void PutString(StrType& ret, std::u32string_view str, const FormatSpec* spec);
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    forceinline void PutString(StrType& ret, std::u8string_view str, const FormatSpec* spec)
    {
        PutString(ret, std::string_view{ reinterpret_cast<const char*>(str.data()), str.size() }, spec);
    }
#endif
    SYSCOMMONAPI virtual void PutInteger(StrType& ret, uint32_t val, bool isSigned, const FormatSpec* spec);
    SYSCOMMONAPI virtual void PutInteger(StrType& ret, uint64_t val, bool isSigned, const FormatSpec* spec);
    SYSCOMMONAPI virtual void PutFloat(StrType& ret, float  val, const FormatSpec* spec);
    SYSCOMMONAPI virtual void PutFloat(StrType& ret, double val, const FormatSpec* spec);
    SYSCOMMONAPI virtual void PutPointer(StrType& ret, uintptr_t val, const FormatSpec* spec);
    SYSCOMMONAPI virtual void PutDate(StrType& ret, std::basic_string_view<Char> fmtStr, const std::tm& date);
    SYSCOMMONAPI virtual void PutDateBase(StrType& ret, std::string_view fmtStr, const std::tm& date);
    template<typename T, typename... Args>
    forceinline void FormatToStatic(std::basic_string<Char>& dst, const T&, Args&&... args)
    {
        static_assert(std::is_same_v<typename std::decay_t<T>::CharType, Char>);
        static constexpr T StrInfo;
        // const auto strInfo = res.ToStrArgInfo();
        const auto mapping = ArgChecker::CheckSS(StrInfo, std::forward<Args>(args)...);
        static constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
        const auto argStore = ArgInfo::PackArgsStatic(std::forward<Args>(args)...);
        FormatterBase::FormatTo<Char>(*this, dst, StrInfo, ArgsInfo, argStore.ArgStore, mapping);
    }
    template<typename... Args>
    forceinline void FormatToDynamic(std::basic_string<Char>& dst, std::basic_string_view<Char> format, Args&&... args)
    {
        static constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
        const auto argStore = ArgInfo::PackArgsStatic(std::forward<Args>(args)...);
        FormatToDynamic_(dst, format, ArgsInfo, argStore.ArgStore);
    }
    template<typename T, typename... Args>
    forceinline std::basic_string<Char> FormatStatic(T&& res, Args&&... args)
    {
        std::basic_string<Char> ret;
        FormatToStatic(ret, std::forward<T>(res), std::forward<Args>(args)...);
        return ret;
    }
    template<typename... Args>
    forceinline std::basic_string<Char> FormatDynamic(std::basic_string_view<Char> format, Args&&... args)
    {
        std::basic_string<Char> ret;
        FormatToDynamic(ret, format, std::forward<Args>(args)...);
        return ret;
    }
private:
    SYSCOMMONAPI void FormatToDynamic_(std::basic_string<Char>& dst, std::basic_string_view<Char> format, const ArgInfo& argInfo, span<const uint16_t> argStore);
};


#define FmtString2(str_) []()                                                   \
{                                                                               \
    using FMT_P = common::str::FormatterParser;                                 \
    using FMT_C = std::decay_t<decltype(str_[0])>;                              \
    constexpr auto Data    = FMT_P::ParseString(str_);                          \
    constexpr auto OpCount = Data.OpCount;                                      \
    constexpr auto NACount = Data.NamedArgCount;                                \
    constexpr auto IACount = Data.IdxArgCount;                                  \
    [[maybe_unused]] constexpr auto Check =                                     \
        FMT_P::CheckErrorCompile<Data.ErrorPos, Data.ErrorNum>();               \
    using FMT_T = common::str::TrimedResult<FMT_C, OpCount, NACount, IACount>;  \
    struct FMT_Type : public FMT_T                                              \
    {                                                                           \
        using CharType = FMT_C;                                                 \
        constexpr FMT_Type() noexcept : FMT_T(FMT_P::ParseString(str_), str_) {}\
    };                                                                          \
    constexpr FMT_Type Dummy;                                                   \
    return Dummy;                                                               \
}()

#define FmtString(str_) *[]()                                       \
{                                                                   \
    using FMT_P = common::str::FormatterParser;                     \
    using FMT_R = common::str::ParseResultBase;                     \
    using FMT_C = std::decay_t<decltype(str_[0])>;                  \
    static constexpr auto Data = FMT_P::ParseString(str_);          \
    [[maybe_unused]] constexpr auto Check =                         \
        FMT_R::CheckErrorCompile<Data.ErrorPos, Data.ErrorNum>();   \
    using FMT_T = common::str::TrimedResult<FMT_C,                  \
        Data.OpCount, Data.NamedArgCount, Data.IdxArgCount>;        \
    struct FMT_Type : public FMT_T                                  \
    {                                                               \
        using CharType = FMT_C;                                     \
        constexpr FMT_Type() noexcept : FMT_T(Data, str_) {}        \
    };                                                              \
    static constexpr FMT_Type Dummy;                                \
    return &Dummy;                                                  \
}()

}

#if COMMON_COMPILER_GCC & COMMON_GCC_VER < 100000
#   pragma GCC diagnostic pop
#endif
#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic pop
#endif
