#pragma once

#include "SystemCommonRely.h"
#include "FormatInclude.h"
#include "common/StrBase.hpp"
#include "common/RefHolder.hpp"
#include "common/SharedString.hpp"
#include <optional>
#include <variant>
#include <chrono>
#include <ctime>
#include <boost/container/small_vector.hpp>

namespace common::str
{


template<typename Char>
struct StrArgInfoCh;

struct ParseResult
{
    static constexpr uint8_t OpIdMask    = 0b11000000;
    static constexpr uint8_t OpFieldMask = 0b00110000;
    static constexpr uint8_t OpDataMask  = 0b00001111;
    static constexpr uint32_t OpSlots = 1024;
    static constexpr uint32_t IdxArgSlots = 64;
    static constexpr uint32_t NamedArgSlots = 16;
    enum class ErrorCode : uint16_t
    {
        Error = 0xff00, FmtTooLong, TooManyIdxArg, TooManyNamedArg, TooManyOp, 
        MissingLeftBrace, MissingRightBrace, 
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

    template<uint16_t ErrorPos, uint16_t OpCount>
    static constexpr bool CheckErrorCompile() noexcept
    {
        if constexpr (ErrorPos != UINT16_MAX)
        {
            constexpr auto err = static_cast<ErrorCode>(OpCount);
#define CHECK_ERROR_MSG(e, msg) static_assert(err != ErrorCode::e, msg)
            SCSTR_HANDLE_PARSE_ERROR(CHECK_ERROR_MSG)
#undef CHECK_ERROR_MSG
            static_assert(!AlwaysTrue2<OpCount>, "Unknown internal error");
            return false;
        }
        return true;
    }
    SYSCOMMONAPI static void CheckErrorRuntime(uint16_t errorPos, uint16_t opCount);


    struct NamedArgType
    {
        uint16_t Offset = 0;
        uint8_t Length = 0;
        ArgDispType Type = ArgDispType::Any;
    };
    //std::string_view FormatString;
    uint16_t ErrorPos = UINT16_MAX;
    uint8_t Opcodes[OpSlots] = { 0 };
    uint16_t OpCount = 0;
    NamedArgType NamedTypes[NamedArgSlots] = {};
    ArgDispType IndexTypes[IdxArgSlots] = { ArgDispType::Any };
    uint8_t NamedArgCount = 0, IdxArgCount = 0, NextArgIdx = 0;
    constexpr ParseResult& SetError(size_t pos, ErrorCode err) noexcept
    {
        ErrorPos = static_cast<uint16_t>(pos);
        OpCount = enum_cast(err);
        return *this;
    }
    
