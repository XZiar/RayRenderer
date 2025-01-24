#pragma once

#include "SystemCommonRely.h"
#include "FormatInclude.h"
#include "Date.h"
#include "common/StrBase.hpp"
#include "common/CompactPack.hpp"
#include "common/RefHolder.hpp"
#include "common/SharedString.hpp"
#include "common/ASCIIChecker.hpp"
#include <optional>
#include <ctime>

#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wassume"
#endif
#if COMMON_COMPILER_GCC && COMMON_GCC_VER < 100000
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wattributes"
#endif
#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4714)
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
        FillNotSingleCP, FillWithoutAlign, WidthTooLarge, InvalidPrecision, PrecisionTooLarge, InvalidCodepoint,
        InvalidType, ExtraFmtSpec, ExtraFmtTooLong, 
        IncompDateSpec, IncompTimeSpec, IncompColorSpec, IncompNumSpec, IncompType, InvalidFmt
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
    handler(FillNotSingleCP,    "Fill is not single codepoint"); \
    handler(FillWithoutAlign,   "Fill used without alignment"); \
    handler(WidthTooLarge,      "Width is too large"); \
    handler(InvalidPrecision,   "Invalid format for precision"); \
    handler(PrecisionTooLarge,  "Precision is too large"); \
    handler(InvalidCodepoint,   "Invalid codepoint"); \
    handler(InvalidType,        "Invalid type specified for arg"); \
    handler(ExtraFmtSpec,       "Unknown extra format spec left at the end"); \
    handler(ExtraFmtTooLong,    "Extra format string too long"); \
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

    static constexpr ASCIIChecker<true> NameArgTailChecker0 = std::string_view("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_");
    // [7...0] for bits [4,6]
    static constexpr auto UTF8MultiLenPack = BitsPackFrom<2>(
        0, // 1000
        0, // 1001
        0, // 1010
        0, // 1011
        1, // 1100
        1, // 1101
        2, // 1110
        3  // 1111
    );
    static constexpr auto UTF8MultiFirstMaskPack = BitsPackFrom<8>(
        0x7f, // 0yyyzzzz
        0x1f, // 110xxxyy
        0xf,  // 1110wwww
        0x7   // 11110uvv
    );
    static constexpr std::string_view AllowedFirstChar = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
    static constexpr auto AllowedFirstCharChecker = common::BitsPackFromIndexesArray<64>(AllowedFirstChar.data(), AllowedFirstChar.size(), -'A');

    struct FormatSpec
    {
        using Align = str::FormatSpec::Align;
        using Sign  = str::FormatSpec::Sign;
        struct TypeIdentifier
        {
            // type        ::=  "a" | "A" | "b" | "B" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "o" | "p" | "s" | "x" | "X" | "T" | "@"
            static constexpr std::string_view TypeChars = "gGaAeEfFdbBoxXcpsT@";
            static constexpr std::pair<char, char> TypeCharMinMax = common::GetMinMaxIn(TypeChars.data(), TypeChars.size());
            // high half for Extra, low half for Type
            static constexpr std::array<uint8_t, TypeCharMinMax.second - TypeCharMinMax.first + 1> TypePackedInfo = []() 
            {
                static_assert(enum_cast(ArgDispType::Any) == 0u);
                constexpr size_t Count = TypeCharMinMax.second - TypeCharMinMax.first + 1;
                std::array<uint8_t, Count> ret = { };
                for (uint32_t i = 0; i < Count; ++i)
                    ret[i] = 0xf0;
#define PutType(ch, type, extra) ret[ch - TypeCharMinMax.first] = static_cast<uint8_t>((enum_cast(ArgDispType::type) & 0xfu) | (static_cast<uint32_t>(extra) << 4))
                PutType('g', Float, 0);
                PutType('G', Float, 1);
                PutType('a', Float, 2);
                PutType('A', Float, 3);
                PutType('e', Float, 4);
                PutType('E', Float, 5);
                PutType('f', Float, 6);
                PutType('F', Float, 7);
                PutType('d', Integer, 0);
                PutType('b', Integer, 1);
                PutType('B', Integer, 2);
                PutType('o', Integer, 3);
                PutType('x', Integer, 4);
                PutType('X', Integer, 5);
                PutType('c', Char, 0);
                PutType('p', Pointer, 0);
                PutType('s', String, 0);
                PutType('T', Date, 0);
                PutType('@', Color, 0);
#undef PutType
                return ret;
            }();

            ArgDispType Type  = ArgDispType::Any;
            uint8_t Extra = 0;
            constexpr TypeIdentifier() noexcept {}
            forceinline constexpr TypeIdentifier(uint8_t ch) noexcept
            {
                IF_LIKELY(ch >= TypeCharMinMax.first && ch <= TypeCharMinMax.second)
                {
                    const auto info = TypePackedInfo[ch - TypeCharMinMax.first];
                    Type = static_cast<ArgDispType>(info & 0xfu);
                    Extra = static_cast<uint8_t>(static_cast<int8_t>(info) >> 4); // high bit arthmetical shift right leads to 0xff
                }
                else
                    Extra = 0xff;
            }
        };

        struct NonDefaultFlags
        {
            static constexpr uint8_t Fill = 0b1;
            static constexpr uint8_t Precision = 0b10;
            static constexpr uint8_t Width = 0b100;
            static constexpr uint8_t Alignment = 0b100;
            static constexpr uint8_t SignFlag = 0b1000;
            static constexpr uint8_t AlterForm = 0b10000;
            static constexpr uint8_t ZeroPad = 0b100000;
        };

        uint32_t Fill       = ' ';
        uint32_t Precision  = 0;
        uint16_t Width      = 0;
        uint16_t FmtOffset  = 0;
        uint8_t FmtLen      = 0;
        TypeIdentifier Type;
        Align Alignment     = Align::None;
        Sign SignFlag       = Sign::None;
        bool AlterForm      = false;
        bool ZeroPad        = false;
        uint8_t NonDefaultFlag = 0;
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
        static forceinline constexpr bool EmitFmtStr(T& result, uint32_t offset, uint32_t length) noexcept
        {
            const auto isOffset16 = offset >= UINT8_MAX;
            const auto isLength16 = length >= UINT8_MAX;
            const auto opcnt = 1 + (isOffset16 ? 2 : 1) + (isLength16 ? 2 : 1);
            auto space = result.ReserveSpace(offset, opcnt);
            IF_UNLIKELY(!space) return false;
            auto opval = Op | FieldFmtStr;
            if (isOffset16) opval |= DataOffset16;
            if (isLength16) opval |= DataLength16;
            *space++ = static_cast<uint8_t>(opval);
            if (common::is_constant_evaluated(true))
            {
                *space++ = static_cast<uint8_t>(offset);
                if (isOffset16)
                    *space++ = static_cast<uint8_t>(offset >> 8);
                *space++ = static_cast<uint8_t>(length);
                if (isLength16)
                    *space++ = static_cast<uint8_t>(length >> 8);
            }
            else
            {
                // Little Endian can simply copy, unused region remains 0, at lease 2B available initially
                *reinterpret_cast<uint16_t*>(space) = static_cast<uint16_t>(offset);
                IF_UNLIKELY(isOffset16)
                {
                    space += 2;
                }
                else
                {
                    space += 1;
                }
                IF_UNLIKELY(isLength16)
                {
                    *reinterpret_cast<uint16_t*>(space) = static_cast<uint16_t>(length);
                }
                else
                {
                    *space = static_cast<uint8_t>(length);
                }
            }
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
        static constexpr uint8_t SpecLength[2] = { 2,15 };
        static constexpr uint8_t Length[2] = { 2,17 };
        static constexpr uint8_t Op = 0x80;
        static constexpr uint8_t FieldIndexed = 0x00;
        static constexpr uint8_t FieldNamed   = 0x20;
        static constexpr uint8_t FieldHasSpec = 0x10;
        template<typename T>
        static forceinline constexpr uint8_t EncodeValLenTo(const T val, uint8_t* output, uint32_t& idx) noexcept
        {
            static_assert(std::is_unsigned_v<T>);
            static_assert(sizeof(T) <= sizeof(uint32_t));
            if (common::is_constant_evaluated(true))
            {
                output[idx++] = static_cast<uint8_t>(val);
                if (val <= UINT8_MAX)
                    return (uint8_t)1;
                output[idx++] = static_cast<uint8_t>(val >> 8);
                if constexpr (sizeof(T) > sizeof(uint16_t))
                {
                    if (val > UINT16_MAX)
                    {
                        output[idx++] = static_cast<uint8_t>(val >> 16);
                        output[idx++] = static_cast<uint8_t>(val >> 24);
                        return (uint8_t)3;
                    }
                }
                return (uint8_t)2;
            }
            else
            {
                // Little Endian can simply copy, unused region remains 0
                *reinterpret_cast<uint32_t*>(&output[idx]) = val;
                IF_LIKELY(val <= UINT8_MAX)
                {
                    idx += 1;
                    return (uint8_t)1;
                }
                else if (val <= UINT16_MAX)
                {
                    idx += 2;
                    return (uint8_t)2;
                }
                else
                {
                    idx += 4;
                    return (uint8_t)3;
                }
            }
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
            // spec0: 7.......5|...4....|3...2|1..0|
            // spec0: TypeExtra|ExtraFmt|Align|Sign|
            // spec1: ..7..|...6...|5.....4|3.....2|1......0|
            // spec1: Alter|ZeroPad|FillLen|PrecLen|WidthLen|
            CM_ASSUME(static_cast<uint8_t>(spec.Alignment) < 4);
            CM_ASSUME(static_cast<uint8_t>(spec.SignFlag) < 4);
            spec0 |= static_cast<uint8_t>(spec.Type.Extra << 5);
            spec0 |= static_cast<uint8_t>(enum_cast(spec.Alignment) << 2);
            spec0 |= static_cast<uint8_t>(enum_cast(spec.SignFlag) << 0);
            uint32_t idx = 2;
            IF_UNLIKELY(spec.Fill != ' ') // + 0~4
            {
                const auto val = EncodeValLenTo(spec.Fill, output, idx);
                spec1 |= static_cast<uint8_t>(val << 4);
            }
            IF_UNLIKELY(spec.Precision != 0) // + 0~4
            {
                const auto val = EncodeValLenTo(spec.Precision, output, idx);
                spec1 |= static_cast<uint8_t>(val << 2);
            }
            IF_UNLIKELY(spec.Width != 0) // + 0~2
            {
                const auto val = EncodeValLenTo(spec.Width, output, idx);
                spec1 |= val;
            }
            IF_UNLIKELY(spec.FmtLen > 0) // + 0~3
            {
                spec0 |= static_cast<uint8_t>(0x10u);
                if (common::is_constant_evaluated(true))
                {
                    output[idx++] = static_cast<uint8_t>(spec.FmtOffset);
                    if (spec.FmtOffset > UINT8_MAX)
                    {
                        output[idx++] = static_cast<uint8_t>(spec.FmtOffset >> 8);
                        spec0 |= static_cast<uint8_t>(0x80u);
                    }
                    output[idx++] = static_cast<uint8_t>(spec.FmtLen);
                }
                else
                {
                    // Little Endian can simply copy, unused region remains 0
                    *reinterpret_cast<uint16_t*>(&output[idx]) = spec.FmtOffset;
                    IF_UNLIKELY(spec.FmtOffset > UINT8_MAX)
                    {
                        idx += 2;
                        spec0 |= static_cast<uint8_t>(0x80u);
                    }
                    else
                    {
                        idx += 1;
                    }
                    output[idx++] = spec.FmtLen;
                }
            }
            if (spec.AlterForm) 
                spec1 |= 0x80;
            if (spec.ZeroPad)
                spec1 |= 0x40;
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

    // following parse does not care about char->uint32 negative convert --> >0x7f is mismatch anyway

    static forceinline constexpr std::optional<uint8_t> TryGetHex8bit(uint32_t hex0, uint32_t hex1) noexcept
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

    static forceinline constexpr FormatSpec::Align CheckAlign(uint32_t ch) noexcept
    {
        // align       ::=  "<" | ">" | "^"
        static_assert('<' == 0b00111100 && '>' == 0b00111110 && '^' == 0b01011110);
        // ch       : 0b00111100, 0b00111110, 0b01011110, 0b.abXXXc.
        const auto valid = (ch & (0xffffff00u | 0b10011101u)) == 0b00011100u;
        const uint32_t chUtf7 = static_cast<uint8_t>(ch);
        // chRS3    : 0b00000111, 0b00000111, 0b00001011, 0b000.abXX
        const auto chRS3 = chUtf7 >> 3;
        // chCombine: 0b00000100, 0b00000110, 0b00001010, 0b000.abc.
        const auto chCombine = chRS3 & chUtf7;
        CM_ASSUME(chCombine < 0b11111u);
        // shift2Cnt: 0b00000010, 0b00000011, 0b00000101 (2,3,5)
        constexpr auto AlignLUT = common::BitsPackFromIndexed<2, 8>(
            0b00000010u, FormatSpec::Align::Left,  // <
            0b00000011u, FormatSpec::Align::Right, // >
            0b00000101u, FormatSpec::Align::Middle // ^
        );
        static_assert(static_cast<uint32_t>(FormatSpec::Align::None) == 0u);
        // exceeded val will be 0 natually, extend for consteval
        const auto lutVal = (static_cast<uint32_t>(AlignLUT.Storage) >> chCombine) & 0x3u;
        return (common::is_constant_evaluated(true) ? valid : CM_UNPREDICT_BOOL(valid)) ? static_cast<FormatSpec::Align>(lutVal) : FormatSpec::Align::None;
    }

    static forceinline constexpr FormatSpec::Sign CheckSign(uint32_t ch) noexcept
    {
        // sign        ::=  "+" | "-" | " "
        static_assert('+' == 0b00101011 && '-' == 0b00101101 && ' ' == 0b00100000);
        // ch       : 0b00101011, 0b00101101, 0b00100000, 0b..X.abcd
        const auto valid = (ch & (0xffffff00u | 0b11110000u)) == 0b00100000u;
        // shiftCnt : 0b00001011, 0b00001101, 0b00000000 (2,3,5)
        constexpr auto SignLUT = common::BitsPackFromIndexed<2, 16>(
            0b00001011u, FormatSpec::Sign::Pos,  // +
            0b00001101u, FormatSpec::Sign::Neg,  // -
            0b00000000u, FormatSpec::Sign::Space //  
        );
        const auto lutVal = SignLUT.Get<FormatSpec::Sign>(ch & 0b00001111u);
        return (common::is_constant_evaluated(true) ? valid : CM_UNPREDICT_BOOL(valid)) ? lutVal : FormatSpec::Sign::None;
    }

    static forceinline constexpr bool TryPutFill([[maybe_unused]] ParseResultBase& result, FormatSpec& fmtSpec, [[maybe_unused]] uint32_t offset, uint32_t ch) noexcept
    {
        fmtSpec.Fill = ch;
        if (ch != ' ')
            fmtSpec.NonDefaultFlag |= FormatSpec::NonDefaultFlags::Fill;
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
    static_assert(Char_9 > Char_0 && Char_z > Char_a && Char_Z > Char_A);
    
    template<uint32_t Limit>
    static forceinline constexpr std::pair<uint32_t, bool> ParseDecTail(uint32_t& val, const Char* __restrict dec, uint32_t len) noexcept
    {
        constexpr uint32_t DigitCount = common::GetDigitCount(Limit, 10);
        static_assert(DigitCount > 1);
        constexpr auto limit1 = Limit / 10;
        constexpr auto limitLast = Limit % 10;
        CM_ASSUME(val < 10);

        // fully unrolled
        const uint32_t safecount = len > DigitCount - 1 ? DigitCount - 1 : len;
        for (uint32_t i = 1; i < safecount; ++i)
        {
            const uint32_t ch = dec[i];
            if (ch >= '0' && ch <= '9')
            {
                const auto num = ch - '0';
                val = val * 10 + num;
            }
            else
                return { i, true };
        }
        if (!(len > DigitCount - 1)) // reach to end of len, now i == len
            return { len, true };
        // reach to safe end, now i == DigitCount - 1
        {
            const uint32_t ch = dec[DigitCount - 1];
            if (ch >= '0' && ch <= '9')
            {
                const auto num = ch - '0';
                IF_UNLIKELY(val > limit1 || (val == limit1 && num > limitLast))
                {
                    return { DigitCount - 1 - 1, false };
                }
                val = val * 10 + num;
            }
            else
                return { DigitCount - 1, true };
        }
        // no more digit allowed
        if (DigitCount < len)
        {
            const uint32_t ch = dec[DigitCount];
            IF_UNLIKELY(ch >= '0' && ch <= '9')
                return { DigitCount - 1, false };
            else
                return { DigitCount, true };
        }
        else
            return { len, true };

        //// limit make sure [i] will not overflow, also expects str len <= UINT32_MAX
        //for (uint32_t i = 1; i < len; ++i)
        //{
        //    const uint32_t ch = dec[i];
        //    if (ch >= '0' && ch <= '9')
        //    {
        //        const auto num = ch - '0';
        //
        //        IF_UNLIKELY(val > limit1 || (val == limit1 && num > limitLast))
        //        {
        //            return { i - 1, false };
        //        }
        //        val = val * 10 + num;
        //    }
        //    else
        //        return { i, true };
        //}
        //return { UINT32_MAX, true };
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
                const auto num = TryGetHex8bit(str[3], str[4]);
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
                    const auto num = TryGetHex8bit(str[j], str[j + 1]);
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

    [[deprecated]] [[maybe_unused]] static forceinline constexpr uint32_t ParseFillAlignOld(FormatSpec& fmtSpec, const std::basic_string_view<Char>& str) noexcept
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

    static forceinline constexpr std::pair<uint32_t, bool> ReadWholeUtf32(const Char* const str, uint32_t& idx, const uint32_t len) noexcept
    {
        if constexpr (sizeof(Char) == 1) // utf8
        {
            const uint8_t ch0 = str[idx++];
            IF_LIKELY(ch0 < 0x80u)
            {
                return { ch0, true };
            }
            else
            {
                const auto byte0 = ch0 - 0x80u;
                const auto leftBytes = UTF8MultiLenPack.Get<uint16_t>(byte0 >> 4);
                IF_UNLIKELY(leftBytes == 0 || idx + leftBytes > len)
                {
                    return { idx - 1, false };
                }
                
                auto val = static_cast<uint32_t>(UTF8MultiFirstMaskPack.Get<uint8_t>(leftBytes) & ch0) << static_cast<uint8_t>(leftBytes * 6);
                switch (leftBytes)
                {
                case 3:
                {
                    const uint32_t ch = static_cast<uint8_t>(str[idx]);
                    const auto tmp = ch ^ 0b10000000u; // flip highest and keep following, valid will be 00xxxxxx, larger is invalid
                    IF_UNLIKELY(tmp > 0b00111111u)
                    {
                        return { idx, false };
                    }
                    idx++;
                    val |= tmp << 12;
                } [[fallthrough]];
                case 2:
                {
                    const uint32_t ch = static_cast<uint8_t>(str[idx]);
                    const auto tmp = ch ^ 0b10000000u; // flip highest and keep following, valid will be 00xxxxxx, larger is invalid
                    IF_UNLIKELY(tmp > 0b00111111u)
                    {
                        return { idx, false };
                    }
                    idx++;
                    val |= tmp << 6;
                } [[fallthrough]];
                case 1:
                {
                    const uint32_t ch = static_cast<uint8_t>(str[idx]);
                    const auto tmp = ch ^ 0b10000000u; // flip highest and keep following, valid will be 00xxxxxx, larger is invalid
                    IF_UNLIKELY(tmp > 0b00111111u)
                    {
                        return { idx, false };
                    }
                    idx++;
                    val |= tmp;
                } break;
                default: CM_UNREACHABLE(); break;
                }
                return { val, true };
            }
        }
        else if constexpr (sizeof(Char) == 2) // utf16
        {
            const uint16_t first = str[idx++];
            IF_UNLIKELY(first >= 0xd800u && first <= 0xdfffu) // surrogates
            {
                IF_UNLIKELY(first >= 0xdc00u || idx == len)
                {
                    return { idx - 1, false };
                }
                const uint16_t second = str[idx];
                IF_LIKELY(second >= 0xdc00u && second <= 0xdfffu)
                {
                    ++idx;
                    uint32_t val = (((first & 0x3ffu) << 10) | (second & 0x3ffu)) + 0x10000u;
                    return { val, true };
                }
                else
                {
                    return { idx, false };
                }
            }
            else
            {
                return { first, true };
            }
        }
        else if constexpr (sizeof(Char) == 4) // utf32
        {
            return { str[idx++], true };
        }
        else
            static_assert(!AlwaysTrue<Char>);
    }

    static forceinline constexpr bool ParseFillAlign(ParseResultBase& result, FormatSpec& fmtSpec, const Char* const str, uint32_t& idx, const uint32_t offset, const uint32_t len) noexcept
    {
        // [[fill]align]
        // fill        ::=  <a character other than '{' or '}'>
        // align       ::=  "<" | ">" | "^"
        if (len > 1)
        {
            if (const auto align = CheckAlign(str[1]); align != FormatSpec::Align::None) // str[0] must be Fill
            {
                // quick utf32 read
                uint32_t fill = UINT32_MAX;
                if constexpr (sizeof(Char) == 1) // utf8
                {
                    const auto val = static_cast<uint8_t>(str[0]);
                    IF_UNLIKELY(val >= 0x80u) // multi
                    {
                        result.SetError(offset, ParseResultBase::ErrorCode::InvalidCodepoint);
                        return false;
                    }
                    fill = val;
                }
                else if constexpr (sizeof(Char) == 2) // utf16
                {
                    const auto val = static_cast<uint16_t>(str[0]);
                    IF_UNLIKELY(val >= 0xd800u && val <= 0xdfffu) // surrogates
                    {
                        result.SetError(offset, ParseResultBase::ErrorCode::InvalidCodepoint);
                        return false;
                    }
                    fill = val;
                }
                else if constexpr (sizeof(Char) == 4) // utf32
                {
                    fill = str[0];
                }
                else
                    static_assert(!AlwaysTrue<Char>);
                IF_UNLIKELY(!TryPutFill(result, fmtSpec, offset, fill))
                    return false;
                fmtSpec.Alignment = align;
                fmtSpec.NonDefaultFlag |= FormatSpec::NonDefaultFlags::Alignment;
                idx += 2;
                return true;
            }

            constexpr ASCIIChecker<true> SpecChecker("<>^+- #0123456789.gGaAeEfFdbBoxXcpsT@");
            if (SpecChecker(str[0])) // str[0] match a spec, but str[1] is not align, str[0] must not be fill and there must be no fill
            {
                if (const auto align = CheckAlign(str[0]); align != FormatSpec::Align::None)
                {
                    fmtSpec.Alignment = align;
                    fmtSpec.NonDefaultFlag |= FormatSpec::NonDefaultFlags::Alignment;
                    idx++;
                }
                return true;
            }
            // str[0] is possible fill, consider utf32 read, this utf32 is not going to be a spec
            // for better error report, try to read whole utf32 anyway even with current fmt 16bit limitation
            const auto [ch, suc] = ReadWholeUtf32(str, idx, len);
            IF_UNLIKELY(!suc)
            {
                result.SetError(offset + ch, ParseResultBase::ErrorCode::InvalidCodepoint);
                return false;
            }
            const auto align = idx == len ? FormatSpec::Align::None : CheckAlign(str[idx]);
            IF_UNLIKELY(align == FormatSpec::Align::None)
            {
                result.SetError(offset, ParseResultBase::ErrorCode::FillWithoutAlign);
                return false;
            }
            IF_UNLIKELY(!TryPutFill(result, fmtSpec, offset, ch))
                return false;
            fmtSpec.Alignment = align;
            fmtSpec.NonDefaultFlag |= FormatSpec::NonDefaultFlags::Alignment;
            idx++;
            return true;
        }
        else // must not have fill
        {
            if (const auto align = CheckAlign(str[0]); align != FormatSpec::Align::None)
            {
                // only align
                fmtSpec.Alignment = align;
                fmtSpec.NonDefaultFlag |= FormatSpec::NonDefaultFlags::Alignment;
                idx++;
            }
            return true;
        }
    }

    // 0 for failed, 1 for read a num, 2 for not a num
    static forceinline constexpr uint32_t ParseWidth(ParseResultBase& result, FormatSpec& fmtSpec, const uint32_t firstCh, const Char* str, uint32_t& idx, const uint32_t offset, const uint32_t len) noexcept
    {
        // width       ::=  integer // | "{" [arg_id] "}"
        if (firstCh > '0' && firstCh <= '9') // find width
        {
            uint32_t width = firstCh - '0';
            const auto [errIdx, inRange] = ParseDecTail<UINT16_MAX>(width, &str[idx], len - idx);
            IF_LIKELY(inRange)
            {
                CM_ASSUME(width > 0);
                fmtSpec.Width = static_cast<uint16_t>(width);
                fmtSpec.NonDefaultFlag |= FormatSpec::NonDefaultFlags::Width;
                idx += errIdx;
                return 1;
            }
            ELSE_UNLIKELY // out of range
            {
                result.SetError(offset + idx + errIdx, ParseResultBase::ErrorCode::WidthTooLarge);
                return 0;
            }
        }
        return 2;
    }

    // 0 for failed, 1 for read a num, 2 for not a num
    static forceinline constexpr uint32_t ParsePrecision(ParseResultBase& result, FormatSpec& fmtSpec, const Char ch, const Char* str, uint32_t& idx, const uint32_t offset, const uint32_t len) noexcept
    {
        // ["." precision]
        // precision   ::=  integer // | "{" [arg_id] "}"
        if (ch == Char_Dot)
        {
            IF_LIKELY(++idx < len)
            {
                const uint32_t firstCh = str[idx];
                IF_LIKELY(firstCh > '0' && firstCh <= '9') // find precision
                {
                    uint32_t precision = firstCh - '0';
                    const auto [errIdx, inRange] = ParseDecTail<UINT32_MAX>(precision, &str[idx], len - idx);
                    IF_LIKELY(inRange)
                    {
                        CM_ASSUME(precision > 0);
                        fmtSpec.Precision = precision;
                        fmtSpec.NonDefaultFlag |= FormatSpec::NonDefaultFlags::Precision;
                        idx += errIdx;
                        return 1;
                    }
                    ELSE_UNLIKELY // out of range
                    {
                        result.SetError(offset + idx + errIdx, ParseResultBase::ErrorCode::WidthTooLarge);
                        return 0;
                    }
                }
            }
            result.SetError(offset + idx, ParseResultBase::ErrorCode::InvalidPrecision);
            return 0;
        }
        return 2;
    }

    static forceinline constexpr bool ParseFormatSpec(ParseResultBase& result, FormatSpec& fmtSpec, const Char* str, const uint32_t offset, /*to avoid using r64 on x86*/const uint32_t len) noexcept
    {
        // format_spec ::=  [[fill]align][sign]["#"]["0"][width]["." precision][type]
        fmtSpec = {}; // clean up
        uint32_t idx = 0;

        IF_UNLIKELY(!ParseFillAlign(result, fmtSpec, str, idx, offset, len))
            return false;
        if (idx == len) return true;
        
        Char ch = str[idx];
        if (const auto sign = CheckSign(ch); sign != FormatSpec::Sign::None)
        {
            fmtSpec.SignFlag = sign;
            fmtSpec.NonDefaultFlag |= FormatSpec::NonDefaultFlags::SignFlag;
            fmtSpec.Type.Type = ArgDispType::Numeric;
            if (++idx == len) return true;
            ch = str[idx];
        }
        
        if (ch == Char_NumSign)
        {
            fmtSpec.AlterForm = true;
            fmtSpec.NonDefaultFlag |= FormatSpec::NonDefaultFlags::AlterForm;
            if (++idx == len) return true;
            ch = str[idx];
        }
        
        uint32_t chOffset0 = ch - Char_0;
        if (chOffset0 == 0) // '0'
        {
            fmtSpec.ZeroPad = true;
            fmtSpec.NonDefaultFlag |= FormatSpec::NonDefaultFlags::ZeroPad;
            fmtSpec.Type.Type = ArgDispType::Numeric;
            if (++idx == len) return true;
            ch = str[idx];
            chOffset0 = ch - Char_0;
        }

        // width       ::=  integer // | "{" [arg_id] "}"
        if (chOffset0 <= 9) // number, find width
        {
            uint32_t width = chOffset0;
            const auto [errIdx, inRange] = ParseDecTail<UINT16_MAX>(width, &str[idx], len - idx);
            IF_LIKELY(inRange)
            {
                CM_ASSUME(width > 0);
                fmtSpec.Width = static_cast<uint16_t>(width);
                fmtSpec.NonDefaultFlag |= FormatSpec::NonDefaultFlags::Width;
                idx += errIdx;

                if (idx == len) return true;
                ch = str[idx];
            }
            ELSE_UNLIKELY // out of range
            {
                result.SetError(offset + idx + errIdx, ParseResultBase::ErrorCode::WidthTooLarge);
                return false;
            }
        }
        
        const auto retcodePrec = ParsePrecision(result, fmtSpec, ch, str, idx, offset, len);
        IF_UNLIKELY(retcodePrec == 0)
            return false;
        if (retcodePrec == 1) // read a num, refresh ch
        {
            if (idx == len) return true;
            ch = str[idx];
        }
        
        const uint32_t typestr = ch;
        IF_LIKELY(typestr <= static_cast<uint32_t>(FormatSpec::TypeIdentifier::TypeCharMinMax.second))
        {
            const FormatSpec::TypeIdentifier type{ static_cast<uint8_t>(typestr) };
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
                IF_UNLIKELY(type.Type == ArgDispType::Date)
                {
                    // zeropad & signflag already checked
                    constexpr uint8_t forceDefualtFlag = FormatSpec::NonDefaultFlags::AlterForm
                        | FormatSpec::NonDefaultFlags::Precision
                        | FormatSpec::NonDefaultFlags::Width
                        | FormatSpec::NonDefaultFlags::Fill
                        | FormatSpec::NonDefaultFlags::Alignment;
                    IF_UNLIKELY(fmtSpec.NonDefaultFlag & forceDefualtFlag)
                    {
                        result.SetError(offset + idx, ParseResultBase::ErrorCode::IncompDateSpec);
                        return false;
                    }
                }
                else IF_UNLIKELY(type.Type == ArgDispType::Color)
                {
                    // zeropad & signflag already checked
                    constexpr uint8_t forceDefualtFlag = FormatSpec::NonDefaultFlags::Precision
                        | FormatSpec::NonDefaultFlags::Width
                        | FormatSpec::NonDefaultFlags::Fill;
                    IF_UNLIKELY(fmtSpec.NonDefaultFlag & forceDefualtFlag)
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
                        const auto fmtLen = len - idx;
                        IF_UNLIKELY(fmtLen >= UINT8_MAX)
                        {
                            result.SetError(offset + idx, ParseResultBase::ErrorCode::ExtraFmtTooLong);
                            return false;
                        }
                        fmtSpec.FmtLen = static_cast<uint8_t>(fmtLen);
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

    static_assert(Char_a > Char_0 && Char_A > Char_0 && Char_Under > Char_0); // make sure allowed char is all above '0'
    static_assert(Char_A < Char_Under && Char_Under < Char_a); // make sure allowed named char is all above 'A'
    static_assert(Char_A > 0x40u); // make sure A and above is in 64 element range'
    // return 0 for arg, 1 for err, 2 for others
    static forceinline constexpr uint32_t ParseArgId(ParseResultCommon& result, uint32_t firstCh, uint8_t& argIdx, ArgDispType*& namedArgDstType, const Char* str, const uint32_t offset, const uint32_t size) noexcept
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
        CM_ASSUME(size > 0);
        IF_LIKELY(firstCh >= Char_0 && firstCh < 0x80u) // ASCII limited
        {
            const auto chOffset0 = firstCh - Char_0;
            if (chOffset0 == 0u)
            {
                IF_UNLIKELY(size != 1)
                {
                    result.SetError(offset, ParseResultBase::ErrorCode::InvalidArgIdx);
                    return 1;
                }
                argIdx = 0;
                return 0;
            }
            if (chOffset0 <= 9u) // 0~9
            {
                uint32_t id = chOffset0;
                if constexpr (ParseResultCommon::IdxArgSlots < 10)
                {
                    IF_UNLIKELY(size > 1)
                    {
                        const auto nextChar = str[offset + 1];
                        const auto nextNotNum = (nextChar < Char_0 || nextChar > Char_9) ? 1u : 0u;
                        result.SetError(offset + nextNotNum, nextNotNum ? ParseResultBase::ErrorCode::InvalidArgIdx : ParseResultBase::ErrorCode::ArgIdxTooLarge);
                        return 1;
                    }
                    if (id >= ParseResultCommon::IdxArgSlots)
                    {
                        result.SetError(offset, ParseResultBase::ErrorCode::ArgIdxTooLarge);
                        return 1;
                    }
                }
                else if constexpr (ParseResultCommon::IdxArgSlots < 100)
                {
                    constexpr uint32_t hiNum = ParseResultCommon::IdxArgSlots / 10;
                    constexpr uint32_t loNum = ParseResultCommon::IdxArgSlots % 10;
                    if (size > 1)
                    {
                        const uint32_t nextChar = str[offset + 1];
                        IF_UNLIKELY(!(nextChar >= Char_0 && nextChar <= Char_9))
                        {
                            result.SetError(offset + 1, ParseResultBase::ErrorCode::InvalidArgIdx);
                            return 1;
                        }
                        const auto nextOffset0 = nextChar - Char_0;
                        CM_ASSUME(nextOffset0 < 10u);
                        IF_UNLIKELY(id > hiNum || (id == hiNum && nextOffset0 >= loNum))
                        {
                            result.SetError(offset, ParseResultBase::ErrorCode::ArgIdxTooLarge);
                            return 1;
                        }
                        IF_UNLIKELY(size > 2)
                        {
                            const auto thridChar = str[offset + 2];
                            const auto thirdNotNum = (thridChar < Char_0 || thridChar > Char_9) ? 1u : 0u;
                            result.SetError(offset + 1 + thirdNotNum, thirdNotNum ? ParseResultBase::ErrorCode::InvalidArgIdx : ParseResultBase::ErrorCode::ArgIdxTooLarge);
                            return 1;
                        }
                        id = id * 10 + nextOffset0;
                    }
                }
                else
                {
                    const auto [errIdx, inRange] = ParseDecTail<ParseResultCommon::IdxArgSlots>(id, str + offset, size);
                    if (!inRange || errIdx != size) // not full finish
                    {
                        result.SetError(offset + errIdx, inRange ? ParseResultBase::ErrorCode::InvalidArgIdx : ParseResultBase::ErrorCode::ArgIdxTooLarge);
                        return 1;
                    }
                }
                argIdx = static_cast<uint8_t>(id);
                return 0;
            }
            if (firstCh >= Char_A)
            {
                const auto chOffsetA = firstCh - Char_A;
                CM_ASSUME(chOffsetA < 64);
                if (AllowedFirstCharChecker.Get<bool>(chOffsetA))
                {
                    constexpr ASCIIChecker<true> NameArgTailChecker("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_");
                    for (uint32_t i = 1; i < size; ++i)
                    {
                        const auto argCh = str[offset + i];
                        IF_UNLIKELY(!NameArgTailChecker(argCh))
                        {
                            result.SetError(offset + i, ParseResultBase::ErrorCode::InvalidArgName);
                            return 1;
                        }
                    }
                    IF_UNLIKELY(size > UINT8_MAX)
                    {
                        result.SetError(offset, ParseResultBase::ErrorCode::ArgNameTooLong);
                        return 1;
                    }

                    CM_ASSUME(namedArgDstType == nullptr);
                    // CM_ASSUME(idx < str.size());
                    // CM_ASSUME(idx + idPartLen <= str.size());
                    // [[maybe_unused]] const auto idPart = str.substr(idx, idPartLen);
                    for (uint8_t i = 0; i < result.NamedArgCount; ++i)
                    {
                        auto& target = result.NamedTypes[i];
                        if (size != target.Length)
                            continue;
                        if (std::char_traits<Char>::compare(&str[target.Offset], &str[offset], size) == 0)
                        {
                            argIdx = i;
                            namedArgDstType = &target.Type;
                            break;
                        }
                    }

                    if (!namedArgDstType)
                    {
                        IF_UNLIKELY(result.NamedArgCount >= ParseResultCommon::NamedArgSlots)
                        {
                            result.SetError(offset, ParseResultBase::ErrorCode::TooManyNamedArg);
                            return 1;
                        }
                        argIdx = result.NamedArgCount++;
                        auto& target = result.NamedTypes[argIdx];
                        target.Offset = static_cast<uint16_t>(offset);
                        target.Length = static_cast<uint8_t>(size);
                        namedArgDstType = &target.Type;
                    }
                    return 0;
                }
            }
        }
        return 2;
    }

    template<bool AllowSIMD = true>
    static forceinline constexpr bool LocateFirstBrace(const Char* str, uint32_t& idx, bool& isLB, const uint32_t size) noexcept
    {
#if defined(COMMON_SIMD_HAS_128)
        if constexpr (AllowSIMD)
        {
            if (!common::is_constant_evaluated(true))
            {
                using namespace common::simd;
                using S = std::conditional_t<sizeof(Char) != 1, std::conditional_t<sizeof(Char) != 2, U32x4, U16x8>, U8x16>;
                using E = typename S::EleType;
                constexpr auto step = static_cast<uint32_t>(S::Count);
                S refLB(static_cast<E>(Char_LB)), refRB(static_cast<E>(Char_RB));
                while (idx + step <= size)
                {
                    const S dat(reinterpret_cast<const E*>(str + idx));
                    const auto checkLeft  = dat.template Compare<CompareType::Equal, MaskType::FullEle>(refLB);
                    const auto checkRight = dat.template Compare<CompareType::Equal, MaskType::FullEle>(refRB);
                    const auto [idxLB, maskLB] = checkLeft .template GetMaskFirstIndex<MaskType::FullEle, true>();
                    const auto [idxRB, maskRB] = checkRight.template GetMaskFirstIndex<MaskType::FullEle, true>();
                    if (maskLB | maskRB)
                    {
                        // if !maskLB, must maskRB, then cidxLB = step, cidxRB < step, cidxLB > cidxRB -> false = !isLB
                        // if  maskLB, and !maskRB, then cidxLB < step, cidxRB = step, cidxLB < cidxRB -> true  =  isLB
                        // if  maskLB, and  maskRB, then cidxLB < step, cidxRB < step, check idxLB < idxRB -> isLB
                        isLB = idxLB < idxRB;
                        const auto idxBrace = isLB ? idxLB : idxRB;
                        idx += idxBrace;
                        CM_ASSUME(idx < size);
                        return true;
                    }
                    idx += step;
                }
            }
        }
#endif
        {
            while (idx < size) // short loop to reduce jump range
            {
                const auto ch = str[idx];
                isLB = ch == Char_LB; // 0 when is LB
                IF_UNLIKELY(isLB || ch == Char_RB)
                    return true;
                idx++;
            }
            return false;
        }
    }

    template<bool AllowSIMD = true>
    static forceinline constexpr bool LocateColonAndRightBrace(const Char* str, uint32_t& idx, uint32_t& splitOffset, const uint32_t size) noexcept
    {
#if defined(COMMON_SIMD_HAS_128)
        if constexpr (AllowSIMD)
        {
            if (!common::is_constant_evaluated(true))
            {
                using namespace common::simd;
                using S = std::conditional_t<sizeof(Char) != 1, std::conditional_t<sizeof(Char) != 2, U32x4, U16x8>, U8x16>;
                using E = typename S::EleType;
                constexpr auto step = static_cast<uint32_t>(S::Count);
                S refColon(static_cast<E>(Char_Colon)), refRB(static_cast<E>(Char_RB));
                while (idx + step <= size)
                {
                    const S dat(reinterpret_cast<const E*>(str + idx));
                    const auto checkColon = dat.template Compare<CompareType::Equal, MaskType::FullEle>(refColon);
                    const auto checkRight = dat.template Compare<CompareType::Equal, MaskType::FullEle>(refRB);
                    const auto [idxCL, maskCL] = checkColon.template GetMaskFirstIndex<MaskType::FullEle, true>();
                    const auto [idxRB, maskRB] = checkRight.template GetMaskFirstIndex<MaskType::FullEle, true>();
                    const auto updOffset = maskCL ? idxCL + idx : 0;
                    splitOffset = splitOffset ? splitOffset : updOffset;
                    if (maskRB)
                    {
                        idx += idxRB;
                        CM_ASSUME(idx < size);
                        // splitOffset is <idx, 0, >idx, later 2 is not found and should replace to idx
                        // splitOffset-1 becomes MAX32, >=idx, both reuslt in >=idx
                        splitOffset = (splitOffset - 1) >= idx ? idx : splitOffset;
                        return true;
                    }
                    idx += step;
                }
            }
        }
#endif
        while (true)
        {
            IF_UNLIKELY(idx == size) // reach end before find RB
                return false;

            const Char ch = str[idx];
            const auto notColon = ch ^ Char_Colon;
            const auto notUpdateSplit = notColon | splitOffset;
            splitOffset = notUpdateSplit ? splitOffset : idx;
            IF_UNLIKELY(ch == Char_RB)
            {
                if (splitOffset == 0) // not assigned
                    splitOffset = idx;
                return true;
            }
            idx++;
        }
        return false;
    }

    template<uint16_t Size>
    static constexpr void ParseString(ParseResult<Size>& result, const std::basic_string_view<Char> str_) noexcept
    {
        using namespace std::string_view_literals;
        //constexpr auto End = std::basic_string_view<Char>::npos;
        //static_assert(End == SIZE_MAX);
        const auto size_ = str_.size();
        IF_UNLIKELY(size_ >= UINT16_MAX)
        {
            return result.SetError(0, ParseResultBase::ErrorCode::FmtTooLong);
        }
        const auto str = str_.data();
        const auto size = static_cast<uint32_t>(size_);
        FormatSpec fmtSpec; // resued outside of loop
        //uint32_t dbg = Size;
        for (uint32_t idx = 0; idx < size;)
        {
            const auto searchBeginIdx = idx;
            bool isLB = false;
            const bool findFirstBrace = LocateFirstBrace(str, idx, isLB, size);
            
            IF_UNLIKELY(!findFirstBrace) // not found a Brace when reach the end
            {
                CM_ASSUME(idx == size);
                const auto strLen = idx - searchBeginIdx;
                IF_UNLIKELY(!BuiltinOp::EmitFmtStr(result, searchBeginIdx, strLen))
                    return;
                break;
            }
            
            // isBrace
            CM_ASSUME(idx < size);
            //result.Opcodes[--dbg] = static_cast<uint8_t>(idx);
            const auto firstBraceIdx = idx;
            IF_UNLIKELY(idx + 1 == size) // last char is brace
            {
                return result.SetError(firstBraceIdx, isLB ? ParseResultBase::ErrorCode::MissingRightBrace : ParseResultBase::ErrorCode::MissingLeftBrace);
            }

            const Char nextCh = str[++idx];
            CM_ASSUME(idx < size);
            const uint32_t needEmitBrace = nextCh == (isLB ? Char_LB : Char_RB) ? 1u : 0u;
            if (const auto checkedLen = firstBraceIdx - searchBeginIdx; checkedLen > 0) // need emit FmtStr
            {
                //result.Opcodes[--dbg] = static_cast<uint8_t>(1u);
                const auto strLen = checkedLen + needEmitBrace;
                IF_UNLIKELY(!BuiltinOp::EmitFmtStr(result, searchBeginIdx, strLen))
                    return;
                IF_UNLIKELY(needEmitBrace)
                {
                    idx += 1; // eat extra brace "{{" or "}}"
                    continue; // continue searching
                }
            }
            else IF_UNLIKELY(needEmitBrace) // need emit only Brace
            {
                //result.Opcodes[--dbg] = static_cast<uint8_t>(3u);
                IF_UNLIKELY(!BuiltinOp::EmitBrace(result, firstBraceIdx, isLB))
                    return;
                idx += 1; // eat brace "{{" or "}}"
                continue; // continue searching
            }
            //result.Opcodes[--dbg] = static_cast<uint8_t>(4u);
            IF_UNLIKELY(!isLB) // is RB and is not emit
            {
                return result.SetError(firstBraceIdx, ParseResultBase::ErrorCode::MissingLeftBrace);
            }

            CM_ASSUME(isLB);
            //result.Opcodes[--dbg] = static_cast<uint8_t>(5u);
            IF_LIKELY(nextCh == Char_RB) // "{}"
            {
                //result.Opcodes[--dbg] = static_cast<uint8_t>(6u);
                IF_UNLIKELY(result.NextArgIdx >= ParseResultCommon::IdxArgSlots)
                {
                    return result.SetError(firstBraceIdx, ParseResultBase::ErrorCode::TooManyIdxArg);
                }
                const auto argIdx = result.NextArgIdx++;
                result.IdxArgCount = std::max(result.NextArgIdx, result.IdxArgCount);
                IF_UNLIKELY(!ArgOp::EmitDefault(result, firstBraceIdx, argIdx))
                    return;
                idx += 1; // eat '}'
                continue; // continue searching
            }

            //result.Opcodes[--dbg] = static_cast<uint8_t>(7u);
            const auto argfmtOffset = idx++; // move to next char
            uint32_t splitOffset = nextCh == Char_Colon ? argfmtOffset : 0u;
            // to find RB
            const auto findRB = LocateColonAndRightBrace(str, idx, splitOffset, size);
            IF_UNLIKELY(!findRB)
            {
                return result.SetError(firstBraceIdx, ParseResultBase::ErrorCode::MissingRightBrace);
            }

            CM_ASSUME(idx < size);
            //result.Opcodes[--dbg] = static_cast<uint8_t>(9u);
            IF_UNLIKELY(splitOffset + 1 == idx) // no split then 0+1 == 1 < idx
            {  // xxx:}
                return result.SetError(firstBraceIdx, ParseResultBase::ErrorCode::MissingFmtSpec);
            }
            // splitOffset < idx - 1 || splitOffset == idx
            //result.Opcodes[--dbg] = static_cast<uint8_t>(10u);

            // begin arg parsing
            uint8_t argIdx = 0;
            ArgDispType* dstType = nullptr;

            const auto hasSplit = splitOffset < idx;
            if (splitOffset != argfmtOffset) // has arg_id, no split will be idx and not equal
            {
                const auto idPartLen = splitOffset - argfmtOffset;
                const auto retcode = ParseArgId(result, nextCh, argIdx, dstType, str, argfmtOffset, idPartLen);
                IF_UNLIKELY(retcode > 0) // error or skip
                {
                    if (retcode > 1)
                    {
                        if (nextCh == Char_At) // Color
                        {
                            IF_UNLIKELY(hasSplit) // has split
                            {
                                return result.SetError(argfmtOffset, ParseResultBase::ErrorCode::InvalidColor);
                            }
                            IF_LIKELY(ParseColor(result, argfmtOffset, { str + argfmtOffset, idPartLen }))
                            {
                                idx += 1; // eat '}'
                                continue;
                            }
                        }
                    }
                    return;
                }
                // get an arg
            }
            else // idPartLen == 0
            {
                CM_ASSUME(hasSplit);
                IF_UNLIKELY(result.NextArgIdx >= ParseResultCommon::IdxArgSlots)
                {
                    return result.SetError(searchBeginIdx, ParseResultBase::ErrorCode::TooManyIdxArg);
                }
                argIdx = result.NextArgIdx++;
            }
            // arg handling left here
            const bool isNamed = dstType != nullptr; // named arg should get dstType set.
            if (!isNamed) // index arg, update argcount
            {
                dstType = &result.IndexTypes[argIdx];
                result.IdxArgCount = std::max(static_cast<uint8_t>(argIdx + 1), result.IdxArgCount);
            }
            CM_ASSUME(dstType);

            if (hasSplit)
            {
                const auto specOffset = splitOffset + 1;
                IF_UNLIKELY(!ParseFormatSpec(result, fmtSpec, str + specOffset, specOffset, idx - specOffset))
                    return;
            }

            IF_UNLIKELY(!ArgOp::Emit(&result, searchBeginIdx, hasSplit ? &fmtSpec : nullptr, dstType, argIdx, isNamed))
                return;
            idx += 1; // eat '}'
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

namespace detail
{
template<typename T, size_t N>
inline constexpr std::array<T, N> PopulateArray(const T* src) noexcept
{
    std::array<T, N> ret = {};
    for (size_t i = 0; i < N; ++i)
        ret[i] = src[i];
    return ret;
}
}

template<uint8_t IdxArgCount>
struct IdxArgLimiter
{
    std::array<ArgDispType, IdxArgCount> IndexTypes;
    constexpr IdxArgLimiter(const ArgDispType* type) noexcept : IndexTypes(detail::PopulateArray<ArgDispType, IdxArgCount>(type))
    { }
};
template<>
struct IdxArgLimiter<0>
{ 
    constexpr IdxArgLimiter(const ArgDispType*) noexcept {}
};
template<uint8_t NamedArgCount>
struct NamedArgLimiter
{
    std::array<ParseResultCommon::NamedArgType, NamedArgCount> NamedTypes;
    constexpr NamedArgLimiter(const ParseResultCommon::NamedArgType* type) noexcept : NamedTypes(detail::PopulateArray<ParseResultCommon::NamedArgType, NamedArgCount>(type))
    { }
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
struct CustomFormatterTag {};

template<typename Char, uint16_t OpCount_, uint8_t NamedArgCount_, uint8_t IdxArgCount_>
struct COMMON_EMPTY_BASES TrimedResult : public CompileTimeFormatter, public OpHolder<Char, OpCount_>, public NamedArgLimiter<NamedArgCount_>, public IdxArgLimiter<IdxArgCount_>
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
    constexpr std::basic_string_view<Char> GetNameSource() const noexcept { return this->FormatString; }
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
#define NAMEARG(name) [](auto&& arg) noexcept       \
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
    forceinline void Put(const T& arg, uint16_t idx) noexcept
    {
        constexpr auto NeedSize = sizeof(T);
        constexpr auto NeedSlot = (NeedSize + 1) / 2;
#if CM_DEBUG
        Expects(CurrentSlot + NeedSlot <= N);
#endif
        memcpy(&ArgStore[CurrentSlot], &arg, sizeof(T));
        //*reinterpret_cast<T*>(&ArgStore[CurrentSlot]) = arg;
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
    template<typename T>
    void FillInto(T& dst) const noexcept
    {
        static_assert(std::is_same_v<T, DateStructure>);
        dst.Base = *this;
        dst.Zone.clear();
        dst.GMTOffset = 0;
        dst.MicroSeconds = UINT32_MAX;
        if constexpr (detail::tm_has_all_zone_info)
        {
            dst.Base.tm_gmtoff = 0;
            dst.Base.tm_zone = nullptr;
        }
    }
};

struct CompactDateEx
{
private:
    template<typename T>
    constexpr void SetFrom([[maybe_unused]] const T& date) noexcept 
    {
        if constexpr (detail::has_gmtoff_v<std::tm>)
            GMTOffset = gsl::narrow_cast<int32_t>(date.tm_gmtoff);
        if constexpr (detail::has_zone_v<std::tm>)
            Zone = date.tm_zone;
    }
    template<typename T>
    constexpr void SetTo([[maybe_unused]] T& date) const noexcept
    {
        if constexpr (detail::has_gmtoff_v<std::tm>)
            date.tm_gmtoff = GMTOffset;
        if constexpr (detail::has_zone_v<std::tm>)
            date.tm_zone = Zone;
    }
public:
    const char* Zone = nullptr;
    int32_t GMTOffset = 0;
    uint32_t MicroSeconds = UINT32_MAX;
    CompactDate Date;
    constexpr CompactDateEx() noexcept {}
    constexpr CompactDateEx(const std::tm& date) noexcept : Date(date)
    {
        SetFrom(date);
    }
    constexpr CompactDateEx(const std::tm& date, uint32_t us) noexcept : CompactDateEx(date)
    {
        Expects(MicroSeconds < std::chrono::microseconds::period::den);
        MicroSeconds = us;
    }
    constexpr operator std::tm() const noexcept
    {
        std::tm date = Date;
        SetTo(date);
        return date;
    }
    template<typename T>
    void FillInto(T& dst) const noexcept
    {
        static_assert(std::is_same_v<T, DateStructure>);
        dst.Base = *this;
        dst.Zone = Zone;
        dst.GMTOffset = GMTOffset;
        dst.MicroSeconds = UINT32_MAX;
        if constexpr (detail::tm_has_all_zone_info)
        {
            dst.Base.tm_gmtoff = GMTOffset;
            dst.Base.tm_zone = Zone;
        }
    }
};

struct ZonedDate
{
    uint64_t Microseconds = 0;
    const common::date::time_zone* TimeZone = nullptr;
    template<typename Duration>
    constexpr ZonedDate(const common::date::zoned_time<Duration, const common::date::time_zone*>& time) noexcept :
        Microseconds(EncodeUs(time.get_local_time().time_since_epoch())), TimeZone(time.get_time_zone())
    { }

    template<typename Rep, typename Period>
    static constexpr uint64_t EncodeUs(std::chrono::duration<Rep, Period> time) noexcept
    {
        const auto deltaUs = std::chrono::duration_cast<std::chrono::duration<int64_t, std::micro>>(time).count();
        auto ret = static_cast<uint64_t>(deltaUs);
        Expects((ret >> 63) == ((ret >> 62) & 1));
        constexpr bool FPSecond = Period::num != 1 || Period::den != 1 || std::is_floating_point_v<Rep>;
        if constexpr (FPSecond)
        {
            ret |= static_cast<uint64_t>(0x8000000000000000);
        }
        else
        {
            ret &= static_cast<uint64_t>(0x7fffffffffffffff);
        }
        return ret;
    }
    template<typename Rep, typename Period>
    static void ConvertToDate(const common::date::time_zone* zone, std::chrono::duration<Rep, Period> time, DateStructure& date) noexcept
    {
        ConvertToDate(zone, EncodeUs(time), date);
    }
    SYSCOMMONAPI static void ConvertToDate(const common::date::time_zone* zone, uint64_t encodedUS, DateStructure& date) noexcept;
};


template<typename Char>
inline auto FormatAs(const SharedString<Char>& arg)
{
    return static_cast<std::basic_string_view<Char>>(arg);
}


namespace detail
{
template<typename T>
concept HasFormatAsMemFn = requires(T x) { x.FormatAs(); };
template<typename T>
concept HasFormatAsSpeFn = requires(T x) { ::common::str::FormatAs(std::forward<T>(x)); };
template<typename T>
concept HasFormatWithMemFn = requires(T x) { x.FormatWith(std::declval<FormatterHost&>(), std::declval<FormatterContext&>(), std::declval<const FormatSpec*>()); };
template<typename T>
concept HasFormatWithSpeFn = requires(T x) { FormatWith(std::forward<T>(x), std::declval<FormatterHost&>(), std::declval<FormatterContext&>(), std::declval<const FormatSpec*>()); };

using FmtWithPtr = void(*)(const void*, FormatterHost&, FormatterContext&, const FormatSpec*);
struct FmtWithPair
{
    const void* Data;
    FmtWithPtr Executor;
};
}


struct ArgInfo
{
    const char* NamePtrs[ParseResultCommon::NamedArgSlots] = { nullptr };
    uint8_t NameLens[ParseResultCommon::NamedArgSlots] = { 0 };
    ArgRealType NamedTypes[ParseResultCommon::NamedArgSlots] = { ArgRealType::Error };
    ArgRealType IndexTypes[ParseResultCommon::IdxArgSlots] = { ArgRealType::Error };
    uint8_t NamedArgCount = 0, IdxArgCount = 0;
    template<typename T>
    forceinline static COMMON_CONSTEVAL ArgRealType EncodeTypeSizeData() noexcept
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
    forceinline static COMMON_CONSTEVAL ArgRealType GetCharTypeData() noexcept
    {
        constexpr auto data = EncodeTypeSizeData<T>();
        static_assert(enum_cast(data) <= 0x20);
        return data;
    }
    template<typename T>
    static COMMON_CONSTEVAL bool CheckCharType() noexcept
    {
        bool result = std::is_same_v<T, char> || std::is_same_v<T, char16_t> || std::is_same_v<T, wchar_t> || std::is_same_v<T, char32_t>;
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
        result |= std::is_same_v<T, char8_t>;
#endif
        return result;
    }
    template<typename T, bool AllowPair = false>
    static COMMON_CONSTEVAL bool CheckColorType() noexcept
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
    static COMMON_CONSTEVAL ArgRealType GetArgType() noexcept
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
        else if constexpr (std::is_same_v<U, std::tm> || std::is_same_v<U, CompactDate> || std::is_same_v<U, CompactDateEx>)
        {
            constexpr auto needExtra = std::is_same_v<U, std::tm> ? detail::tm_has_zone_info : std::is_same_v<U, CompactDateEx>;
            return needExtra ? ArgRealType::Date | ArgRealType::DateZoneBit : ArgRealType::Date;
        }
        else if constexpr (is_specialization<U, std::chrono::time_point>::value)
        {
            return ArgRealType::Date | ArgRealType::DateDeltaBit;
        }
#if SYSCOMMON_DATE
#   if SYSCOMMON_DATE == 1
        else if constexpr (is_specialization<U, std::chrono::zoned_time>::value)
#   elif SYSCOMMON_DATE == 2
        else if constexpr (is_specialization<U, ::date::zoned_time>::value)
#   endif
        {
            return ArgRealType::Date | ArgRealType::DateZoneBit | ArgRealType::DateDeltaBit;
        }
#endif
        else if constexpr (CheckColorType<U>())
        {
            // size 1,2,4,8: CommonColor, pair<CommonColor>, ScreenColor, pair<ScreenColor>
            return ArgRealType::Color | EncodeTypeSizeData<U>();
        }
        else if constexpr (detail::HasFormatAsMemFn<T>)
        {
            return GetArgType<decltype(std::declval<T>().FormatAs())>();
        }
        else if constexpr (detail::HasFormatAsSpeFn<T>)
        {
            return GetArgType<decltype(FormatAs(std::declval<T>()))>();
        }
        else if constexpr (detail::HasFormatWithMemFn<U> || detail::HasFormatWithSpeFn<U>)
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
                static_assert(Name.Name.size() <= UINT8_MAX, "Arg name length should not exceed UINT8_MAX");
                result.NamePtrs[result.NamedArgCount] = Name.Name.data();
                result.NameLens[result.NamedArgCount] = static_cast<uint8_t>(Name.Name.size());
            }
            result.NamedArgCount++;
        }
        else
        {
            result.IndexTypes[result.IdxArgCount++] = GetArgType<T>();
        }
    }
    template<typename... Args>
    static COMMON_CONSTEVAL ArgInfo ParseArgs() noexcept
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
            else if constexpr (std::is_same_v<U, CompactDateEx>)
            {
                return arg;
            }
            else if constexpr (std::is_same_v<U, std::tm>)
            {
                if constexpr (detail::tm_has_zone_info)
                    return CompactDateEx(arg);
                else
                    return CompactDate(arg);
            }
            else if constexpr (is_specialization<U, std::chrono::time_point>::value)
            {
                return ZonedDate::EncodeUs(arg.time_since_epoch());
            }
#if SYSCOMMON_DATE
#   if SYSCOMMON_DATE == 1
            else if constexpr (is_specialization<U, std::chrono::zoned_time>::value)
#   elif SYSCOMMON_DATE == 2
            else if constexpr (is_specialization<U, ::date::zoned_time>::value)
#   endif
            {
                return ZonedDate(arg);
            }
#endif
            else if constexpr (CheckColorType<U>())
            {
                return arg;
            }
            else if constexpr (detail::HasFormatAsMemFn<T>)
            {
                return BoxAnArg(arg.FormatAs());
            }
            else if constexpr (detail::HasFormatAsSpeFn<T>)
            {
                return BoxAnArg(FormatAs(arg));
            }
            else if constexpr (detail::HasFormatWithMemFn<T>)
            {
                detail::FmtWithPair data{ &arg, [](const void* data, FormatterHost& host, FormatterContext& context, const FormatSpec* spec)
                    {
                        reinterpret_cast<const U*>(data)->FormatWith(host, context, spec);
                    } };
                return data;
            }
            else if constexpr (detail::HasFormatWithSpeFn<T>)
            {
                detail::FmtWithPair data{ &arg, [](const void* data, FormatterHost& host, FormatterContext& context, const FormatSpec* spec)
                    {
                        FormatWith(*reinterpret_cast<const U*>(data), host, context, spec);
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
    static COMMON_CONSTEVAL void CheckIdxArgMismatch() noexcept
    {
        static_assert(Index == ParseResultCommon::IdxArgSlots, "Type mismatch");
    }
    template<typename Char>
    static constexpr NamedCheckResult GetNamedArgMismatch(const ParseResultCommon::NamedArgType* ask, const std::basic_string_view<Char> fmtStr, uint8_t askCount,
        const ArgRealType* give, const char* const* giveNamePtrs, const uint8_t* giveNameLens, uint8_t giveCount) noexcept
    {
        NamedCheckResult ret;
        for (uint8_t i = 0; i < askCount; ++i)
        {
            //CM_ASSUME(ask[i].Offset + ask[i].Length < fmtStr.size());
            const auto askName = &fmtStr[ask[i].Offset];
            const auto askNameLen = ask[i].Length;
            //const auto askName = fmtStr.substr(ask[i].Offset, ask[i].Length);
            bool found = false, match = false;
            uint8_t j = 0;
            for (; j < giveCount; ++j)
            {
                const auto giveNameLen = giveNameLens[j];
                if (giveNameLen == askNameLen)
                {
                    const auto giveName = giveNamePtrs[j];
                    bool fullMatch = true;
                    for (uint32_t k = 0; k < giveNameLen; ++k)
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
    static COMMON_CONSTEVAL void CheckNamedArgMismatch() noexcept
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
    static constexpr NamedMapper EmptyMapper = { 0 };
    template<typename StrType, typename... Args>
    static COMMON_CONSTEVAL NamedMapper CheckSS() noexcept
    {
        constexpr StrType StrInfo;
        constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
        static_assert(ArgsInfo.IdxArgCount >= StrInfo.IdxArgCount, "No enough indexed arg");
        static_assert(ArgsInfo.NamedArgCount >= StrInfo.NamedArgCount, "No enough named arg");
        if constexpr (StrInfo.IdxArgCount > 0)
        {
            constexpr auto Index = GetIdxArgMismatch(StrInfo.IndexTypes.data(), ArgsInfo.IndexTypes, StrInfo.IdxArgCount);
            CheckIdxArgMismatch<Index>();
        }
        if constexpr (StrInfo.NamedArgCount > 0)
        {
            constexpr auto NamedRet = GetNamedArgMismatch(StrInfo.NamedTypes.data(), StrInfo.GetNameSource(), StrInfo.NamedArgCount,
                ArgsInfo.NamedTypes, ArgsInfo.NamePtrs, ArgsInfo.NameLens, ArgsInfo.NamedArgCount);
            CheckNamedArgMismatch<NamedRet.AskIndex ? *NamedRet.AskIndex : ParseResultCommon::NamedArgSlots,
                NamedRet.GiveIndex ? *NamedRet.GiveIndex : ParseResultCommon::NamedArgSlots>();
            return NamedRet.Mapper;
        }
        return EmptyMapper;
    }
    template<typename StrType, typename... Args>
    static constexpr NamedMapper CheckSS(const StrType&, Args&&...) noexcept
    {
        return CheckSS<StrType, Args...>();
    }
};


template<typename Char>
struct Formatter;
struct FormatterBase
{
protected:
    template<typename Char>
    SYSCOMMONAPI static void NestedExecute(FormatterHost& host, FormatterContext& context,
        const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo, span<const uint16_t> argStore, const NamedMapper& mapping);
public:
    template<typename T, typename... Args>
    forceinline static void NestedFormat(FormatterHost& host, FormatterContext& context, const T&, Args&&... args)
    {
        using Char = typename std::decay_t<T>::CharType;
        static constexpr auto Mapping = ArgChecker::CheckSS<T, Args...>();
        static constexpr T StrInfo;
        static constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
        const auto argStore = ArgInfo::PackArgsStatic(std::forward<Args>(args)...);
        NestedExecute(host, context, StrInfo.ToStrArgInfo(), ArgsInfo, argStore.ArgStore, Mapping);
    }
};

struct SpecReader
{
public:
    [[nodiscard]] forceinline static constexpr uint32_t ReadLengthedVal(const uint8_t* ptr, uint32_t& size, uint32_t lenval, uint32_t val) noexcept
    {
        if (common::is_constant_evaluated(true))
        {
            switch (lenval & 0b11u)
            {
            case 1:  val = ptr[size]; size += 1; break;
            case 2:  val = ptr[size + 0] | (ptr[size + 1] << 8); size += 2; break;
            case 3:  val = ptr[size + 0] | (ptr[size + 1] << 8) | (ptr[size + 2] << 16) | (ptr[size + 3] << 24); size += 4; break;
            case 0:
            default: break;
            }
        }
        else // since it's LE, simply cast-pointer and read it
        {
            switch (lenval & 0b11u)
            {
            case 1:  val = ptr[size]; size += 1; break;
            case 2:  val = *reinterpret_cast<const uint16_t*>(&ptr[size]); size += 2; break;
            case 3:  val = *reinterpret_cast<const uint32_t*>(&ptr[size]); size += 4; break;
            case 0:
            default: break;
            }
        }
        return val;
    };
    forceinline static constexpr uint32_t ReadSpec(FormatSpec& spec, const uint8_t* ptr) noexcept
    {
        if (!ptr)
            return 0;
        uint32_t size = 0;
        const auto val0 = ptr[size++];
        const auto val1 = ptr[size++];
        spec.TypeExtra = static_cast<uint8_t>(val0 >> 5);
        spec.Alignment = static_cast<FormatSpec::Align>((val0 >> 2) & 0b11);
        spec.SignFlag  = static_cast<FormatSpec::Sign> ((val0 >> 0) & 0b11);
        spec.AlterForm = val1 & 0x80;
        spec.ZeroPad   = val1 & 0x40;
        IF_UNLIKELY(val1 & 0b111111u)
        {
            spec.Fill       = ReadLengthedVal(ptr, size, val1 >> 4, ' ');
            spec.Precision  = ReadLengthedVal(ptr, size, val1 >> 2, UINT32_MAX);
            spec.Width      = static_cast<uint16_t>(ReadLengthedVal(ptr, size, val1, 0));
        }
        else
        {
            spec.Fill = ' ';
            spec.Precision = UINT32_MAX;
            spec.Width = 0;
        }
        const bool hasExtraFmt = val0 & 0x10;
        IF_UNLIKELY(hasExtraFmt)
        {
            spec.TypeExtra &= static_cast<uint8_t>(0x1u);
            spec.FmtOffset  = static_cast<uint16_t>(ReadLengthedVal(ptr, size, val0 & 0x80 ? 2 : 1, 0));
            spec.FmtLen     = ptr[size++];
        }
        else
        {
            spec.FmtOffset = spec.FmtLen = 0;
        }
        return size;
    }
};


template<typename Char>
struct Formatter : public DateFormatter<Formatter<Char>, std::basic_string<Char>>
{
    using StrType = std::basic_string<Char>;
public:
    SYSCOMMONAPI static void PutString(StrType& ret, const void* str, size_t len, StringType type, const OpaqueFormatSpec& spec);
    SYSCOMMONAPI static void PutInteger(StrType& ret, uint32_t val, bool isSigned, const OpaqueFormatSpec& spec);
    SYSCOMMONAPI static void PutInteger(StrType& ret, uint64_t val, bool isSigned, const OpaqueFormatSpec& spec);
    SYSCOMMONAPI static void PutFloat(StrType& ret, float  val, const OpaqueFormatSpec& spec);
    SYSCOMMONAPI static void PutFloat(StrType& ret, double val, const OpaqueFormatSpec& spec);
    SYSCOMMONAPI static void PutPointer(StrType& ret, uintptr_t val, const OpaqueFormatSpec& spec);

    SYSCOMMONAPI static void PutColorStr(StrType& ret, ScreenColor color, const FormatSpec* spec);

    SYSCOMMONAPI static void PutString(StrType& ret, const void* str, size_t len, StringType type, const FormatSpec* spec);
    SYSCOMMONAPI static void PutInteger(StrType& ret, uint32_t val, bool isSigned, const FormatSpec* spec);
    SYSCOMMONAPI static void PutInteger(StrType& ret, uint64_t val, bool isSigned, const FormatSpec* spec);
    SYSCOMMONAPI static void PutFloat(StrType& ret, float  val, const FormatSpec* spec);
    SYSCOMMONAPI static void PutFloat(StrType& ret, double val, const FormatSpec* spec);
    SYSCOMMONAPI static void PutPointer(StrType& ret, uintptr_t val, const FormatSpec* spec);

    SYSCOMMONAPI static void PutColor(StrType& ret, ScreenColor color);
    SYSCOMMONAPI static void PutDate(StrType& ret, const void* fmtStr, size_t len, StringType type, const DateStructure& date);


    template<typename Spec>
    forceinline static void PutString(StrType& ret, ::std::string_view str, const Spec& spec)
    {
        PutString(ret, str.data(), str.size(), StringType::UTF8, spec);
    }
    template<typename Spec>
    forceinline static void PutString(StrType& ret, ::std::wstring_view str, const Spec& spec)
    {
        PutString(ret, str.data(), str.size(), sizeof(wchar_t) == sizeof(char16_t) ? StringType::UTF16 : StringType::UTF32, spec);
    }
    template<typename Spec>
    forceinline static void PutString(StrType& ret, ::std::u16string_view str, const Spec& spec)
    {
        PutString(ret, str.data(), str.size(), StringType::UTF16, spec);
    }
    template<typename Spec>
    forceinline static void PutString(StrType& ret, ::std::u32string_view str, const Spec& spec)
    {
        PutString(ret, str.data(), str.size(), StringType::UTF32, spec);
    }
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    template<typename Spec>
    forceinline static void PutString(StrType& ret, ::std::u8string_view str, const Spec& spec)
    {
        PutString(ret, str.data(), str.size(), StringType::UTF8, spec);
    }
#endif

    using DateFormatter<Formatter<Char>, std::basic_string<Char>>::PutDate;

    template<typename T, typename... Args>
    forceinline void FormatToStatic(std::basic_string<Char>& dst, const T&, Args&&... args)
    {
        static_assert(std::is_same_v<typename std::decay_t<T>::CharType, Char>);
        static constexpr auto Mapping = ArgChecker::CheckSS<T, Args...>();
        static constexpr T StrInfo;
        static constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
        const auto argStore = ArgInfo::PackArgsStatic(std::forward<Args>(args)...);
        FormatTo(dst, StrInfo, ArgsInfo, argStore.ArgStore, Mapping);
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
    template<typename T>
    forceinline void DirectFormatTo(std::basic_string<Char>& dst, const T& target, const FormatSpec* spec = nullptr)
    {
        static_assert(ArgInfo::GetArgType<T>() == ArgRealType::Custom);
        const auto fmtPair = ArgInfo::BoxAnArg(target);
        static_assert(std::is_same_v<std::decay_t<decltype(fmtPair)>, detail::FmtWithPair>);
        DirectFormatTo_(dst, fmtPair, spec);
    }
    template<typename T>
    forceinline std::basic_string<Char> DirectFormat(const T& target, const FormatSpec* spec = nullptr)
    {
        std::basic_string<Char> ret;
        DirectFormatTo(ret, target, spec);
        return ret;
    }
    SYSCOMMONAPI void FormatTo(std::basic_string<Char>& ret, const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo, span<const uint16_t> argStore, const NamedMapper& mapping);
private:
    SYSCOMMONAPI void DirectFormatTo_(std::basic_string<Char>& dst, const detail::FmtWithPair& fmtPair, const FormatSpec* spec);
    SYSCOMMONAPI void FormatToDynamic_(std::basic_string<Char>& dst, std::basic_string_view<Char> format, const ArgInfo& argInfo, span<const uint16_t> argStore);
};


#define FmtString2(str_) []() noexcept                                          \
{                                                                               \
    using FMT_P = ::common::str::FormatterParser;                               \
    using FMT_R = ::common::str::ParseResultBase;                               \
    using FMT_C = std::decay_t<decltype(str_[0])>;                              \
    constexpr auto Data    = FMT_P::ParseString(str_);                          \
    [[maybe_unused]] constexpr auto Check =                                     \
        FMT_R::CheckErrorCompile<Data.ErrorPos, Data.ErrorNum>();               \
    using FMT_T = ::common::str::TrimedResult<FMT_C,                            \
        Data.OpCount, Data.NamedArgCount, Data.IdxArgCount>;                    \
    struct FMT_Type : public FMT_T                                              \
    {                                                                           \
        using CharType = FMT_C;                                                 \
        constexpr FMT_Type() noexcept : FMT_T(FMT_P::ParseString(str_), str_) {}\
    };                                                                          \
    constexpr FMT_Type Dummy;                                                   \
    return Dummy;                                                               \
}()

#define FmtString(str_) *[]() noexcept                              \
{                                                                   \
    using FMT_P = ::common::str::FormatterParser;                   \
    using FMT_R = ::common::str::ParseResultBase;                   \
    using FMT_C = std::decay_t<decltype(str_[0])>;                  \
    static constexpr auto Data = FMT_P::ParseString(str_);          \
    [[maybe_unused]] constexpr auto Check =                         \
        FMT_R::CheckErrorCompile<Data.ErrorPos, Data.ErrorNum>();   \
    using FMT_T = ::common::str::TrimedResult<FMT_C,                \
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

#if COMMON_COMPILER_GCC && COMMON_GCC_VER < 100000
#   pragma GCC diagnostic pop
#endif
#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic pop
#endif
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
