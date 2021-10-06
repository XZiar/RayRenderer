#include "rely.h"
#include <algorithm>
#include "SystemCommon/Format.h"

#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace common::str::exp;


#define CheckSuc() EXPECT_EQ(ret.ErrorPos, UINT16_MAX)

#define CheckFail(pos, err) do                                                      \
{                                                                                   \
    EXPECT_EQ(ret.ErrorPos,  static_cast<uint16_t>(pos));                           \
    EXPECT_EQ(ret.OpCount,   static_cast<uint16_t>(ParseResult::ErrorCode::err));   \
} while(0)

#define CheckEachIdxArgType(r, ret, i, type) EXPECT_EQ(ret.IndexTypes[i], ArgType::type);
#define CheckIdxArgType(ret, next, ...) do                                                      \
{                                                                                               \
    EXPECT_EQ(ret.NextArgIdx,    static_cast<uint8_t>(next));                                   \
    EXPECT_EQ(ret.IdxArgCount,   BOOST_PP_VARIADIC_SIZE(__VA_ARGS__));                          \
    BOOST_PP_SEQ_FOR_EACH_I(CheckEachIdxArgType, ret, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))    \
} while(0)
#define CheckIdxArgCount(ret, next, cnt) do                     \
{                                                               \
    EXPECT_EQ(ret.NextArgIdx,    static_cast<uint8_t>(next));   \
    EXPECT_EQ(ret.IdxArgCount,   static_cast<uint8_t>(cnt));    \
} while(0)

#define CheckOp(op, ...) do { SCOPED_TRACE("Check" #op); Check##op##Op_(ret, idx, __VA_ARGS__); } while(0)
#define CheckOpFinish() EXPECT_EQ(ret.OpCount, idx)

static std::string OpToString(uint8_t op)
{
    const auto opid     = op & ParseResult::OpIdMask;
    const auto opfield  = op & ParseResult::OpFieldMask;
    const auto opdata   = op & ParseResult::OpDataMask;
    switch (opid)
    {
    case ParseResult::BuiltinOp::Op:
    {
        switch (opfield)
        {
        case ParseResult::BuiltinOp::FieldFmtStr:
        {
            auto ret = "[FmtStr]"s;
            ret.append(opdata & ParseResult::BuiltinOp::DataOffset16 ? "(off16)" : "(off8)");
            ret.append(opdata & ParseResult::BuiltinOp::DataLength16 ? "(len16)" : "(len8)");
            return ret;
        }
        case ParseResult::BuiltinOp::FieldBrace:
        {
            auto ret = "[Brace]"s;
            switch (opdata)
            {
            case 0:  return ret.append("({)");
            case 1:  return ret.append("(})");
            default: return ret.append("(Error)");
            }
        }
        default:
            return "[Builtin Error]"s;
        }
    }
    case ParseResult::ColorOp::Op:
    {
        auto ret = "[Color]"s;
        ret.append((opfield & ParseResult::ColorOp::FieldBackground) ? "(BG)" : "(FG)");
        if (opfield & ParseResult::ColorOp::FieldSpecial)
        {
            switch (opdata)
            {
            case ParseResult::ColorOp::DataDefault: return ret.append("(default)");
            case ParseResult::ColorOp::DataBit8:    return ret.append("(8bit)");
            case ParseResult::ColorOp::DataBit24:   return ret.append("(24bit)");
            default:                                return ret.append("(Error)");
            }
        }
        else
        {
            return ret.append(common::GetColorName(static_cast<common::CommonColor>(opdata)));
        }
    }
    case ParseResult::ArgOp::Op:
    {
        auto ret = "[Arg]"s;
        ret.append((opfield & ParseResult::ArgOp::FieldNamed) ? "(Idx)" : "(Named)");
        if (opfield & ParseResult::ArgOp::FieldHasSpec) ret.append("(spec)");
        return ret;
    }
    default:
        return "[Error]"s;
    }
}

static void CheckOpCode(uint8_t opcode, uint8_t ref)
{
    EXPECT_EQ(opcode, ref) << "opcode is " << OpToString(opcode);
}