    static constexpr std::optional<ArgDispType> CheckCompatible(ArgDispType prev, ArgDispType cur) noexcept
    {
        if (prev == cur) return prev;
        if (cur  == ArgDispType::Any) return prev;
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

    struct FormatSpec
    {
        using Align = str::FormatSpec::Align;
        using Sign  = str::FormatSpec::Sign;
        struct TypeIdentifier
        {
            ArgDispType Type  = ArgDispType::Any;
            uint8_t Extra = 0;
            constexpr TypeIdentifier() noexcept {}
            constexpr TypeIdentifier(char ch) noexcept
            {
                // type        ::=  "a" | "A" | "b" | "B" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "o" | "p" | "s" | "x" | "X" | "T" | "@"
                constexpr std::string_view types{ "gGaAeEfFdbBoxXcpsT@" };
                const auto typeidx = types.find_first_of(ch);
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
                else
                {
                    if (ch != '\0')
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
        static constexpr bool EmitFmtStr(ParseResult& result, size_t offset, size_t length) noexcept
        {
            const auto isOffset16 = offset >= UINT8_MAX;
            const auto isLength16 = length >= UINT8_MAX;
            const auto opcnt = 1 + (isOffset16 ? 2 : 1) + (isLength16 ? 2 : 1);
            if (result.OpCount > ParseResult::OpSlots - opcnt)
            {
                result.SetError(offset + length, ParseResult::ErrorCode::TooManyOp);
                return false;
            }
            result.Opcodes[result.OpCount++] = Op | FieldFmtStr;
            result.Opcodes[result.OpCount++] = static_cast<uint8_t>(offset);
            if (isOffset16)
                result.Opcodes[result.OpCount++] = static_cast<uint8_t>(offset >> 8);
            result.Opcodes[result.OpCount++] = static_cast<uint8_t>(length);
            if (isLength16)
                result.Opcodes[result.OpCount++] = static_cast<uint8_t>(length >> 8);
            return true;
        }
        static constexpr bool EmitBrace(ParseResult& result, size_t offset, bool isLeft) noexcept
        {
            if (result.OpCount > ParseResult::OpSlots - 1)
            {
                result.SetError(offset, ParseResult::ErrorCode::TooManyOp);
                return false;
            }
            result.Opcodes[result.OpCount++] = Op | FieldBrace | uint8_t(isLeft ? 0x0 : 0x1);
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
        static constexpr bool Emit(ParseResult& result, size_t offset, CommonColor color, bool isForeground) noexcept
        {
            if (result.OpCount > ParseResult::OpSlots - 1)
            {
                result.SetError(offset, ParseResult::ErrorCode::TooManyOp);
                return false;
            }
            result.Opcodes[result.OpCount++] = Op | FieldCommon | (isForeground ? FieldForeground : FieldBackground) | enum_cast(color);
            return true;
        }
        static constexpr bool EmitDefault(ParseResult& result, size_t offset, bool isForeground) noexcept
        {
            if (result.OpCount > ParseResult::OpSlots - 1)
            {
                result.SetError(offset, ParseResult::ErrorCode::TooManyOp);
                return false;
            }
            result.Opcodes[result.OpCount++] = Op | FieldSpecial | (isForeground ? FieldForeground : FieldBackground) | DataDefault;
            return true;
        }
        static constexpr bool Emit(ParseResult& result, size_t offset, uint8_t color, bool isForeground) noexcept
        {
            if (color < 16) // use common color
                return Emit(result, offset, static_cast<CommonColor>(color), isForeground);
            if (result.OpCount > ParseResult::OpSlots - 2)
            {
                result.SetError(offset, ParseResult::ErrorCode::TooManyOp);
                return false;
            }
            result.Opcodes[result.OpCount++] = Op | FieldSpecial | (isForeground ? FieldForeground : FieldBackground) | DataBit8;
            result.Opcodes[result.OpCount++] = color;
            return true;
        }
        static constexpr bool Emit(ParseResult& result, size_t offset, uint8_t red, uint8_t green, uint8_t blue, bool isForeground) noexcept
        {
            if (result.OpCount > ParseResult::OpSlots - 4)
            {
                result.SetError(offset, ParseResult::ErrorCode::TooManyOp);
                return false;
            }
            result.Opcodes[result.OpCount++] = Op | FieldSpecial | (isForeground ? FieldForeground : FieldBackground) | DataBit24;
            result.Opcodes[result.OpCount++] = red;
            result.Opcodes[result.OpCount++] = green;
            result.Opcodes[result.OpCount++] = blue;
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
        static constexpr std::pair<uint8_t, uint8_t> EncodeValLen(uint32_t val) noexcept
        {
            if (val <= UINT8_MAX)  return { (uint8_t)1, (uint8_t)1 };
            if (val <= UINT16_MAX) return { (uint8_t)2, (uint8_t)2 };
            return { (uint8_t)3, (uint8_t)4 };
        }
        static constexpr uint8_t EncodeSpec(const FormatSpec& spec, uint8_t(&output)[SpecLength[1]]) noexcept
        {
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
            output[0] |= static_cast<uint8_t>(spec.Type.Extra << 5);
            output[0] |= static_cast<uint8_t>((enum_cast(spec.Alignment) & 0x3) << 2);
            output[0] |= static_cast<uint8_t>((enum_cast(spec.SignFlag)  & 0x3) << 0);
            uint8_t idx = 2;
            if (spec.Fill != ' ') // + 0~4
            {
                const auto [val, ele] = EncodeValLen(spec.Fill);
                if (ele == 1)
                    output[idx++] = static_cast<uint8_t>(spec.Fill);
                else if (ele == 2)
                {
                    output[idx++] = static_cast<uint8_t>(spec.Fill);
                    output[idx++] = static_cast<uint8_t>(spec.Fill >> 8);
                }
                else if (ele == 4)
                {
                    output[idx++] = static_cast<uint8_t>(spec.Fill);
                    output[idx++] = static_cast<uint8_t>(spec.Fill >> 8);
                    output[idx++] = static_cast<uint8_t>(spec.Fill >> 16);
                    output[idx++] = static_cast<uint8_t>(spec.Fill >> 24);
                }
                output[1] |= static_cast<uint8_t>(val << 6);
            }
            if (spec.Precision != 0) // + 0~4
            {
                const auto [val, ele] = EncodeValLen(spec.Precision);
                if (ele == 1)
                    output[idx++] = static_cast<uint8_t>(spec.Precision);
                else if (ele == 2)
                {
                    output[idx++] = static_cast<uint8_t>(spec.Precision);
                    output[idx++] = static_cast<uint8_t>(spec.Precision >> 8);
                }
                else if (ele == 4)
                {
                    output[idx++] = static_cast<uint8_t>(spec.Precision);
                    output[idx++] = static_cast<uint8_t>(spec.Precision >> 8);
                    output[idx++] = static_cast<uint8_t>(spec.Precision >> 16);
                    output[idx++] = static_cast<uint8_t>(spec.Precision >> 24);
                }
                output[1] |= static_cast<uint8_t>(val << 4);
            }
            if (spec.Width != 0) // + 0~2
            {
                const auto [val, ele] = EncodeValLen(spec.Width);
                if (ele == 1)
                    output[idx++] = static_cast<uint8_t>(spec.Width);
                else if (ele == 2)
                {
                    output[idx++] = static_cast<uint8_t>(spec.Width);
                    output[idx++] = static_cast<uint8_t>(spec.Width >> 8);
                }
                output[1] |= static_cast<uint8_t>(val << 2);
            }
            if (spec.FmtLen > 0) // + 0~4
            {
                output[0] |= static_cast<uint8_t>(0x10u);
                if (spec.FmtOffset <= UINT8_MAX)
                {
                    output[idx++] = static_cast<uint8_t>(spec.FmtOffset);
                }
                else
                {
                    output[idx++] = static_cast<uint8_t>(spec.FmtOffset);
                    output[idx++] = static_cast<uint8_t>(spec.FmtOffset >> 8);
                    output[0] |= static_cast<uint8_t>(0x80u);
                }
                if (spec.FmtLen <= UINT8_MAX)
                {
                    output[idx++] = static_cast<uint8_t>(spec.FmtLen);
                }
                else
                {
                    output[idx++] = static_cast<uint8_t>(spec.FmtLen);
                    output[idx++] = static_cast<uint8_t>(spec.FmtLen >> 8);
                    output[0] |= static_cast<uint8_t>(0x40u);
                }
            }
            if (spec.AlterForm) 
                output[1] |= 0b10;
            if (spec.ZeroPad)
                output[1] |= 0b01;
            return idx;
        }
        static constexpr bool EmitDefault(ParseResult& result, size_t offset) noexcept
        {
            if (result.OpCount > ParseResult::OpSlots - 2)
            {
                result.SetError(offset, ParseResult::ErrorCode::TooManyOp);
                return false;
            }
            if (result.NextArgIdx >= ParseResult::IdxArgSlots)
            {
                result.SetError(offset, ParseResult::ErrorCode::TooManyIdxArg);
                return false;
            }
            // don't modify argType
            // auto& argType = result.Types[result.ArgIdx];
            result.Opcodes[result.OpCount++] = Op | FieldIndexed;
            result.Opcodes[result.OpCount++] = result.NextArgIdx;
            result.IdxArgCount = std::max(++result.NextArgIdx, result.IdxArgCount);
            return true;
        }
        template<typename Char>
        using ArgIdType = std::variant<std::monostate, uint8_t, std::basic_string_view<Char>>;
        template<typename Char>
        static constexpr bool Emit(ParseResult& result, std::basic_string_view<Char> str, size_t offset, const ArgIdType<Char>& id,
            const FormatSpec* spec) noexcept
        {
            uint16_t opcnt = 2;
            uint8_t opcode = Op;
            uint8_t argIdx = 0;
            ArgDispType* dstType = nullptr;
            if (id.index() == 2)
            {
                const auto name = std::get<2>(id);
                for (uint8_t i = 0; i < result.NamedArgCount; ++i)
                {
                    auto& target = result.NamedTypes[i];
                    const auto targetName = str.substr(target.Offset, target.Length);
                    if (targetName == name)
                    {
                        argIdx = i;
                        dstType = &target.Type;
                        break;
                    }
                }
                if (!dstType)
                {
                    if (result.NamedArgCount >= ParseResult::NamedArgSlots)
                    {
                        result.SetError(offset, ParseResult::ErrorCode::TooManyNamedArg);
                        return false;
                    }
                    argIdx = result.NamedArgCount;
                    auto& target = result.NamedTypes[argIdx];
                    target.Offset = static_cast<uint16_t>(name.data() - str.data());
                    target.Length = static_cast<uint8_t>(name.size());
                    dstType = &target.Type;
                }
                opcode |= FieldNamed;
            }
            else
            {
                argIdx = id.index() == 0 ? result.NextArgIdx : std::get<1>(id);
                if (argIdx >= ParseResult::IdxArgSlots)
                {
                    result.SetError(offset, ParseResult::ErrorCode::TooManyIdxArg);
                    return false;
                }
                dstType = &result.IndexTypes[argIdx];
                opcode |= FieldIndexed;
            }
            uint8_t output[SpecLength[1]] = { 0 }, tailopcnt = 0;
            auto type = ArgDispType::Any;
            if (spec)
            {
                opcode |= FieldHasSpec;
                tailopcnt = EncodeSpec(*spec, output);
                type = spec->Type.Type;
            }
            opcnt += tailopcnt;
            if (result.OpCount > ParseResult::OpSlots - opcnt)
            {
                result.SetError(offset, ParseResult::ErrorCode::TooManyOp);
                return false;
            }
            const auto compType = CheckCompatible(*dstType, type);
            if (!compType)
            {
                result.SetError(offset, ParseResult::ErrorCode::IncompType);
                return false;
            }
            // update argType
            *dstType = *compType;
            // emit op
            result.Opcodes[result.OpCount++] = opcode;
            result.Opcodes[result.OpCount++] = argIdx;
            for (uint8_t i = 0; i < tailopcnt; ++i)
                result.Opcodes[result.OpCount++] = output[i];
            // update argIdx
            if (id.index() == 2)
                result.NamedArgCount = std::max(static_cast<uint8_t>(argIdx + 1), result.NamedArgCount);
            else
            {
                if (id.index() == 0) // need update next idx
                    ++result.NextArgIdx;
                result.IdxArgCount = std::max(static_cast<uint8_t>(argIdx + 1), std::max(result.NextArgIdx, result.IdxArgCount));
            }
            return true;
        }
    };
    
    template<typename Char>
    static constexpr auto ParseString(const std::basic_string_view<Char> str) noexcept;
    template<typename Char>
    static constexpr auto ParseString(const Char* str) noexcept
    {
        return ParseString(std::basic_string_view<Char>(str));
    }

    template<typename Char>
    constexpr StrArgInfoCh<Char> ToInfo(const std::basic_string_view<Char> str) const noexcept;
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
struct ParseResultCh : public ParseResult, public ParseLiterals<Char>
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
    //std::basic_string_view<Char> FormatString;
    static constexpr std::optional<uint8_t> ParseHex8bit(Char hex0, Char hex1) noexcept
    {
        Char hex[2] = { hex0, hex1 };
        uint32_t ret = 0;
        for (uint32_t i = 0; i < 2; ++i)
        {
                 if (hex[i] >= Char_0 && hex[i] <= Char_9) ret = ret * 16 + (hex[i] - Char_0);
            else if (hex[i] >= Char_a && hex[i] <= Char_f) ret = ret * 16 + (hex[i] - Char_a + 10);
            else if (hex[i] >= Char_A && hex[i] <= Char_F) ret = ret * 16 + (hex[i] - Char_A + 10);
            else return {};
        }
        return static_cast<uint8_t>(ret);
    }
    template<typename T>
    static constexpr std::pair<size_t, bool> ParseDecTail(std::basic_string_view<Char> dec, T& val, T limit) noexcept
    {
        for (size_t i = 1; i < dec.size(); ++i)
        {
            const auto ch = dec[i];
            if (ch >= Char_0 && ch <= Char_9)
                val = val * 10 + (ch - Char_0);
            else
                return { i, true };
            if (val >= limit)
                return { i, false };
        }
        return { SIZE_MAX, true };
    }
    static constexpr bool ParseColor(ParseResult& result, size_t pos, std::basic_string_view<Char> str) noexcept
    {
        using namespace std::string_view_literals;
        constexpr auto CommonColorMap = []() 
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
        if (str.size() < 2) // at least 2 char needed
        {
            result.SetError(pos, ParseResult::ErrorCode::InvalidColor);
            return false;
        }
        const auto fgbg = str[0];
        if (fgbg != Char_LT && fgbg != Char_GT)
        {
            result.SetError(pos, ParseResult::ErrorCode::MissingColorFGBG);
            return false;
        }
        const bool isFG = fgbg == Char_LT;
        if (str[1] == Char_Space)
        {
            if (str.size() == 2)
                return ColorOp::EmitDefault(result, pos, isFG);
        }
        else if (str[1] == Char_x)
        {
            if (str.size() == 4) // <xff
            {
                const auto num = ParseHex8bit(str[2], str[3]);
                if (!num)
                {
                    result.SetError(pos, ParseResult::ErrorCode::Invalid8BitColor);
                    return false;
                }
                return ColorOp::Emit(result, pos, *num, isFG);
            }
            else if (str.size() == 8) // <xffeedd
            {
                uint8_t rgb[3] = { 0, 0, 0 };
                for (uint8_t i = 0, j = 2; i < 3; ++i, j += 2)
                {
                    const auto num = ParseHex8bit(str[j], str[j + 1]);
                    if (!num)
                    {
                        result.SetError(pos, ParseResult::ErrorCode::Invalid24BitColor);
                        return false;
                    }
                    rgb[i] = *num;
                }
                return ColorOp::Emit(result, pos, rgb[0], rgb[1], rgb[2], isFG);
            }
        }
        else if (str.size() == 2)
        {
            uint8_t tmp = 0;
            bool isBright = false;
            if (str[1] >= Char_a && str[1] <= Char_z)
            {
                tmp = CommonColorMap[str[1] - Char_a];
            }
            else if (str[1] >= Char_A && str[1] <= Char_Z)
            {
                tmp = CommonColorMap[str[1] - Char_A];
                isBright = true;
            }
            if (tmp)
            {
                auto color = static_cast<CommonColor>(tmp & 0x7fu);
                if (isBright)
                    color |= CommonColor::BrightBlack; // make it bright
                return ColorOp::Emit(result, pos, color, isFG);
            }
        }
        result.SetError(pos, ParseResult::ErrorCode::InvalidColor);
        return false;
    }
    static constexpr void ParseFillAlign(FormatSpec& fmtSpec, std::basic_string_view<Char>& str) noexcept
    {
        // [[fill]align]
        // fill        ::=  <a character other than '{' or '}'>
        // align       ::=  "<" | ">" | "^"
        Char fillalign[2] = { str[0], 0 };
        if (str.size() >= 2)
            fillalign[1] = str[1];
        for (uint8_t i = 0; i < 2; ++i)
        {
            if (fillalign[i] == Char_LT || fillalign[i] == Char_GT || fillalign[i] == Char_UP) // align
            {
                fmtSpec.Alignment = fillalign[i] == Char_LT ? FormatSpec::Align::Left :
                    (fillalign[i] == Char_GT ? FormatSpec::Align::Right : FormatSpec::Align::Middle);
                if (i == 1) // with fill
                    fmtSpec.Fill = fillalign[0];
                str.remove_prefix(i + 1);
                return;
            }
        }
        return;
    }
    static constexpr void ParseSign(FormatSpec& fmtSpec, std::basic_string_view<Char>& str) noexcept
    {
        // sign        ::=  "+" | "-" | " "
             if (str[0] == Char_Plus)  fmtSpec.SignFlag = FormatSpec::Sign::Pos;
        else if (str[0] == Char_Minus) fmtSpec.SignFlag = FormatSpec::Sign::Neg;
        else if (str[0] == Char_Space) fmtSpec.SignFlag = FormatSpec::Sign::Space;
        else return;
        str.remove_prefix(1);
        return;
    }
    static constexpr bool ParseWidth(ParseResult& result, FormatSpec& fmtSpec, size_t pos, std::basic_string_view<Char>& str) noexcept
    {
        // width       ::=  integer // | "{" [arg_id] "}"
        const auto firstCh = str[0];
        if (firstCh > Char_0 && firstCh <= Char_9) // find width
        {
            uint32_t width = firstCh - Char_0;
            const auto [errIdx, inRange] = ParseDecTail<uint32_t>(str, width, UINT16_MAX);
            if (inRange)
            {
                fmtSpec.Width = static_cast<uint16_t>(width);
                if (errIdx == SIZE_MAX) // full finish
                    str.remove_prefix(str.size());
                else // assume successfully get a width
                    str.remove_prefix(errIdx);
            }
            else // out of range
            {
                result.SetError(pos + errIdx, ErrorCode::WidthTooLarge);
                return false;
            }
        }
        return true;
    }
    static constexpr bool ParsePrecision(ParseResult& result, FormatSpec& fmtSpec, size_t pos, std::basic_string_view<Char>& str) noexcept
    {
        // ["." precision]
        // precision   ::=  integer // | "{" [arg_id] "}"
        if (str[0] == Char_Dot)
        {
            if (str.size() > 1)
            {
                str.remove_prefix(1);
                const auto firstCh = str[0];
                if (firstCh > Char_0 && firstCh <= Char_9) // find precision
                {
                    uint32_t precision = firstCh - Char_0;
                    const auto [errIdx, inRange] = ParseDecTail<uint32_t>(str, precision, UINT32_MAX);
                    if (inRange)
                    {
                        fmtSpec.Precision = precision;
                        if (errIdx == SIZE_MAX) // full finish
                            str.remove_prefix(str.size());
                        else // assume successfully get a width
                            str.remove_prefix(errIdx);
                        return true;
                    }
                    else // out of range
                    {
                        result.SetError(pos + errIdx, ErrorCode::WidthTooLarge);
                        return false;
                    }
                }
            }
            result.SetError(pos + 1, ErrorCode::InvalidPrecision);
            return false;
        }
        return true;
    }
    static constexpr bool ParseFormatSpec(ParseResult& result, FormatSpec& fmtSpec, const Char* start, std::basic_string_view<Char> str) noexcept
    {
        // format_spec ::=  [[fill]align][sign]["#"]["0"][width]["." precision][type]

        ParseFillAlign(fmtSpec, str);
        if (str.empty()) return true;
        ParseSign(fmtSpec, str);
        if (str.empty()) return true;
        if (str[0] == Char_NumSign)
        {
            fmtSpec.AlterForm = true;
            str.remove_prefix(1);
        }
        if (str.empty()) return true;
        if (str[0] == Char_0)
        {
            fmtSpec.ZeroPad = true;
            str.remove_prefix(1);
        }
        if (str.empty()) return true;
        if (!ParseWidth(result, fmtSpec, str.data() - start, str))
            return false;
        if (str.empty()) return true;
        if (!ParsePrecision(result, fmtSpec, str.data() - start, str))
            return false;
        if (!str.empty())
        {
            const auto typestr = str[0];
            if (typestr <= static_cast<Char>(127))
            {
                fmtSpec.Type = { static_cast<char>(str[0]) };
                if (fmtSpec.Type.Extra != 0xff)
                    str.remove_prefix(1);
            }
        }
        if (fmtSpec.Type.Extra == 0xff)
        {
            result.SetError(str.data() - start, ErrorCode::InvalidType);
            return false;
        }
        // time type check
        if (fmtSpec.Type.Type == ArgDispType::Date)
        {
            if (fmtSpec.ZeroPad || fmtSpec.AlterForm || fmtSpec.Precision != 0 || fmtSpec.Width != 0 || fmtSpec.Fill != ' ' ||
                fmtSpec.SignFlag != FormatSpec::Sign::None || fmtSpec.Alignment != FormatSpec::Align::None)
            {
                result.SetError(str.data() - start, ErrorCode::IncompDateSpec);
                return false;
            }
        }
        else if (fmtSpec.Type.Type == ArgDispType::Color)
        {
            if (fmtSpec.ZeroPad || fmtSpec.Precision != 0 || fmtSpec.Width != 0 || fmtSpec.Fill != ' ' ||
                fmtSpec.SignFlag != FormatSpec::Sign::None)
            {
                result.SetError(str.data() - start, ErrorCode::IncompColorSpec);
                return false;
            }
        }
        // enhanced type check
        ArgDispType gneralType = ArgDispType::Any;
        if (fmtSpec.ZeroPad || fmtSpec.SignFlag != FormatSpec::Sign::None)
            gneralType = ArgDispType::Numeric;
        const auto newType = CheckCompatible(gneralType, fmtSpec.Type.Type);
        if (!newType)
        {
            if (gneralType == ArgDispType::Numeric)
                result.SetError(str.data() - start, ErrorCode::IncompNumSpec);
            else
                result.SetError(str.data() - start, ErrorCode::IncompType);
            return false;
        }
        // extra field handling
        if (fmtSpec.Type.Type == ArgDispType::Date || fmtSpec.Type.Type == ArgDispType::Custom)
        {
            if (!str.empty())
            {
                fmtSpec.FmtOffset = static_cast<uint16_t>(str.data() - start);
                fmtSpec.FmtLen = static_cast<uint16_t>(str.size());
            }
        }
        else if (!str.empty())
        {
            result.SetError(str.data() - start, ErrorCode::ExtraFmtSpec);
            return false;
        }
        fmtSpec.Type.Type = *newType;
        return true;
    }
    static constexpr ParseResult ParseString(const std::basic_string_view<Char> str) noexcept
    {
        using namespace std::string_view_literals;
        constexpr auto End = std::basic_string_view<Char>::npos;
        ParseResult result;
        size_t offset = 0;
        const auto size = str.size();
        if (size >= UINT16_MAX)
        {
            return result.SetError(0, ParseResult::ErrorCode::FmtTooLong);
        }
        while (offset < size)
        {
            const auto occur = str.find_first_of(LCH::BracePair, offset);
            bool isBrace = false;
            if (occur != End)
            {
                if (occur + 1 < size)
                {
                    isBrace = str[occur + 1] == str[occur]; // "{{" or "}}", emit single '{' or '}'
                }
                if (!isBrace && Char_RB == str[occur])
                    return result.SetError(occur, ParseResult::ErrorCode::MissingLeftBrace);
            }
            if (occur != offset) // need emit FmtStr
            {
                const auto strLen = (occur == End ? size : occur) - offset + (isBrace ? 1 : 0);
                if (!BuiltinOp::EmitFmtStr(result, offset, strLen))
                    return result;
                if (occur == End) // finish
                    return result;
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
                    if (!BuiltinOp::EmitBrace(result, occur, Char_LB == str[occur]))
                        return result;
                    offset += 2; // eat brace "{{" or "}}"
                    continue;
                }
            }
            const auto argEnd = str.find_first_of(Char_RB, offset);
            if (argEnd == End)
                return result.SetError(occur, ParseResult::ErrorCode::MissingRightBrace);

            // begin arg parsing
            const auto argfmt = str.substr(offset + 1, argEnd - offset - 1);
            if (argfmt.empty()) // "{}"
            {
                if (!ArgOp::EmitDefault(result, offset))
                    return result;
                offset += 2; // eat "{}"
                continue;
            }
            const auto specSplit = argfmt.find_first_of(Char_Colon);
            const auto argPart1 = specSplit == End ? argfmt : argfmt.substr(0, specSplit);
            const auto argPart2 = specSplit == End ? argfmt.substr(0, 0) : argfmt.substr(specSplit + 1);
            ArgOp::ArgIdType<Char> argId;
            if (!argPart1.empty())
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
                const auto firstCh = argPart1[0];
                if (Char_0 == firstCh)
                {
                    if (argPart1.size() != 1)
                        return result.SetError(offset, ParseResult::ErrorCode::InvalidArgIdx);
                    argId = { static_cast<uint8_t>(0) };
                }
                else if (firstCh > Char_0 && firstCh <= Char_9)
                {
                    uint32_t id = firstCh - Char_0;
                    const auto [errIdx, inRange] = ParseDecTail(argPart1, id, ParseResult::IdxArgSlots);
                    if (errIdx != SIZE_MAX)
                        return result.SetError(offset + errIdx, inRange ? ErrorCode::InvalidArgIdx : ErrorCode::ArgIdxTooLarge);
                    argId = { static_cast<uint8_t>(id) };
                }
                else if ((firstCh >= Char_a && firstCh <= Char_z) ||
                    (firstCh >= Char_A && firstCh <= Char_Z) || firstCh == Char_Under)
                {
                    for (size_t i = 1; i < argPart1.size(); ++i)
                    {
                        const auto ch = argPart1[i];
                        if ((ch >= Char_a && ch <= Char_z) || (ch >= Char_A && ch <= Char_Z) ||
                            (ch >= Char_0 && ch <= Char_9) || ch == Char_Under)
                            continue;
                        else
                            return result.SetError(offset + i, ParseResult::ErrorCode::InvalidArgName);
                    }
                    if (argPart1.size() > UINT8_MAX)
                        return result.SetError(offset, ParseResult::ErrorCode::ArgNameTooLong);
                    argId = { argPart1 };
                }
                else if (static_cast<Char>(Char_At) == firstCh) // Color
                {
                    if (!argPart2.empty())
                        return result.SetError(offset + 1, ParseResult::ErrorCode::InvalidColor);
                    if (!ParseColor(result, offset + 1, argPart1.substr(1)))
                        return result;
                    offset += 2 + argfmt.size(); // eat "{xxx}"
                    continue;
                }
                else
                    return result.SetError(offset, ParseResult::ErrorCode::InvalidArgName);
            }
            FormatSpec fmtSpec;
            if (!argPart2.empty())
            {
                if (!ParseFormatSpec(result, fmtSpec, str.data(), argPart2))
                    return result;
            }
            if (!ArgOp::Emit<Char>(result, str, offset, argId, !argPart2.empty() ? &fmtSpec : nullptr)) // error
                return result;
            offset += 2 + argfmt.size(); // eat "{xxx}"
        }
        return result;
    }
};

template<>
forceinline constexpr auto ParseResult::ParseString<char>(const std::basic_string_view<char> str) noexcept
{
    return ParseResultCh<char>::ParseString(str);
}
template<>
forceinline constexpr auto ParseResult::ParseString<char16_t>(const std::basic_string_view<char16_t> str) noexcept
{
    return ParseResultCh<char16_t>::ParseString(str);
}
template<>
forceinline constexpr auto ParseResult::ParseString<char32_t>(const std::basic_string_view<char32_t> str) noexcept
{
    return ParseResultCh<char32_t>::ParseString(str);
}
template<>
forceinline constexpr auto ParseResult::ParseString<wchar_t>(const std::basic_string_view<wchar_t> str) noexcept
{
    return ParseResultCh<wchar_t>::ParseString(str);
}
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template<>
forceinline constexpr auto ParseResult::ParseString<char8_t>(const std::basic_string_view<char8_t> str) noexcept
{
    return ParseResultCh<char8_t>::ParseString(str);
}
#endif


struct StrArgInfo
{
    common::span<const uint8_t> Opcodes;
    common::span<const ArgDispType> IndexTypes;
    common::span<const ParseResult::NamedArgType> NamedTypes;
};
template<typename Char>
struct StrArgInfoCh : public StrArgInfo
{
    using CharType = Char;
    std::basic_string_view<Char> FormatString;
};


template<typename Char>
forceinline constexpr StrArgInfoCh<Char> ParseResult::ToInfo(const std::basic_string_view<Char> str) const noexcept
{
    common::span<const ArgDispType> idxTypes;
    if (IdxArgCount > 0)
        idxTypes = { IndexTypes, IdxArgCount };
    common::span<const ParseResult::NamedArgType> namedTypes;
    if (NamedArgCount > 0)
        namedTypes = { NamedTypes, NamedArgCount };
    common::span<const uint8_t> opCodes = { Opcodes, OpCount };
    return { { opCodes, idxTypes, namedTypes }, str };
}


class COMMON_EMPTY_BASES DynamicTrimedResult : public FixedLenRefHolder<DynamicTrimedResult, uint32_t>
{
    friend RefHolder<DynamicTrimedResult>;
    friend FixedLenRefHolder<DynamicTrimedResult, uint32_t>;
private:
    static constexpr size_t NASize = sizeof(ParseResult::NamedArgType);
    static constexpr size_t IASize = sizeof(ArgDispType);
    [[nodiscard]] forceinline uintptr_t GetDataPtr() const noexcept
    {
        return Pointer;
    }
    forceinline void Destruct() noexcept { }
protected:
    uintptr_t Pointer = 0;
    uint32_t StrSize = 0;
    uint16_t OpCount = 0;
    uint8_t NamedArgCount = 0;
    uint8_t IdxArgCount = 0;
    SYSCOMMONAPI DynamicTrimedResult(const ParseResult& result, size_t strLength, size_t charSize);
    DynamicTrimedResult(const DynamicTrimedResult& other) noexcept :
        Pointer(other.Pointer), StrSize(other.StrSize), OpCount(other.OpCount), NamedArgCount(other.NamedArgCount), IdxArgCount(other.IdxArgCount)
    {
        this->Increase();
    }
    DynamicTrimedResult(DynamicTrimedResult&& other) noexcept :
        Pointer(other.Pointer), StrSize(other.StrSize), OpCount(other.OpCount), NamedArgCount(other.NamedArgCount), IdxArgCount(other.IdxArgCount)
    {
        other.Pointer = 0;
    }
    common::span<const uint8_t> GetOpcodes() const noexcept
    {
        return { reinterpret_cast<const uint8_t*>(GetStrPtr() + StrSize + IASize * IdxArgCount), OpCount };
    }
    common::span<const ArgDispType> GetIndexTypes() const noexcept
    {
        if (IdxArgCount > 0)
            return { reinterpret_cast<const ArgDispType*>(GetStrPtr() + StrSize), IdxArgCount };
        return {};
    }
    uintptr_t GetStrPtr() const noexcept
    {
        return Pointer + NASize * NamedArgCount;
    }
    common::span<const ParseResult::NamedArgType> GetNamedTypes() const noexcept
    {
        if (NamedArgCount > 0)
            return { reinterpret_cast<const ParseResult::NamedArgType*>(Pointer), NamedArgCount };
        return {};
    }
public:
    ~DynamicTrimedResult()
    {
        this->Decrease();
    }
};

template<typename Char>
class DynamicTrimedResultCh : public DynamicTrimedResult
{
public:
    DynamicTrimedResultCh(const ParseResult& result, std::basic_string_view<Char> str) :
        DynamicTrimedResult(result, str.size(), sizeof(Char))
    {
        const auto strptr = reinterpret_cast<Char*>(GetStrPtr());
        memcpy_s(strptr, StrSize, str.data(), sizeof(Char) * str.size());
        strptr[str.size()] = static_cast<Char>('\0');
    }
    DynamicTrimedResultCh(const DynamicTrimedResultCh& other) noexcept : DynamicTrimedResult(other) {}
    DynamicTrimedResultCh(DynamicTrimedResultCh&& other) noexcept : DynamicTrimedResult(other) {}
    constexpr operator StrArgInfoCh<Char>() const noexcept
    {
        return { { GetOpcodes(), GetIndexTypes(), GetNamedTypes() },
            { reinterpret_cast<Char*>(GetStrPtr()), StrSize / sizeof(Char) - 1 } };
    }
    constexpr StrArgInfoCh<Char> ToStrArgInfo() const noexcept
    {
        return *this;
    }
};


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
    ParseResult::NamedArgType NamedTypes[NamedArgCount] = {};
    constexpr NamedArgLimiter(const ParseResult::NamedArgType* type) noexcept
    {
        for (uint8_t i = 0; i < NamedArgCount; ++i)
            NamedTypes[i] = type[i];
    }
};
template<>
struct NamedArgLimiter<0>
{ 
    constexpr NamedArgLimiter(const ParseResult::NamedArgType*) noexcept {}
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
    constexpr TrimedResult(const ParseResult& result, std::basic_string_view<Char> fmtStr) noexcept :
        OpHolder<Char, OpCount>(fmtStr, result.Opcodes),
        NamedArgLimiter<NamedArgCount>(result.NamedTypes),
        IdxArgLimiter<IdxArgCount>(result.IndexTypes)
    { }
    constexpr operator StrArgInfoCh<Char>() const noexcept
    {
        common::span<const ArgDispType> idxTypes;
        if constexpr (IdxArgCount > 0)
            idxTypes = this->IndexTypes;
        common::span<const ParseResult::NamedArgType> namedTypes;
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

struct ArgPack
{
    using NamedMapper = std::array<uint8_t, ParseResult::NamedArgSlots>;
    boost::container::small_vector<uint16_t, 44> Args;
    NamedMapper Mapper;
    template<typename T>
    forceinline void Put(T arg, uint16_t idx) noexcept
    {
        constexpr auto NeedSize = sizeof(T);
        constexpr auto NeedSlot = (NeedSize + 1) / 2;
        const auto offset = Args.size();
        Args.resize(offset + NeedSlot);
        *reinterpret_cast<T*>(&Args[offset]) = arg;
        Args[idx] = static_cast<uint16_t>(offset);
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


template<typename T>
inline auto FormatAs(const T& arg);

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
template<typename Char>
inline auto FormatAs(const SharedString<Char>& arg)
{
    return static_cast<std::basic_string_view<Char>>(arg);
}


template<typename T>
inline auto FormatWith(const T& arg, FormatterExecutor& executor, FormatterExecutor::Context& context, const FormatSpec* spec);

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
    std::string_view Names[ParseResult::NamedArgSlots] = {};
    ArgRealType NamedTypes[ParseResult::NamedArgSlots] = { ArgRealType::Error };
    ArgRealType IndexTypes[ParseResult::IdxArgSlots] = { ArgRealType::Error };
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
    forceinline static void PackAnArg([[maybe_unused]] ArgPack& pack, [[maybe_unused]] T&& arg, [[maybe_unused]] uint16_t& idx) noexcept
    {
        if constexpr (!std::is_base_of_v<NamedArgTag, T>)
        {
            using U = std::decay_t<T>;
            if constexpr (common::is_specialization<U, std::basic_string_view>::value ||
                common::is_specialization<U, std::basic_string>::value)
            {
                pack.Put(std::pair{ reinterpret_cast<uintptr_t>(arg.data()), arg.size() }, idx);
            }
            else if constexpr (CheckCharType<U>())
            {
                pack.Put(arg, idx);
            }
            else if constexpr (std::is_pointer_v<U>)
            {
                pack.Put(reinterpret_cast<uintptr_t>(arg), idx);
            }
            else if constexpr (std::is_floating_point_v<U>)
            {
                pack.Put(arg, idx);
            }
            else if constexpr (std::is_same_v<U, bool>)
            {
                pack.Put(static_cast<uint16_t>(arg ? 1 : 0), idx);
            }
            else if constexpr (std::is_integral_v<U>)
            {
                pack.Put(arg, idx);
            }
            else if constexpr (std::is_same_v<U, CompactDate>)
            {
                pack.Put(arg, idx);
            }
            else if constexpr (std::is_same_v<U, std::tm>)
            {
                pack.Put(CompactDate(arg), idx);
            }
            else if constexpr (is_specialization<U, std::chrono::time_point>::value)
            {
                const auto deltaMs = std::chrono::duration_cast<std::chrono::duration<int64_t, std::milli>>(arg.time_since_epoch()).count();
                pack.Put(deltaMs, idx);
            }
            else if constexpr (CheckColorType<U>())
            {
                pack.Put(arg, idx);
            }
            else if constexpr (is_detected_v<detail::HasFormatAsMemFn, U>)
            {
                PackAnArg(pack, arg.FormatAs(), idx);
                return; // index already increased
            }
            else if constexpr (is_detected_v<detail::HasFormatAsSpeFn, U>)
            {
                PackAnArg(pack, FormatAs(arg), idx);
                return; // index already increased
            }
            else if constexpr (is_detected_v<detail::HasFormatWithMemFn, U>)
            {
                detail::FmtWithPair data{ &arg, [](const void* data, FormatterExecutor& executor, FormatterExecutor::Context& context, const FormatSpec* spec)
                    {
                        reinterpret_cast<const U*>(data)->FormatWith(executor, context, spec);
                    } };
                pack.Put(data, idx);
            }
            else if constexpr (is_detected_v<detail::HasFormatWithSpeFn, U>)
            {
                detail::FmtWithPair data{ &arg, [](const void* data, FormatterExecutor& executor, FormatterExecutor::Context& context, const FormatSpec* spec)
                    {
                        FormatWith(*reinterpret_cast<const U*>(data), executor, context, spec);
                    } };
                pack.Put(data, idx);
            }
            else
            {
                static_assert(!AlwaysTrue<T>, "unsupported type");
            }
            idx++;
        }
    }
    template<typename T>
    forceinline static void PackAnNamedArg([[maybe_unused]] ArgPack& pack, [[maybe_unused]] T&& arg, [[maybe_unused]] uint16_t& idx) noexcept
    {
        if constexpr (std::is_base_of_v<NamedArgTag, T>)
            PackAnArg(pack, arg.Data, idx);
    }
    template<typename... Ts>
    forceinline static ArgPack PackArgs(Ts&&... args) noexcept
    {
        constexpr auto ArgCount = sizeof...(Ts);
        ArgPack pack;
        pack.Args.resize(ArgCount);
        uint16_t argIdx = 0;
        (..., PackAnArg     (pack, std::forward<Ts>(args), argIdx));
        (..., PackAnNamedArg(pack, std::forward<Ts>(args), argIdx));
        return pack;
    }
};


struct ArgChecker
{
    struct NamedCheckResult
    {
        ArgPack::NamedMapper Mapper{ 0 };
        std::optional<uint8_t> AskIndex;
        std::optional<uint8_t> GiveIndex;
    };
    static constexpr bool CheckCompatible(ArgDispType ask, ArgRealType give_) noexcept
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
        /*case ArgDispType::Numeric:
            return isInteger || give == ArgRealType::Bool || give == ArgRealType::Float;*/
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
    static constexpr uint32_t GetIdxArgMismatch(const ArgDispType* ask, const ArgRealType* give, uint8_t count) noexcept
    {
        for (uint8_t i = 0; i < count; ++i)
            if (!CheckCompatible(ask[i], give[i]))
                return i;
        return ParseResult::IdxArgSlots;
    }
    template<uint32_t Index>
    static constexpr void CheckIdxArgMismatch() noexcept
    {
        static_assert(Index == ParseResult::IdxArgSlots, "Type mismatch");
    }
    template<typename Char>
    static constexpr NamedCheckResult GetNamedArgMismatch(const ParseResult::NamedArgType* ask, const std::basic_string_view<Char> fmtStr, uint8_t askCount,
        const ArgRealType* give, const std::string_view* giveNames, uint8_t giveCount) noexcept
    {
        NamedCheckResult ret;
        for (uint8_t i = 0; i < askCount; ++i)
        {
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
        static_assert(GiveIdx == ParseResult::NamedArgSlots, "Named arg type mismatch at [AskIdx, GiveIdx]");
        static_assert(AskIdx  == ParseResult::NamedArgSlots, "Missing named arg at [AskIdx]");
    }
    SYSCOMMONAPI static void CheckDDBasic(const StrArgInfo& strInfo, const ArgInfo& argInfo);
    template<typename Char>
    SYSCOMMONAPI static ArgPack::NamedMapper CheckDDNamedArg(const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo);
    template<typename Char>
    static ArgPack::NamedMapper CheckDD(const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo)
    {
        CheckDDBasic(strInfo, argInfo);
        return CheckDDNamedArg(strInfo, argInfo);
    }
    template<typename Char, uint16_t Op, uint8_t Na, uint8_t Ia>
    static ArgPack::NamedMapper CheckDD(const TrimedResult<Char, Op, Na, Ia>& res, const ArgInfo& argInfo)
    {
        return CheckDD(res.ToStrArgInfo(), argInfo);
    }
    template<typename T, typename... Args>
    static ArgPack::NamedMapper CheckDS(const T& strInfo, Args&&...)
    {
        return CheckDD(strInfo, ArgInfo::ParseArgs<Args...>());
    }
    template<typename StrType>
    static ArgPack::NamedMapper CheckSD(StrType&& strInfo, const ArgInfo& argInfo)
    {
        return CheckDD(strInfo, argInfo);
    }
    template<typename StrType, typename... Args>
    static constexpr ArgPack::NamedMapper CheckSS(const StrType&, Args&&...)
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
        ArgPack::NamedMapper mapper = { 0 };
        if constexpr (StrInfo.NamedArgCount > 0)
        {
            constexpr auto NamedRet = GetNamedArgMismatch(StrInfo.NamedTypes, StrInfo.FormatString, StrInfo.NamedArgCount,
                ArgsInfo.NamedTypes, ArgsInfo.Names, ArgsInfo.NamedArgCount);
            CheckNamedArgMismatch<NamedRet.AskIndex ? *NamedRet.AskIndex : ParseResult::NamedArgSlots,
                NamedRet.GiveIndex ? *NamedRet.GiveIndex : ParseResult::NamedArgSlots>();
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
    SYSCOMMONAPI static void FormatTo(Formatter<Char>& formatter, std::basic_string<Char>& ret, const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo, const ArgPack& argPack);
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
            if (hasExtraFmt)
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
        auto argPack = ArgInfo::PackArgs(std::forward<Args>(args)...);
        argPack.Mapper = mapping;
        FormatterBase::FormatTo<Char>(*this, dst, StrInfo, ArgsInfo, argPack);
    }
    template<typename... Args>
    forceinline void FormatToDynamic(std::basic_string<Char>& dst, std::basic_string_view<Char> format, Args&&... args)
    {
        static constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
        auto argPack = ArgInfo::PackArgs(std::forward<Args>(args)...);
        FormatToDynamic_(dst, format, ArgsInfo, argPack);
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
    SYSCOMMONAPI void FormatToDynamic_(std::basic_string<Char>& dst, std::basic_string_view<Char> format, const ArgInfo& argInfo, ArgPack& argPack);
};

template<typename Char, typename Fmter>
struct CombinedExecutor : public FormatterExecutor, protected Fmter
{
    static_assert(std::is_base_of_v<Formatter<Char>, Fmter>, "Fmter need to derive from Formatter<Char>");
    using CFmter = CommonFormatter<Char>;
    using CTX = FormatterExecutor::Context;
    struct Context : public CTX
    {
        std::basic_string<Char>& Dst;
        std::basic_string_view<Char> FmtStr;
        constexpr Context(std::basic_string<Char>& dst, std::basic_string_view<Char> fmtstr) noexcept : 
            Dst(dst), FmtStr(fmtstr) { }
    };

    void OnBrace(CTX& ctx, bool isLeft) final
    {
        auto& context = static_cast<Context&>(ctx);
        context.Dst.push_back(ParseLiterals<Char>::BracePair[isLeft ? 0 : 1]);
    }
    void OnColor(CTX& ctx, ScreenColor color) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutColor(context.Dst, color);
    }
    
    void PutString(CTX& ctx, ::std::   string_view str, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutString(context.Dst, str, &spec);
    }
    void PutString(CTX& ctx, ::std::u16string_view str, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutString(context.Dst, str, &spec);
    }
    void PutString(CTX& ctx, ::std::u32string_view str, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutString(context.Dst, str, &spec);
    }
    void PutInteger(CTX& ctx, uint32_t val, bool isSigned, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutInteger(context.Dst, val, isSigned, spec);
    }
    void PutInteger(CTX& ctx, uint64_t val, bool isSigned, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutInteger(context.Dst, val, isSigned, spec);
    }
    void PutFloat  (CTX& ctx, float  val, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutFloat(context.Dst, val, spec);
    }
    void PutFloat  (CTX& ctx, double val, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutFloat(context.Dst, val, spec);
    }
    void PutPointer(CTX& ctx, uintptr_t val, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutPointer(context.Dst, val, spec);
    }

    void PutString(CTX& ctx, std::   string_view str, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutString(context.Dst, str, spec);
    }
    void PutString(CTX& ctx, std::u16string_view str, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutString(context.Dst, str, spec);
    }
    void PutString(CTX& ctx, std::u32string_view str, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutString(context.Dst, str, spec);
    }
    void PutInteger(CTX& ctx, uint32_t val, bool isSigned, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutInteger(context.Dst, val, isSigned, spec);
    }
    void PutInteger(CTX& ctx, uint64_t val, bool isSigned, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutInteger(context.Dst, val, isSigned, spec);
    }
    void PutFloat  (CTX& ctx, float  val, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutFloat(context.Dst, val, spec);
    }
    void PutFloat  (CTX& ctx, double val, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutFloat(context.Dst, val, spec);
    }
    void PutPointer(CTX& ctx, uintptr_t val, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutPointer(context.Dst, val, spec);
    }
    void PutDate   (CTX& ctx, std::string_view fmtStr, const std::tm& date) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutDateBase(context.Dst, fmtStr, date);
    }
protected:
    using Fmter::PutString;
    using Fmter::PutInteger;
    using Fmter::PutFloat;
    using Fmter::PutPointer;
    using Fmter::PutDate;
private:
    void OnFmtStr(CTX& ctx, uint32_t offset, uint32_t length) final
    {
        auto& context = static_cast<Context&>(ctx);
        context.Dst.append(context.FmtStr.data() + offset, length);
    }
};

template<typename Char>
struct FormatterBase::StaticExecutor
{
    struct Context 
    {
        std::basic_string<Char>& Dst;
        const StrArgInfoCh<Char>& StrInfo;
        const ArgInfo& TheArgInfo;
        const ArgPack& TheArgPack;
    };
    Formatter<Char>& Fmter;
    template<typename T>
    forceinline static std::basic_string_view<T> BuildStr(uintptr_t ptr, size_t len) noexcept
    {
        const auto arg = reinterpret_cast<const T*>(ptr);
        if (len == SIZE_MAX)
            len = std::char_traits<T>::length(arg);
        return { arg, len };
    }
    forceinline void OnFmtStr(Context& context, uint32_t offset, uint32_t length)
    {
        context.Dst.append(context.StrInfo.FormatString.data() + offset, length);
    }
    forceinline void OnBrace(Context& context, bool isLeft)
    {
        context.Dst.push_back(ParseLiterals<Char>::BracePair[isLeft ? 0 : 1]);
    }
    forceinline void OnColor(Context& context, ScreenColor color)
    {
        Fmter.PutColor(context.Dst, color);
    }
    SYSCOMMONAPI void OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader);
};


#define FmtString2(str_) []()                                                   \
{                                                                               \
    using FMT_P = common::str::ParseResult;                                     \
    using FMT_C = std::decay_t<decltype(str_[0])>;                              \
    constexpr auto Data    = FMT_P::ParseString(str_);                          \
    constexpr auto OpCount = Data.OpCount;                                      \
    constexpr auto NACount = Data.NamedArgCount;                                \
    constexpr auto IACount = Data.IdxArgCount;                                  \
    [[maybe_unused]] constexpr auto Check =                                     \
        FMT_P::CheckErrorCompile<Data.ErrorPos, OpCount>();                     \
    using FMT_T = common::str::TrimedResult<FMT_C, OpCount, NACount, IACount>;  \
    struct FMT_Type : public FMT_T                                              \
    {                                                                           \
        using CharType = FMT_C;                                                 \
        constexpr FMT_Type() noexcept : FMT_T(FMT_P::ParseString(str_), str_) {}\
    };                                                                          \
    constexpr FMT_Type Dummy;                                                   \
    return Dummy;                                                               \
}()

#define FmtString(str_) *[]()                                                   \
{                                                                               \
    using FMT_P = common::str::ParseResult;                                     \
    using FMT_C = std::decay_t<decltype(str_[0])>;                              \
    static constexpr auto Data = FMT_P::ParseString(str_);                      \
    constexpr auto OpCount = Data.OpCount;                                      \
    constexpr auto NACount = Data.NamedArgCount;                                \
    constexpr auto IACount = Data.IdxArgCount;                                  \
    [[maybe_unused]] constexpr auto Check =                                     \
        FMT_P::CheckErrorCompile<Data.ErrorPos, OpCount>();                     \
    using FMT_T = common::str::TrimedResult<FMT_C, OpCount, NACount, IACount>;  \
    struct FMT_Type : public FMT_T                                              \
    {                                                                           \
        using CharType = FMT_C;                                                 \
        constexpr FMT_Type() noexcept : FMT_T(Data, str_) {}                    \
    };                                                                          \
    static constexpr FMT_Type Dummy;                                            \
    return &Dummy;                                                              \
}()

}

