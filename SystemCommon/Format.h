#pragma once

#include "SystemCommonRely.h"
#include <optional>
#include <variant>

namespace common::str::exp
{


enum class ArgType : uint8_t
{
    Any = 0, String, Char, Integer, Float, Pointer, Time, 
    Custom = 0xf
};
MAKE_ENUM_BITFIELD(ArgType)


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
        InvalidType, ExtraFmtSpec, IncompType, InvalidFmt
    };
    template<uint16_t ErrorPos, uint16_t OpCount>
    static constexpr void CheckErrorCompile() noexcept
    {
        if constexpr (ErrorPos != UINT16_MAX)
        {
            constexpr auto err = static_cast<ErrorCode>(OpCount);
#define CHECK_ERROR_MSG(e, msg) static_assert(err != ErrorCode::e, msg)
            CHECK_ERROR_MSG(FmtTooLong,         "Format string too long");
            CHECK_ERROR_MSG(TooManyIdxArg,      "Too many IdxArg");
            CHECK_ERROR_MSG(TooManyNamedArg,    "Too many NamedArg");
            CHECK_ERROR_MSG(TooManyOp,          "Format string too complex with too many generated OP");
            CHECK_ERROR_MSG(MissingLeftBrace,   "Missing left brace");
            CHECK_ERROR_MSG(MissingRightBrace,  "Missing right brace");
            CHECK_ERROR_MSG(InvalidColor,       "Invalid color format");
            CHECK_ERROR_MSG(MissingColorFGBG,   "Missing color foreground/background identifier");
            CHECK_ERROR_MSG(InvalidCommonColor, "Invalid common color");
            CHECK_ERROR_MSG(Invalid8BitColor,   "Invalid 8bit color");
            CHECK_ERROR_MSG(Invalid24BitColor,  "Invalid 24bit color");
            CHECK_ERROR_MSG(InvalidArgIdx,      "Invalid index for IdxArg");
            CHECK_ERROR_MSG(ArgIdxTooLarge,     "Arg index is too large");
            CHECK_ERROR_MSG(InvalidArgName,     "Invalid name for NamedArg");
            CHECK_ERROR_MSG(ArgNameTooLong,     "Arg name is too long");
            CHECK_ERROR_MSG(WidthTooLarge,      "Width is too large");
            CHECK_ERROR_MSG(InvalidPrecision,   "Invalid format for precision");
            CHECK_ERROR_MSG(PrecisionTooLarge,  "Precision is too large");
            CHECK_ERROR_MSG(InvalidType,        "Invalid type specified for arg");
            CHECK_ERROR_MSG(ExtraFmtSpec,       "Unknown extra format spec left at the end");
            CHECK_ERROR_MSG(IncompType,         "In-compatible type being specified for the arg");
            CHECK_ERROR_MSG(InvalidFmt,         "Invalid format string");
#undef CHECK_ERROR_MSG
            static_assert(!AlwaysTrue2<OpCount>, "Unknown internal error");
        }
    }


    struct NamedArgType
    {
        uint16_t Offset = 0;
        uint8_t Length = 0;
        ArgType Type = ArgType::Any;
    };
    std::string_view FormatString;
    uint16_t ErrorPos = UINT16_MAX;
    uint8_t Opcodes[OpSlots] = { 0 };
    uint16_t OpCount = 0;
    NamedArgType NamedTypes[NamedArgSlots] = {};
    ArgType IndexTypes[IdxArgSlots] = { ArgType::Any };
    uint8_t NamedArgCount = 0, IdxArgCount = 0, NextArgIdx = 0;
    constexpr ParseResult& SetError(size_t pos, ErrorCode err) noexcept
    {
        ErrorPos = static_cast<uint16_t>(pos);
        OpCount = enum_cast(err);
        return *this;
    }
    
    static constexpr std::optional<ArgType> CheckCompatible(ArgType prev, ArgType cur) noexcept
    {
        if (cur  == ArgType::Any) return prev;
        if (prev == ArgType::Any) return cur;
        if (prev == ArgType::Integer) // can cast to detail type
        {
            if (cur == ArgType::Char || cur == ArgType::Pointer) return cur;
        }
        if (prev == cur) return prev; // need to be the same
        return {}; // incompatible
    }

    struct FormatSpec
    {
        enum class Align : uint8_t { None, Left, Right, Middle };
        enum class Sign  : uint8_t { None, Pos, Neg, Space };
        uint32_t Fill       = ' ';
        uint32_t Precision  = 0;
        uint16_t Width      = 0;
        char Type           = '\0';
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
        static constexpr uint8_t Length[2] = { 2,14 };
        static constexpr uint8_t Op = 0x80;
        static constexpr uint8_t FieldIndexed = 0x00;
        static constexpr uint8_t FieldNamed   = 0x20;
        static constexpr uint8_t FieldHasSpec = 0x10;
        static constexpr std::pair<uint8_t, uint8_t> EncodeValLen(uint32_t val) noexcept
        {
            if (val < UINT8_MAX)  return { (uint8_t)1, (uint8_t)1 };
            if (val < UINT16_MAX) return { (uint8_t)2, (uint8_t)2 };
            return { (uint8_t)3, (uint8_t)4 };
        }
        static constexpr std::pair<ArgType, uint8_t> EncodeSpec(const FormatSpec& spec, uint8_t(&output)[12]) noexcept
        {
            /*struct FormatSpec
            {
                uint32_t Fill;//2
                enum class Align : uint8_t { None, Left, Right, Middle };
                enum class Sign : uint8_t { None, Pos, Neg, Space };
                uint32_t Precision = 0;//2
                uint16_t Width = 0;//2
                char Type = '\0';//4
                Align Alignment = Align::None;//2
                Sign SignFlag = Sign::None;//2
                bool AlterForm = false;//1
                bool ZeroPad = false;//1
            };*/
            auto type = ArgType::Any;
            // type        ::=  "a" | "A" | "b" | "B" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "o" | "p" | "s" | "x" | "X"
            switch (spec.Type)
            {
            case 'g' : output[0] |= 0x00; type = ArgType::Float;   break;
            case 'G' : output[0] |= 0x10; type = ArgType::Float;   break;
            case 'a' : output[0] |= 0x20; type = ArgType::Float;   break;
            case 'A' : output[0] |= 0x30; type = ArgType::Float;   break;
            case 'e' : output[0] |= 0x40; type = ArgType::Float;   break;
            case 'E' : output[0] |= 0x50; type = ArgType::Float;   break;
            case 'f' : output[0] |= 0x60; type = ArgType::Float;   break;
            case 'F' : output[0] |= 0x70; type = ArgType::Float;   break;
            case 'd' : output[0] |= 0x00; type = ArgType::Integer; break;
            case 'b' : output[0] |= 0x10; type = ArgType::Integer; break;
            case 'B' : output[0] |= 0x20; type = ArgType::Integer; break;
            case 'o' : output[0] |= 0x30; type = ArgType::Integer; break;
            case 'x' : output[0] |= 0x40; type = ArgType::Integer; break;
            case 'X' : output[0] |= 0x50; type = ArgType::Integer; break;
            case 'c' : output[0] |= 0x80; type = ArgType::Char;    break;
            case 'p' : output[0] |= 0x00; type = ArgType::Pointer; break;
            case 's' : output[0] |= 0x00; type = ArgType::String;  break;
            case '\0': output[0] |= 0x00; type = ArgType::Any;     break;
            default: break; // should not happen
            }
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
            if (spec.AlterForm) 
                output[1] |= 0b10;
            if (spec.ZeroPad)
                output[1] |= 0b01;
            return { type, idx };
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
        using ArgIdType = std::variant<std::monostate, uint8_t, std::string_view>;
        static constexpr bool Emit(ParseResult& result, size_t offset, const ArgIdType& id,
            const FormatSpec* spec) noexcept
        {
            uint16_t opcnt = 2;
            uint8_t opcode = Op;
            uint8_t argIdx = 0;
            ArgType* dstType = nullptr;
            if (id.index() == 2)
            {
                const auto name = std::get<2>(id);
                for (uint8_t i = 0; i < result.NamedArgCount; ++i)
                {
                    auto& target = result.NamedTypes[i];
                    const auto targetName = result.FormatString.substr(target.Offset, target.Length);
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
                    target.Offset = static_cast<uint16_t>(name.data() - result.FormatString.data());
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
            uint8_t output[12] = { 0 }, tailopcnt = 0;
            auto type = ArgType::Any;
            if (spec)
            {
                opcode |= FieldHasSpec;
                const auto [type_, opcnt_] = EncodeSpec(*spec, output);
                type = type_;
                tailopcnt = opcnt_;
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
                result.NamedArgCount = std::max(argIdx, result.NamedArgCount);
            else
            {
                if (id.index() == 0) // need update next idx
                    ++result.NextArgIdx;
                result.IdxArgCount = std::max(static_cast<uint8_t>(argIdx + 1), std::max(result.NextArgIdx, result.IdxArgCount));
            }
            return true;
        }
    };
    static constexpr std::optional<uint8_t> ParseHex8bit(std::string_view hex) noexcept
    {
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
    template<typename T>
    static constexpr std::pair<size_t, bool> ParseDecTail(std::string_view dec, T& val, T limit) noexcept
    {
        for (size_t i = 1; i < dec.size(); ++i)
        {
            const auto ch = dec[i];
            if (ch >= '0' && ch <= '9')
                val = val * 10 + (ch - '0');
            else
                return { i, true };
            if (val >= limit)
                return { i, false };
        }
        return { SIZE_MAX, true };
    }
    static constexpr bool ParseColor(ParseResult& result, size_t pos, std::string_view str) noexcept
    {
        using namespace std::string_view_literals;
        if (str.size() <= 2) // at least 2 char needed
        {
            result.SetError(pos, ParseResult::ErrorCode::InvalidColor);
            return false;
        }
        const auto fgbg = str[0];
        if (fgbg != '<' && fgbg != '>')
        {
            result.SetError(pos, ParseResult::ErrorCode::MissingColorFGBG);
            return false;
        }
        const bool isFG = fgbg == '<';
        str.remove_prefix(1);
        if (str == "default")
            return ColorOp::EmitDefault(result, pos, isFG);
        std::optional<CommonColor> commonclr;
#define CHECK_COLOR_CASE(s, clr) if (str.size() >= s.size() && std::char_traits<char>::compare(str.data(), s.data(), s.size()) == 0)    \
    { commonclr = CommonColor::clr; str.remove_prefix(s.size()); }
             CHECK_COLOR_CASE("black"sv,    Black)
        else CHECK_COLOR_CASE("red"sv,      Red)
        else CHECK_COLOR_CASE("green"sv,    Green)
        else CHECK_COLOR_CASE("yellow"sv,   Yellow)
        else CHECK_COLOR_CASE("blue"sv,     Blue)
        else CHECK_COLOR_CASE("magenta"sv,  Magenta)
        else CHECK_COLOR_CASE("cyan"sv,     Cyan)
        else CHECK_COLOR_CASE("white"sv,    White)
#undef CHECK_COLOR_CASE
        if (commonclr)
        {
            if (!str.empty())
            {
                if (str.size() == 1 && str[0] == '+')
                    *commonclr |= CommonColor::BrightBlack; // make it bright
                else
                {
                    result.SetError(pos, ParseResult::ErrorCode::InvalidColor);
                    return false;
                }
            }
            return ColorOp::Emit(result, pos, *commonclr, isFG);
        }
        if (str[0] == 'b' && str.size() == 3)
        {
            const auto num = ParseHex8bit(str.substr(1));
            if (!num)
            {
                result.SetError(pos, ParseResult::ErrorCode::Invalid8BitColor);
                return false;
            }
            return ColorOp::Emit(result, pos, *num, isFG);
        }
        else if (str.size() == 6)
        {
            uint8_t rgb[3] = { 0, 0, 0 };
            for (uint8_t i = 0; i < 3; ++i)
            {
                const auto num = ParseHex8bit(str.substr(i * 2));
                if (!num)
                {
                    result.SetError(pos, ParseResult::ErrorCode::Invalid24BitColor);
                    return false;
                }
                rgb[i] = *num;
            }
            return ColorOp::Emit(result, pos, rgb[0], rgb[1], rgb[2], isFG);
        }
        result.SetError(pos, ParseResult::ErrorCode::InvalidColor);
        return false;
    }
    static constexpr void ParseFillAlign(FormatSpec& fmtSpec, std::string_view& str) noexcept
    {
        // [[fill]align]
        // fill        ::=  <a character other than '{' or '}'>
        // align       ::=  "<" | ">" | "^"
        char fillalign[2] = { str[0], 0 };
        if (str.size() >= 2)
            fillalign[1] = str[1];
        for (uint8_t i = 0; i < 2; ++i)
        {
            if (fillalign[i] == '<' || fillalign[i] == '>' || fillalign[i] == '^') // align
            {
                fmtSpec.Alignment = fillalign[i] == '<' ? FormatSpec::Align::Left :
                    (fillalign[i] == '>' ? FormatSpec::Align::Right : FormatSpec::Align::Middle);
                if (i == 1) // with fill
                    fmtSpec.Fill = fillalign[0];
                str.remove_prefix(i + 1);
                return;
            }
        }
        return;
    }
    static constexpr void ParseSign(FormatSpec& fmtSpec, std::string_view& str) noexcept
    {
        // sign        ::=  "+" | "-" | " "
             if (str[0] == '+') fmtSpec.SignFlag = FormatSpec::Sign::Pos;
        else if (str[0] == '-') fmtSpec.SignFlag = FormatSpec::Sign::Neg;
        else if (str[0] == ' ') fmtSpec.SignFlag = FormatSpec::Sign::Space;
        else return;
        str.remove_prefix(1);
        return;
    }
    static constexpr bool ParseWidth(ParseResult& result, FormatSpec& fmtSpec, size_t pos, std::string_view& str) noexcept
    {
        // width       ::=  integer // | "{" [arg_id] "}"
        const auto firstCh = str[0];
        if (firstCh > '0' && firstCh <= '9') // find width
        {
            uint32_t width = firstCh - '0'; 
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
    static constexpr bool ParsePrecision(ParseResult& result, FormatSpec& fmtSpec, size_t pos, std::string_view& str) noexcept
    {
        // ["." precision]
        // precision   ::=  integer // | "{" [arg_id] "}"
        if (str[0] == '.')
        {
            if (str.size() > 1)
            {
                str.remove_prefix(1);
                const auto firstCh = str[0];
                if (firstCh > '0' && firstCh <= '9') // find precision
                {
                    uint32_t precision = firstCh - '0';
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
    static constexpr bool ParseFormatSpec(ParseResult& result, FormatSpec& fmtSpec, const char* start, std::string_view str) noexcept
    {
        // format_spec ::=  [[fill]align][sign]["#"]["0"][width]["." precision][type]

        ParseFillAlign(fmtSpec, str);
        if (str.empty()) return true;
        ParseSign(fmtSpec, str);
        if (str.empty()) return true;
        if (str[0] == '#')
        {
            fmtSpec.AlterForm = true;
            str.remove_prefix(1);
        }
        if (str.empty()) return true;
        if (str[0] == '0')
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

        // type        ::=  "a" | "A" | "b" | "B" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "o" | "p" | "s" | "x" | "X"
        {
            constexpr std::string_view types{ "aAbBcdeEfFgGopsxX" };
            const char type = static_cast<char>(str[0]);
            if (types.find_first_of(type) == std::string_view::npos)
            {
                result.SetError(str.data() - start, ErrorCode::InvalidType);
                return false;
            }
            fmtSpec.Type = type;
            str.remove_prefix(1);
        }
        if (!str.empty())
        {
            result.SetError(str.data() - start, ErrorCode::ExtraFmtSpec);
            return false;
        }
        return true;
    }
    static constexpr ParseResult ParseString(const std::string_view str) noexcept
    {
        constexpr auto End = std::string_view::npos;
        ParseResult result;
        result.FormatString = str;
        size_t offset = 0;
        const auto size = str.size();
        if (size >= UINT16_MAX)
        {
            return result.SetError(0, ParseResult::ErrorCode::FmtTooLong);
        }
        while (offset < size)
        {
            const auto occur = str.find_first_of("{}", offset);
            bool isBrace = false;
            if (occur != End)
            {
                if (occur + 1 < size)
                {
                    isBrace = str[occur + 1] == str[occur]; // "{{" or "}}", emit single '{' or '}'
                }
                if (!isBrace && str[occur] == '}')
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
                    if (!BuiltinOp::EmitBrace(result, occur, str[occur] == '{'))
                        return result;
                    offset += 2; // eat brace "{{" or "}}"
                    continue;
                }
            }
            const auto argEnd = str.find_first_of('}', offset);
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
            const auto specSplit = argfmt.find_first_of(':');
            const auto argPart1 = specSplit == End ? argfmt : argfmt.substr(0, specSplit);
            const auto argPart2 = specSplit == End ? argfmt.substr(0, 0) : argfmt.substr(specSplit + 1);
            ArgOp::ArgIdType argId;
            if (!argPart1.empty())
            {
                // see https://fmt.dev/latest/syntax.html#formatspec
                // replacement_field ::=  "{" [arg_id] [":" (format_spec | chrono_format_spec)] "}"
                // arg_id            ::=  integer | identifier | color
                // integer           ::=  digit+
                // digit             ::=  "0"..."9"
                // identifier        ::=  id_start id_continue*
                // id_start          ::=  "a"..."z" | "A"..."Z" | "_"
                // id_continue       ::=  id_start | digit
                // color             ::=  "@" "<" | ">" id_start | digit
                const auto firstCh = argPart1[0];
                if (firstCh == '0')
                {
                    if (argPart1.size() != 0)
                        return result.SetError(offset, ParseResult::ErrorCode::InvalidArgIdx);
                    argId = { static_cast<uint8_t>(0) };
                }
                else if (firstCh > '0' && firstCh <= '9')
                {
                    uint32_t id = firstCh - '0';
                    const auto [errIdx, inRange] = ParseDecTail(argPart1, id, ParseResult::IdxArgSlots);
                    if (errIdx != SIZE_MAX)
                        return result.SetError(offset + errIdx, inRange ? ErrorCode::InvalidArgIdx : ErrorCode::ArgIdxTooLarge);
                    argId = { static_cast<uint8_t>(id) };
                }
                else if ((firstCh >= 'a' && firstCh <= 'z') || (firstCh >= 'A' && firstCh <= 'Z') || firstCh == '_')
                {
                    for (size_t i = 1; i < argPart1.size(); ++i)
                    {
                        const auto ch = argPart1[i];
                        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                            (ch >= '0' && ch <= '9') || ch == '_')
                            continue;
                        else
                            return result.SetError(offset + i, ParseResult::ErrorCode::InvalidArgName);
                    }
                    if (argPart1.size() > UINT8_MAX)
                        return result.SetError(offset, ParseResult::ErrorCode::ArgNameTooLong);
                    argId = { argPart1 };
                }
                else if (firstCh == '@') // Color
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
            if (!ArgOp::Emit(result, offset, argId, !argPart2.empty() ? &fmtSpec : nullptr)) // error
                return result;
            offset += 2 + argfmt.size(); // eat "{xxx}"
        }
        return result;
    }
};

template<uint8_t IdxArgCount>
struct IdxArgLimiter
{
    ArgType IndexTypes[IdxArgCount] = { ArgType::Any };
    constexpr IdxArgLimiter(const ArgType* type) noexcept
    {
        for (uint8_t i = 0; i < IdxArgCount; ++i)
            IndexTypes[i] = type[i];
    }
};
template<>
struct IdxArgLimiter<0>
{ 
    constexpr IdxArgLimiter(const ArgType*) noexcept {}
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
template<uint16_t OpCount>
struct OpHolder
{
    std::string_view FormatString;
    uint8_t Opcodes[OpCount] = { 0 };
    constexpr OpHolder(std::string_view str, const uint8_t* op) noexcept : FormatString(str)
    {
        for (uint16_t i = 0; i < OpCount; ++i)
            Opcodes[i] = op[i];
    }
};
template<>
struct OpHolder<0>
{
    std::string_view FormatString;
    constexpr OpHolder(std::string_view str, const uint8_t*) noexcept : FormatString(str)
    { }
};

template<uint16_t OpCount, uint8_t NamedArgCount, uint8_t IdxArgCount>
struct COMMON_EMPTY_BASES TrimedResult : public OpHolder<OpCount>, NamedArgLimiter<NamedArgCount>, IdxArgLimiter<IdxArgCount>
{
    constexpr TrimedResult(const ParseResult& result) noexcept :
        OpHolder<OpCount>(result.FormatString, result.Opcodes),
        NamedArgLimiter<NamedArgCount>(result.NamedTypes),
        IdxArgLimiter<IdxArgCount>(result.IndexTypes)
    { }
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

struct ArgResult
{
    std::string_view Names[ParseResult::NamedArgSlots] = {};
    uint8_t NamedTypes[ParseResult::NamedArgSlots] = { enum_cast(ArgType::Any) };
    uint8_t IndexTypes[ParseResult::IdxArgSlots] = { enum_cast(ArgType::Any) };
    uint8_t NamedArgCount = 0, IdxArgCount = 0;
    template<typename T>
    static constexpr uint8_t EncodeTypeSizeData() noexcept
    {
        switch (sizeof(T))
        {
        case 1:  return 0x00;
        case 2:  return 0x10;
        case 4:  return 0x20;
        case 8:  return 0x30;
        case 16: return 0x40;
        case 32: return 0x50;
        case 64: return 0x60;
        default: return 0xff;
        }
    }
    template<typename T>
    static constexpr uint8_t GetCharTypeData() noexcept
    {
        constexpr auto data = EncodeTypeSizeData<T>();
        static_assert(data <= 0x20);
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
    template<typename T>
    static constexpr uint8_t GetArgType() noexcept
    {
        using U = std::decay_t<T>;
        if constexpr (common::is_specialization<U, std::basic_string_view>::value || 
            common::is_specialization<U, std::basic_string>::value)
        {
            return GetCharTypeData<typename U::value_type>() | enum_cast(ArgType::String);
        }
        else if constexpr (CheckCharType<U>())
        {
            return GetCharTypeData<U>() | enum_cast(ArgType::Char);
        }
        else if constexpr (std::is_pointer_v<U>)
        {
            using X = std::decay_t<std::remove_pointer_t<U>>;
            if constexpr (CheckCharType<X>())
                return GetCharTypeData<X>() | enum_cast(ArgType::String);
            return uint8_t(std::is_same_v<X, void> ? 0x80 : 0x0) | enum_cast(ArgType::Pointer);
        }
        else if constexpr (std::is_floating_point_v<U>)
        {
            return EncodeTypeSizeData<U>() | enum_cast(ArgType::Float);
        }
        else if constexpr (std::is_integral_v<U>)
        {
            return uint8_t(std::is_unsigned_v<U> ? 0x80 : 0x0) | EncodeTypeSizeData<U>() | enum_cast(ArgType::Integer);
        }
        else
        {
            static_assert(!AlwaysTrue<T>, "unsupported type");
        }
    }
    template<typename T>
    static constexpr void ParseAnArg(ArgResult& result) noexcept
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
    static constexpr ArgResult ParseArgs() noexcept
    {
        ArgResult result;
        (..., ParseAnArg<Args>(result));
        return result;
    }
};

template<uint16_t OpCount, uint8_t NamedArgCount, uint8_t IdxArgCount, typename... Args>
std::string FormatSS(const TrimedResult<OpCount, NamedArgCount, IdxArgCount>& cookie, Args&&... args)
{
    std::string target; 
    target.reserve(cookie.FormatString.size());
    return target;
}

#define PasreFmtString(str) []()                                    \
{                                                                   \
    constexpr auto Result = ParseResult::ParseString(str);          \
    constexpr auto OpCount       = Result.OpCount;                  \
    constexpr auto NamedArgCount = Result.NamedArgCount;            \
    constexpr auto IdxArgCount   = Result.IdxArgCount;              \
    ParseResult::CheckErrorCompile<Result.ErrorPos, OpCount>();     \
    TrimedResult<OpCount, NamedArgCount, IdxArgCount> ret(Result);  \
    return ret;                                                     \
}()


}