static void CheckFmtStrOp_(const ParseResult& ret, uint16_t& idx, uint16_t offset, uint16_t length, std::string_view str)
{
    ASSERT_EQ(ret.FormatString.substr(offset, length), str);
    uint8_t op = ParseResult::BuiltinOp::Op | ParseResult::BuiltinOp::FieldFmtStr;
    if (offset > UINT8_MAX) op |= ParseResult::BuiltinOp::DataOffset16;
    if (length > UINT8_MAX) op |= ParseResult::BuiltinOp::DataLength16;
    CheckOpCode(ret.Opcodes[idx++], op);
    EXPECT_EQ(ret.Opcodes[idx++], static_cast<uint8_t>(offset));
    if (offset > UINT8_MAX)
    {
        EXPECT_EQ(ret.Opcodes[idx++], static_cast<uint8_t>(offset >> 8));
    }
    EXPECT_EQ(ret.Opcodes[idx++], static_cast<uint8_t>(length));
    if (length > UINT8_MAX)
    {
        EXPECT_EQ(ret.Opcodes[idx++], static_cast<uint8_t>(length >> 8));
    }
}

static void CheckBraceOp_(const ParseResult& ret, uint16_t& idx, const char ch)
{
    CheckOpCode(ret.Opcodes[idx++], ParseResult::BuiltinOp::Op | ParseResult::BuiltinOp::FieldBrace | (ch == '{' ? 0x0 : 0x1));
}

using ColorArgData = std::variant<std::monostate, common::CommonColor, uint8_t, std::array<uint8_t, 3>>;
static void CheckColorArgOp_(const ParseResult& ret, uint16_t& idx, bool isFG, ColorArgData data)
{
    uint8_t op = ParseResult::ColorOp::Op | 
        (isFG ? ParseResult::ColorOp::FieldForeground : ParseResult::ColorOp::FieldBackground);
    switch (data.index())
    {
    case 0: op |= ParseResult::ColorOp::FieldSpecial | ParseResult::ColorOp::DataDefault; break;
    case 1: op |= ParseResult::ColorOp::FieldCommon  | common::enum_cast(std::get<1>(data)); break;
    case 2: op |= ParseResult::ColorOp::FieldSpecial | ParseResult::ColorOp::DataBit8; break;
    case 3: op |= ParseResult::ColorOp::FieldSpecial | ParseResult::ColorOp::DataBit24; break;
    default: ASSERT_FALSE(data.index());
    }
    CheckOpCode(ret.Opcodes[idx++], op);
    if (data.index() == 2)
        EXPECT_EQ(ret.Opcodes[idx++], std::get<2>(data));
    else if (data.index() == 3)
    {
        const auto& rgb = std::get<3>(data);
        EXPECT_EQ(ret.Opcodes[idx++], rgb[0]);
        EXPECT_EQ(ret.Opcodes[idx++], rgb[1]);
        EXPECT_EQ(ret.Opcodes[idx++], rgb[2]);
    }
}

static void CheckIdxArgOp_(const ParseResult& ret, uint16_t& idx, uint8_t argidx, const ParseResult::FormatSpec* spec)
{
    const uint8_t field = ParseResult::ArgOp::FieldIndexed | (spec ? ParseResult::ArgOp::FieldHasSpec : 0);
    CheckOpCode(ret.Opcodes[idx++], ParseResult::ArgOp::Op | field);
    EXPECT_EQ(ret.Opcodes[idx++], argidx);
    if (spec)
    {
        uint8_t data[12] = { 0 };
        const auto [type, opcnt] = ParseResult::ArgOp::EncodeSpec(*spec, data);
        common::span<const uint8_t> ref{ data, opcnt }, src{ ret.Opcodes + idx, opcnt };
        EXPECT_THAT(src, testing::ContainerEq(ref));
        idx += opcnt;
    }
}


