#pragma once

#include "SystemCommonRely.h"
#include <optional>
#include <variant>
#include <boost/container/small_vector.hpp>

namespace common::str::exp
{


enum class ArgDispType : uint8_t
{
    Any = 0, String, Char, Integer, Float, Pointer, Time, Numeric, Custom,
};
//- Any
//    - String
//    - Time
//    - Custom
//    - Numeric
//        - Integer
//            - Char
//            - Pointer
//        - Float
MAKE_ENUM_BITFIELD(ArgDispType)


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
        InvalidType, ExtraFmtSpec, IncompNumSpec, IncompType, InvalidFmt
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
            CHECK_ERROR_MSG(IncompNumSpec,      "Numeric spec applied on non-numeric type");
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
        ArgDispType Type = ArgDispType::Any;
    };
    std::string_view FormatString;
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
        enum class Align : uint8_t { None, Left, Right, Middle };
        enum class Sign  : uint8_t { None, Pos, Neg, Space };
        struct TypeIdentifier
        {
            ArgDispType Type  = ArgDispType::Any;
            uint8_t Extra = 0;
            constexpr TypeIdentifier() noexcept {}
            constexpr TypeIdentifier(char ch) noexcept
            {
                // type        ::=  "a" | "A" | "b" | "B" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "o" | "p" | "s" | "x" | "X"
                constexpr std::string_view types{ "gGaAeEfFdbBoxXcps" };
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
                else if (typeidx == 14)
                {
                    Type = ArgDispType::Char;
                    Extra = 0x80;
                }
                else if (typeidx == 15)
                {
                    Type = ArgDispType::Pointer;
                    Extra = 0x00;
                }
                else if (typeidx == 16)
                {
                    Type = ArgDispType::String;
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
        static constexpr uint8_t EncodeSpec(const FormatSpec& spec, uint8_t(&output)[12]) noexcept
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
            output[0] |= static_cast<uint8_t>(spec.Type.Extra << 4);
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
        using ArgIdType = std::variant<std::monostate, uint8_t, std::string_view>;
        static constexpr bool Emit(ParseResult& result, size_t offset, const ArgIdType& id,
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
        if (!str.empty())
        {
            fmtSpec.Type = { str[0] };
            str.remove_prefix(1);
        }
        if (!str.empty())
        {
            result.SetError(str.data() - start, ErrorCode::ExtraFmtSpec);
            return false;
        }
        if (fmtSpec.Type.Extra == 0xff)
        {
            result.SetError(str.data() - start, ErrorCode::InvalidType);
            return false;
        }
        // enhanced type check
        ArgDispType gneralType = ArgDispType::Any;
        if (fmtSpec.ZeroPad)
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
        fmtSpec.Type.Type = *newType;
        return true;
    }
    static constexpr ParseResult ParseString(const std::string_view str) noexcept
    {
        using namespace std::string_view_literals;
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
            const auto occur = str.find_first_of("{}"sv, offset);
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


struct StrArgInfo
{
    std::string_view FormatString;
    common::span<const uint8_t> Opcodes;
    common::span<const ArgDispType> IndexTypes;
    common::span<const ParseResult::NamedArgType> NamedTypes;
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

template<uint16_t OpCount_, uint8_t NamedArgCount_, uint8_t IdxArgCount_>
struct COMMON_EMPTY_BASES TrimedResult : public OpHolder<OpCount_>, NamedArgLimiter<NamedArgCount_>, IdxArgLimiter<IdxArgCount_>
{
    static constexpr uint16_t OpCount = OpCount_;
    static constexpr uint8_t NamedArgCount = NamedArgCount_;
    static constexpr uint8_t IdxArgCount = IdxArgCount_;
    constexpr TrimedResult(const ParseResult& result) noexcept :
        OpHolder<OpCount>(result.FormatString, result.Opcodes),
        NamedArgLimiter<NamedArgCount>(result.NamedTypes),
        IdxArgLimiter<IdxArgCount>(result.IndexTypes)
    { }
    constexpr operator StrArgInfo() const noexcept
    {
        common::span<const ArgDispType> idxTypes;
        if constexpr (IdxArgCount > 0)
            idxTypes = this->IndexTypes;
        common::span<const ParseResult::NamedArgType> namedTypes;
        if constexpr (NamedArgCount > 0)
            namedTypes = this->NamedTypes;
        return { this->FormatString, this->Opcodes, idxTypes, namedTypes };
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

enum class ArgRealType : uint8_t
{
    BaseTypeMask = 0xf0, SizeMask8 = 0b111, SizeMask4 = 0b11, SpanBit = 0b1000,
    Error = 0x00, Custom = 0x01, Ptr = 0x02, Bool = 0x04, PtrVoidBit = 0b1,
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

struct ArgInfo
{
    std::string_view Names[ParseResult::NamedArgSlots] = {};
    ArgRealType NamedTypes[ParseResult::NamedArgSlots] = { ArgRealType::Error };
    ArgRealType IndexTypes[ParseResult::IdxArgSlots] = { ArgRealType::Error };
    uint8_t NamedArgCount = 0, IdxArgCount = 0;
    static constexpr ArgRealType CleanRealType(ArgRealType type) noexcept
    {
        const auto base = type & ArgRealType::BaseTypeMask;
        switch (base)
        {
        case ArgRealType::SInt:
        case ArgRealType::UInt:
        case ArgRealType::Float:
        case ArgRealType::Char:
        case ArgRealType::String:
            return base;
        default:
            return type <= ArgRealType::SpecialMax ? type : ArgRealType::Error;
        }
    }
    template<typename T>
    static constexpr ArgRealType EncodeTypeSizeData() noexcept
    {
        switch (sizeof(T))
        {
        case 1:  return static_cast<ArgRealType>(0x0);
        case 2:  return static_cast<ArgRealType>(0x1);
        case 4:  return static_cast<ArgRealType>(0x2);
        case 8:  return static_cast<ArgRealType>(0x3);
        case 16: return static_cast<ArgRealType>(0x4);
        case 32: return static_cast<ArgRealType>(0x5);
        case 64: return static_cast<ArgRealType>(0x6);
        default: return static_cast<ArgRealType>(0xff);
        }
    }
    template<typename T>
    static constexpr ArgRealType GetCharTypeData() noexcept
    {
        constexpr auto data = EncodeTypeSizeData<T>();
        static_assert(enum_cast(data) <= 0x2);
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
                return (std::is_same_v<X, void> ? ArgRealType::PtrVoidBit : ArgRealType::Empty) | ArgRealType::Ptr;
        }
        else if constexpr (std::is_floating_point_v<U>)
        {
            return ArgRealType::Float | EncodeTypeSizeData<U>();
        }
        else if constexpr (std::is_integral_v<U>)
        {
            return (std::is_unsigned_v<U> ? ArgRealType::UInt : ArgRealType::SInt) | EncodeTypeSizeData<U>();
        }
        else if constexpr (std::is_same_v<U, bool>)
        {
            return ArgRealType::Bool;
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
    static void PackAnArg([[maybe_unused]] ArgPack& pack, [[maybe_unused]] T&& arg, [[maybe_unused]] uint16_t& idx) noexcept
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
                using X = std::decay_t<std::remove_pointer_t<U>>;
                if constexpr (CheckCharType<X>())
                {
                    pack.Put(std::pair{ reinterpret_cast<uintptr_t>(arg), std::char_traits<X>::length(arg) }, idx);
                }
                else
                    pack.Put(reinterpret_cast<uintptr_t>(arg), idx);
            }
            else if constexpr (std::is_floating_point_v<U>)
            {
                pack.Put(arg, idx);
            }
            else if constexpr (std::is_integral_v<U>)
            {
                pack.Put(arg, idx);
            }
            else if constexpr (std::is_same_v<U, bool>)
            {
                pack.Put(static_cast<uint8_t>(arg ? 1 : 0), idx);
            }
            else
            {
                static_assert(!AlwaysTrue<T>, "unsupported type");
            }
            idx++;
        }
    }
    template<typename T>
    static void PackAnNamedArg([[maybe_unused]] ArgPack& pack, [[maybe_unused]] T&& arg, uint16_t& idx) noexcept
    {
        if constexpr (std::is_base_of_v<NamedArgTag, T>)
            PackAnArg(pack, arg.Data, idx);
    }
    template<typename... Args>
    static ArgPack PackArgs(Args&&... args) noexcept
    {
        ArgPack pack;
        pack.Args.resize(sizeof...(Args));
        uint16_t argIdx = 0;
        (..., PackAnArg     (pack, std::forward<Args>(args), argIdx));
        (..., PackAnNamedArg(pack, std::forward<Args>(args), argIdx));
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
        const auto give = ArgInfo::CleanRealType(give_);
        if (give == ArgRealType::Error) return false;
        if (ask == ArgDispType::Any) return true;
        const auto isInteger = give == ArgRealType::SInt || give == ArgRealType::UInt || give == ArgRealType::Char || give == ArgRealType::Bool;
        switch (ask)
        {
        case ArgDispType::Integer:
            return isInteger || give == ArgRealType::Ptr;
        case ArgDispType::Float:
            return isInteger || give == ArgRealType::Float;
        case ArgDispType::Numeric:
            return isInteger || give == ArgRealType::Float || give == ArgRealType::Ptr;
        case ArgDispType::Char:
            return give == ArgRealType::Char || give == ArgRealType::Bool;
        case ArgDispType::String:
            return give == ArgRealType::String;
        case ArgDispType::Pointer:
            return give == ArgRealType::Ptr || (give == ArgRealType::String && HAS_FIELD(give, ArgRealType::StrPtrBit));
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
    static constexpr NamedCheckResult GetNamedArgMismatch(const ParseResult::NamedArgType* ask, const std::string_view str, uint8_t askCount,
        const ArgRealType* give, const std::string_view* giveNames, uint8_t giveCount) noexcept
    {
        NamedCheckResult ret;
        for (uint8_t i = 0; i < askCount; ++i)
        {
            const auto askName = str.substr(ask[i].Offset, ask[i].Length);
            bool found = false, match = false;
            uint8_t j = 0;
            for (; j < giveCount; ++j)
            {
                if (giveNames[j] == askName)
                {
                    found = true;
                    match = CheckCompatible(ask[i].Type, give[j]);
                    break;
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
    SYSCOMMONAPI static ArgPack::NamedMapper CheckDD(const StrArgInfo& strInfo, const ArgInfo& argInfo);
    template<typename... Args>
    static ArgPack::NamedMapper CheckDS(const StrArgInfo& strInfo, Args&&...)
    {
        return CheckDD(strInfo, ArgInfo::ParseArgs<Args...>());
    }
    template<typename StrType>
    static ArgPack::NamedMapper CheckSD(StrType&& strInfo, const ArgInfo& argInfo)
    {
        return CheckDD(strInfo, argInfo);
    }
    template<typename StrType, typename... Args>
    static ArgPack::NamedMapper CheckSS(StrType&&, Args&&...)
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

struct Formatter;
struct FormatterBase
{
protected:
    using Color = std::variant<std::monostate, uint8_t, std::array<uint8_t, 3>, CommonColor>;
    struct FormatSpec
    {
        enum class Align : uint8_t { None, Left, Right, Middle };
        enum class Sign  : uint8_t { None, Pos, Neg, Space };
        uint32_t Fill       = ' ';
        uint32_t Precision  = 0;
        uint16_t Width      = 0;
        uint8_t TypeExtra   = 0;
        Align Alignment     : 2;
        Sign SignFlag       : 2;
        bool AlterForm      : 2;
        bool ZeroPad        : 2;
        constexpr FormatSpec() noexcept : 
            Alignment(Align::None), SignFlag(Sign::None), AlterForm(false), ZeroPad(false) {}
    };
public:
    SYSCOMMONAPI static void FormatTo(const Formatter& formatter, std::string& ret, const StrArgInfo& strInfo, const ArgInfo& argInfo, const ArgPack& argPack);
};
struct Formatter : public FormatterBase
{
    friend FormatterBase;
private:
    /*virtual*/ void PutColor(std::string& ret, bool isBackground, Color color) const;
    /*virtual*/ void PutString(std::string& ret, std::   string_view str, const FormatSpec* spec) const;
    /*virtual*/ void PutString(std::string& ret, std::u16string_view str, const FormatSpec* spec) const;
    /*virtual*/ void PutString(std::string& ret, std::u32string_view str, const FormatSpec* spec) const;
public:
};


#define FmtString2(str) []()                                        \
{                                                                   \
    constexpr auto Result = ParseResult::ParseString(str);          \
    constexpr auto OpCount       = Result.OpCount;                  \
    constexpr auto NamedArgCount = Result.NamedArgCount;            \
    constexpr auto IdxArgCount   = Result.IdxArgCount;              \
    ParseResult::CheckErrorCompile<Result.ErrorPos, OpCount>();     \
    TrimedResult<OpCount, NamedArgCount, IdxArgCount> ret(Result);  \
    return ret;                                                     \
}
#define FmtString(str) []()                                                     \
{                                                                               \
    struct Type_ { const ParseResult Data = ParseResult::ParseString(str); };   \
    constexpr Type_ Result;                                                     \
    constexpr auto OpCount       = Result.Data.OpCount;                         \
    constexpr auto NamedArgCount = Result.Data.NamedArgCount;                   \
    constexpr auto IdxArgCount   = Result.Data.IdxArgCount;                     \
    ParseResult::CheckErrorCompile<Result.Data.ErrorPos, OpCount>();            \
    struct Type : public TrimedResult<OpCount, NamedArgCount, IdxArgCount>      \
    {                                                                           \
        constexpr Type() noexcept : TrimedResult(Type_{}.Data) {}               \
    };                                                                          \
    return Type{};                                                              \
}()

}