TEST(Format, ParseString)
{
    {
        constexpr auto ret = ParseResult::ParseString(""sv);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOpFinish();
    }
    {
        constexpr auto ret = ParseResult::ParseString("{}"sv);
        CheckSuc();
        CheckIdxArgType(ret, 1, Any);
        uint16_t idx = 0;
        CheckOp(IdxArg, 0, nullptr);
        CheckOpFinish();
    }
    {
        constexpr auto ret = ParseResult::ParseString("}"sv);
        CheckFail(0, MissingLeftBrace);
        CheckIdxArgCount(ret, 0, 0);
    }
    {
        constexpr auto ret = ParseResult::ParseString("{"sv);
        CheckFail(0, MissingRightBrace);
        CheckIdxArgCount(ret, 0, 0);
    }
    {
        constexpr auto ret = ParseResult::ParseString("{{"sv);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(Brace, '{');
        CheckOpFinish();
    }
    {
        constexpr auto ret = ParseResult::ParseString("}}"sv);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(Brace, '}');
        CheckOpFinish();
    }
    {
        constexpr auto ret = ParseResult::ParseString("{{{{{{"sv);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(Brace, '{');
        CheckOp(Brace, '{');
        CheckOp(Brace, '{');
        CheckOpFinish();
    }
    {
        constexpr auto ret = ParseResult::ParseString("{{}"sv);
        CheckFail(2, MissingLeftBrace);
        CheckIdxArgCount(ret, 0, 0);
    }
    {
        constexpr auto ret = ParseResult::ParseString("Hello"sv);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(FmtStr, 0, 5, "Hello");
        CheckOpFinish();
    }
    {
        constexpr auto ret = ParseResult::ParseString("Hello{{"sv);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(FmtStr, 0, 6, "Hello{");
        CheckOpFinish();
    }
    {
        constexpr auto ret = ParseResult::ParseString("Hello{}"sv);
        CheckSuc();
        CheckIdxArgType(ret, 1, Any);
        uint16_t idx = 0;
        CheckOp(FmtStr, 0, 5, "Hello");
        CheckOp(IdxArg, 0, nullptr);
        CheckOpFinish();
    }
    {
        constexpr auto ret = ParseResult::ParseString("Hello{}{}"sv);
        CheckSuc();
        CheckIdxArgType(ret, 2, Any, Any);
        uint16_t idx = 0;
        CheckOp(FmtStr, 0, 5, "Hello");
        CheckOp(IdxArg, 0, nullptr);
        CheckOp(IdxArg, 1, nullptr);
        CheckOpFinish();
    }
    {
        constexpr auto ret = ParseResult::ParseString("Hello{}{3}{}"sv);
        CheckSuc();
        CheckIdxArgType(ret, 2, Any, Any, Any, Any);
        uint16_t idx = 0;
        CheckOp(FmtStr, 0, 5, "Hello");
        CheckOp(IdxArg, 0, nullptr);
        CheckOp(IdxArg, 3, nullptr);
        CheckOp(IdxArg, 1, nullptr);
        CheckOpFinish();
    }
    {
        constexpr auto ret = ParseResult::ParseString("{@<default}{@>black}{@<blue+}{@>bff}{@<b06}{@>badbad}"sv);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(ColorArg, true, {});
        CheckOp(ColorArg, false, common::CommonColor::Black);
        CheckOp(ColorArg, true, common::CommonColor::BrightBlue);
        CheckOp(ColorArg, false, uint8_t(0xff));
        CheckOp(ColorArg, true, static_cast<common::CommonColor>(0x06));
        std::array<uint8_t, 3> rgb = { 0xba, 0xdb, 0xad };
        CheckOp(ColorArg, false, rgb);
        CheckOpFinish();
    }
    {
        const auto ret = ParseResult::ParseString("{@}"sv);
        CheckFail(1, InvalidColor);
        CheckIdxArgCount(ret, 0, 0);
    }
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wpedantic"
#endif
    {
        constexpr auto ret = ParseResult::ParseString("{:d}{:6.3f}{:<#0x}"sv);
        CheckSuc();
        CheckIdxArgType(ret, 3, Integer, Float, Integer);
        uint16_t idx = 0;
        ParseResult::FormatSpec spec0
        {
            .Type = 'd'
        };
        CheckOp(IdxArg, 0, &spec0);
        ParseResult::FormatSpec spec1
        {
            .Precision = 3,
            .Width = 6,
            .Type = 'f'
        };
        CheckOp(IdxArg, 1, &spec1);
        ParseResult::FormatSpec spec2
        {
            .Type = 'x',
            .Alignment = ParseResult::FormatSpec::Align::Left,
            .AlterForm = true,
            .ZeroPad = true
        };
        CheckOp(IdxArg, 2, &spec2);
        CheckOpFinish();
    }
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic pop
#endif
}
