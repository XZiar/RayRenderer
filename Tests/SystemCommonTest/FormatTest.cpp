#include "rely.h"
#include <algorithm>
#include "SystemCommon/Format.h"
#include "SystemCommon/FormatExtra.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/StringFormat.h"
#include "common/TimeUtil.hpp"
#include "3rdParty/fmt/include/fmt/compile.h"

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace common::str;
using common::BaseException;


#define CheckSuc() EXPECT_EQ(ret.ErrorPos, UINT16_MAX)

#define CheckFail(pos, err) do                                                      \
{                                                                                   \
    EXPECT_EQ(ret.ErrorPos, static_cast<uint16_t>(pos));                            \
    EXPECT_EQ(ret.ErrorNum, static_cast<uint16_t>(ParseResultBase::ErrorCode::err));\
} while(0)

#define CheckEachIdxArgType(r, ret, type) EXPECT_EQ(ret.IndexTypes[theArgIdx++], ArgDispType::type);
#define CheckIdxArgType(ret, next, ...) do                                                  \
{                                                                                           \
    EXPECT_EQ(ret.NextArgIdx,    static_cast<uint8_t>(next));                               \
    uint8_t theArgIdx = 0;                                                                  \
    BOOST_PP_SEQ_FOR_EACH(CheckEachIdxArgType, ret, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))  \
    EXPECT_EQ(ret.IdxArgCount,   theArgIdx);                                                \
} while(0)
#define CheckIdxArgCount(ret, next, cnt) do                     \
{                                                               \
    EXPECT_EQ(ret.NextArgIdx,    static_cast<uint8_t>(next));   \
    EXPECT_EQ(ret.IdxArgCount,   static_cast<uint8_t>(cnt));    \
} while(0)
#define CheckEachNamedArg(r, x, tp) do                                      \
{                                                                           \
    const auto& namedArg = pres.NamedTypes[theArgIdx];                      \
    EXPECT_EQ(namedArg.Type, ArgDispType::BOOST_PP_TUPLE_ELEM(2, 0, tp));   \
    const auto str = fmtstr.substr(namedArg.Offset, namedArg.Length);       \
    EXPECT_EQ(str, BOOST_PP_TUPLE_ELEM(2, 1, tp));                          \
    theArgIdx++;                                                            \
} while(0);
#define CheckNamedArgs(ret, fmtStr, ...) do                                             \
{                                                                                       \
    uint8_t theArgIdx = 0;                                                              \
    const auto& pres = ret;                                                             \
    const auto& fmtstr = fmtStr;                                                        \
    BOOST_PP_SEQ_FOR_EACH(CheckEachNamedArg, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))  \
    EXPECT_EQ(pres.NamedArgCount, theArgIdx);                                           \
} while(0)
#define CheckNamedArgCount(ret, cnt) EXPECT_EQ(ret.NamedArgCount, static_cast<uint8_t>(cnt))

#define CheckOp(op, ...) do { SCOPED_TRACE("Check" #op); Check##op##Op_(ret, idx, __VA_ARGS__); } while(0)
#define CheckOpFinish() EXPECT_EQ(ret.OpCount, idx)


static std::string OpToString(uint8_t op)
{
    const auto opid     = op & FormatterParser::OpIdMask;
    const auto opfield  = op & FormatterParser::OpFieldMask;
    const auto opdata   = op & FormatterParser::OpDataMask;
    switch (opid)
    {
    case FormatterParser::BuiltinOp::Op:
    {
        switch (opfield)
        {
        case FormatterParser::BuiltinOp::FieldFmtStr:
        {
            auto ret = "[FmtStr]"s;
            ret.append(opdata & FormatterParser::BuiltinOp::DataOffset16 ? "(off16)" : "(off8)");
            ret.append(opdata & FormatterParser::BuiltinOp::DataLength16 ? "(len16)" : "(len8)");
            return ret;
        }
        case FormatterParser::BuiltinOp::FieldBrace:
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
    case FormatterParser::ColorOp::Op:
    {
        auto ret = "[Color]"s;
        ret.append((opfield & FormatterParser::ColorOp::FieldBackground) ? "(BG)" : "(FG)");
        if (opfield & FormatterParser::ColorOp::FieldSpecial)
        {
            switch (opdata)
            {
            case FormatterParser::ColorOp::DataDefault: return ret.append("(default)");
            case FormatterParser::ColorOp::DataBit8:    return ret.append("(8bit)");
            case FormatterParser::ColorOp::DataBit24:   return ret.append("(24bit)");
            default:                                return ret.append("(Error)");
            }
        }
        else
        {
            return ret.append(common::GetColorName(static_cast<common::CommonColor>(opdata)));
        }
    }
    case FormatterParser::ArgOp::Op:
    {
        auto ret = "[Arg]"s;
        ret.append((opfield & FormatterParser::ArgOp::FieldNamed) ? "(Idx)" : "(Named)");
        if (opfield & FormatterParser::ArgOp::FieldHasSpec) ret.append("(spec)");
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

template<typename Char>
static void CheckFmtStrOp_(const ParseResult<>& ret, uint16_t& idx, std::basic_string_view<Char> fmtStr, uint16_t offset, uint16_t length, std::basic_string_view<Char> str)
{
    ASSERT_EQ(fmtStr.substr(offset, length), str);
    uint8_t op = FormatterParser::BuiltinOp::Op | FormatterParser::BuiltinOp::FieldFmtStr;
    if (offset > UINT8_MAX) op |= FormatterParser::BuiltinOp::DataOffset16;
    if (length > UINT8_MAX) op |= FormatterParser::BuiltinOp::DataLength16;
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

static void CheckBraceOp_(const ParseResult<>& ret, uint16_t& idx, const char ch)
{
    CheckOpCode(ret.Opcodes[idx++], FormatterParser::BuiltinOp::Op | FormatterParser::BuiltinOp::FieldBrace | (ch == '{' ? 0x0 : 0x1));
}

using ColorArgData = std::variant<std::monostate, common::CommonColor, uint8_t, std::array<uint8_t, 3>>;
static void CheckColorArgOp_(const ParseResult<>& ret, uint16_t& idx, bool isFG, ColorArgData data)
{
    uint8_t op = FormatterParser::ColorOp::Op |
        (isFG ? FormatterParser::ColorOp::FieldForeground : FormatterParser::ColorOp::FieldBackground);
    switch (data.index())
    {
    case 0: op |= FormatterParser::ColorOp::FieldSpecial | FormatterParser::ColorOp::DataDefault; break;
    case 1: op |= FormatterParser::ColorOp::FieldCommon  | common::enum_cast(std::get<1>(data)); break;
    case 2: op |= FormatterParser::ColorOp::FieldSpecial | FormatterParser::ColorOp::DataBit8; break;
    case 3: op |= FormatterParser::ColorOp::FieldSpecial | FormatterParser::ColorOp::DataBit24; break;
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

static void CheckIdxArgOp_(const ParseResult<>& ret, uint16_t& idx, uint8_t argidx, const FormatterParser::FormatSpec* spec)
{
    const uint8_t field = FormatterParser::ArgOp::FieldIndexed | (spec ? FormatterParser::ArgOp::FieldHasSpec : 0);
    CheckOpCode(ret.Opcodes[idx++], FormatterParser::ArgOp::Op | field);
    EXPECT_EQ(ret.Opcodes[idx++], argidx);
    if (spec)
    {
        uint8_t data[FormatterParser::ArgOp::SpecLength[1]] = { 0 };
        const auto opcnt = FormatterParser::ArgOp::EncodeSpec(*spec, data);
        common::span<const uint8_t> ref{ data, opcnt }, src{ ret.Opcodes + idx, opcnt };
        EXPECT_THAT(src, testing::ElementsAreArray(ref));
        idx += static_cast<uint16_t>(opcnt);
    }
}

static void CheckNamedArgOp_(const ParseResult<>& ret, uint16_t& idx, uint8_t argidx, const FormatterParser::FormatSpec* spec)
{
    const uint8_t field = FormatterParser::ArgOp::FieldNamed | (spec ? FormatterParser::ArgOp::FieldHasSpec : 0);
    CheckOpCode(ret.Opcodes[idx++], FormatterParser::ArgOp::Op | field);
    EXPECT_EQ(ret.Opcodes[idx++], argidx);
    if (spec)
    {
        uint8_t data[FormatterParser::ArgOp::SpecLength[1]] = { 0 };
        const auto opcnt = FormatterParser::ArgOp::EncodeSpec(*spec, data);
        common::span<const uint8_t> ref{ data, opcnt }, src{ ret.Opcodes + idx, opcnt };
        EXPECT_THAT(src, testing::ElementsAreArray(ref));
        idx += static_cast<uint16_t>(opcnt);
    }
}


TEST(Format, ParseString)
{
    {
        constexpr auto ret = FormatterParser::ParseString(""sv);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOpFinish();
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{}"sv);
        CheckSuc();
        CheckIdxArgType(ret, 1, Any);
        uint16_t idx = 0;
        CheckOp(IdxArg, 0, nullptr);
        CheckOpFinish();
    }
    {
        constexpr auto ret = FormatterParser::ParseString("}"sv);
        CheckFail(0, MissingLeftBrace);
        CheckIdxArgCount(ret, 0, 0);
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{"sv);
        CheckFail(0, MissingRightBrace);
        CheckIdxArgCount(ret, 0, 0);
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{:0s}"sv);
        CheckFail(4, IncompNumSpec);
        CheckIdxArgCount(ret, 1, 1); // an arg_id already assigned
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{{"sv);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(Brace, '{');
        CheckOpFinish();
    }
    {
        constexpr auto ret = FormatterParser::ParseString("}}"sv);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(Brace, '}');
        CheckOpFinish();
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{{{{{{"sv);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(Brace, '{');
        CheckOp(Brace, '{');
        CheckOp(Brace, '{');
        CheckOpFinish();
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{{}"sv);
        CheckFail(2, MissingLeftBrace);
        CheckIdxArgCount(ret, 0, 0);
    }
    {
        constexpr auto fmtStr = "Hello"sv;
        constexpr auto ret = FormatterParser::ParseString(fmtStr);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(FmtStr, fmtStr, 0, 5, "Hello"sv);
        CheckOpFinish();
    }
    {
        constexpr auto fmtStr = "Hello{{"sv;
        constexpr auto ret = FormatterParser::ParseString(fmtStr);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(FmtStr, fmtStr, 0, 6, "Hello{"sv);
        CheckOpFinish();
    }
    {
        constexpr auto fmtStr = "Hello{}"sv;
        constexpr auto ret = FormatterParser::ParseString(fmtStr);
        CheckSuc();
        CheckIdxArgType(ret, 1, Any);
        uint16_t idx = 0;
        CheckOp(FmtStr, fmtStr, 0, 5, "Hello"sv);
        CheckOp(IdxArg, 0, nullptr);
        CheckOpFinish();
    }
    {
        constexpr auto fmtStr = "Hello{}{}"sv;
        constexpr auto ret = FormatterParser::ParseString(fmtStr);
        CheckSuc();
        CheckIdxArgType(ret, 2, Any, Any);
        uint16_t idx = 0;
        CheckOp(FmtStr, fmtStr, 0, 5, "Hello"sv);
        CheckOp(IdxArg, 0, nullptr);
        CheckOp(IdxArg, 1, nullptr);
        CheckOpFinish();
    }
    {
        constexpr auto fmtStr = "Hello{}{3}{}{x}"sv;
        constexpr auto ret = FormatterParser::ParseString(fmtStr);
        CheckSuc();
        CheckIdxArgType(ret, 2, Any, Any, Any, Any);
        CheckNamedArgs(ret, fmtStr, (Any, "x"));
        uint16_t idx = 0;
        CheckOp(FmtStr, fmtStr, 0, 5, "Hello"sv);
        CheckOp(IdxArg, 0, nullptr);
        CheckOp(IdxArg, 3, nullptr);
        CheckOp(IdxArg, 1, nullptr);
        CheckOp(NamedArg, 0, nullptr);
        CheckOpFinish();
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{@< }{@>k}{@<B}{@>xff}{@<x06}{@>xbadbad}"sv);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(ColorArg, true,  {});
        CheckOp(ColorArg, false, common::CommonColor::Black);
        CheckOp(ColorArg, true,  common::CommonColor::BrightBlue);
        CheckOp(ColorArg, false, uint8_t(0xff));
        CheckOp(ColorArg, true,  static_cast<common::CommonColor>(0x06));
        std::array<uint8_t, 3> rgb = { 0xba, 0xdb, 0xad };
        CheckOp(ColorArg, false, rgb);
        CheckOpFinish();
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{@}"sv);
        CheckFail(1, InvalidColor);
        CheckIdxArgCount(ret, 0, 0);
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{@<kred}"sv);
        CheckFail(1, InvalidColor);
        CheckIdxArgCount(ret, 0, 0);
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{@#e}"sv);
        CheckFail(1, MissingColorFGBG);
        CheckIdxArgCount(ret, 0, 0);
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{@<x0h}"sv);
        CheckFail(1, Invalid8BitColor);
        CheckIdxArgCount(ret, 0, 0);
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{@<x0f0f7x}"sv);
        CheckFail(1, Invalid24BitColor);
        CheckIdxArgCount(ret, 0, 0);
    }
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wpedantic"
#elif COMMON_COMPILER_CLANG
#   pragma clang diagnostic push
#   if COMMON_CLANG_VER >= 110000
#       pragma clang diagnostic ignored "-Wc++20-extensions"
#   elif COMMON_CLANG_VER >= 100000
#       pragma clang diagnostic ignored "-Wc++2a-extensions"
#   else
#       pragma clang diagnostic ignored "-Wc99-extensions"
#   endif
#endif
    {
        constexpr auto ret = FormatterParser::ParseString("{:d}{:6.3f}{:<#0x}"sv);
        CheckSuc();
        CheckIdxArgType(ret, 3, Integer, Float, Integer);
        uint16_t idx = 0;
        FormatterParser::FormatSpec spec0
        {
            .Type = 'd'
        };
        CheckOp(IdxArg, 0, &spec0);
        FormatterParser::FormatSpec spec1
        {
            .Precision = 3,
            .Width = 6,
            .Type = 'f'
        };
        CheckOp(IdxArg, 1, &spec1);
        FormatterParser::FormatSpec spec2
        {
            .Type = 'x',
            .Alignment = FormatterParser::FormatSpec::Align::Left,
            .AlterForm = true,
            .ZeroPad = true
        };
        CheckOp(IdxArg, 2, &spec2);
        CheckOpFinish();
    }
    {
        constexpr auto fmtStr = "{3:p}xyz{@<K}"sv;
        constexpr auto ret = FormatterParser::ParseString(fmtStr);
        CheckSuc();
        CheckIdxArgType(ret, 0, Any, Any, Any, Pointer);
        uint16_t idx = 0;
        FormatterParser::FormatSpec spec0
        {
            .Type = 'p',
        };
        CheckOp(IdxArg, 3, &spec0);
        CheckOp(FmtStr, fmtStr, 5, 3, "xyz"sv);
        CheckOp(ColorArg, true, common::CommonColor::BrightBlack);
        CheckOpFinish();
    }
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic pop
#elif COMMON_COMPILER_CLANG
#   pragma clang diagnostic pop
#endif
    /*[[maybe_unused]] constexpr auto parse0 = FmtString("{}"sv);
    [[maybe_unused]] constexpr auto parse1 = FmtString("here"sv);
    [[maybe_unused]] constexpr auto parse2 = FmtString("{3:p}xyz{@<K}"sv);*/
    /*constexpr auto klp = []()
    {
        constexpr auto Result = ParseResult::ParseString("");
        constexpr auto OpCount = Result.OpCount;
        constexpr auto NamedArgCount = Result.NamedArgCount;
        constexpr auto IdxArgCount = Result.IdxArgCount;
        ParseResult::CheckErrorCompile<Result.ErrorPos, OpCount>();
        TrimedResult<OpCount, NamedArgCount, IdxArgCount> ret(Result);
        return ret;
    }();*/

}


#define CheckArg(T, type, at, ...) do { SCOPED_TRACE("Check" #T "Arg"); Check##T##Arg_(ret, idx++, #type, ArgRealType::at, __VA_ARGS__); } while(0)
#define CheckArgFinish(T) EXPECT_EQ(ret.T##ArgCount, idx)

void CheckAnArgType(std::string_view tname, ArgRealType real, ArgRealType ref, uint8_t extra)
{
    const auto want = ref | static_cast<ArgRealType>(extra);
    EXPECT_EQ(real, want) << "expects [" << tname.data() << "]";
}
void CheckIdxArg_(const ArgInfo& ret, uint8_t idx, std::string_view tname, ArgRealType type, uint8_t extra)
{
    CheckAnArgType(tname, ret.IndexTypes[idx], type, extra);
}
void CheckNamedArg_(const ArgInfo& ret, uint8_t idx, std::string_view tname, ArgRealType type, uint8_t extra, std::string_view argName)
{
    EXPECT_EQ(ret.Names[idx], argName);
    CheckAnArgType(tname, ret.NamedTypes[idx], type, extra);
}

template<typename... Args>
static constexpr ArgInfo ParseArgs(Args&&...) noexcept
{
    return ArgInfo::ParseArgs<Args...>();
}

struct TypeA
{
    int16_t FormatAs() const noexcept
    {
        return 1;
    }
};
struct TypeB
{};
struct TypeC
{
    void FormatWith(common::str::FormatterExecutor& executor, common::str::FormatterExecutor::Context& context, const common::str::FormatSpec*) const
    {
        executor.PutString(context, "5"sv, nullptr);
    }
};
struct TypeD
{};
namespace common::str
{
template<> inline auto FormatAs<TypeB>(const TypeB&) { return 3.5f; }
template<> inline auto FormatWith<TypeD>(const TypeD&, FormatterExecutor& executor, FormatterExecutor::Context& context, const FormatSpec*)
{ 
    executor.PutString(context, "7.5"sv, nullptr);
}
}

TEST(Format, ParseArg)
{
    {
        constexpr auto ret = ArgInfo::ParseArgs<>();
        EXPECT_EQ(ret.IdxArgCount, static_cast<uint8_t>(0));
        EXPECT_EQ(ret.NamedArgCount, static_cast<uint8_t>(0));
    }
    constexpr auto TmExtra = common::str::detail::tm_has_zone_info ? 0x20 : 0x00;
    {
        using ZonedTime = common::date::zoned_time<std::chrono::seconds>;
        constexpr auto ret = ArgInfo::ParseArgs<int, uint64_t, void*, char*, double, char32_t, std::string_view, 
            TypeA, TypeB, TypeC, TypeD, std::tm, ZonedTime>();
        EXPECT_EQ(ret.NamedArgCount, static_cast<uint8_t>(0));
        {
            uint8_t idx = 0;
            CheckArg(Idx, int,          SInt,   0x20);
            CheckArg(Idx, uint64_t,     UInt,   0x30);
            CheckArg(Idx, void*,        Ptr,    0x10);
            CheckArg(Idx, char*,        String, 0x40);
            CheckArg(Idx, double,       Float,  0x30);
            CheckArg(Idx, char32_t,     Char,   0x20);
            CheckArg(Idx, string_view,  String, 0x00);
            CheckArg(Idx, TypeA,        SInt,   0x10);
            CheckArg(Idx, TypeB,        Float,  0x20);
            CheckArg(Idx, TypeC,        Custom, 0x00);
            CheckArg(Idx, TypeD,        Custom, 0x00);
            CheckArg(Idx, std::tm,      Date,   TmExtra);
            CheckArg(Idx, ZonedTime,    Date,   0x30);
            CheckArgFinish(Idx);
        }
    }
    {
        constexpr void* ptr = nullptr;
        constexpr auto ret = ParseArgs(3, UINT64_MAX, ptr, "x", 1.0, U'x', "x"sv, 
            TypeA{}, TypeB{}, TypeC{}, TypeD{}, std::tm{}, std::chrono::system_clock::time_point{});
        EXPECT_EQ(ret.NamedArgCount, static_cast<uint8_t>(0));
        {
            uint8_t idx = 0;
            CheckArg(Idx, int,          SInt,   0x20);
            CheckArg(Idx, uint64_t,     UInt,   0x30);
            CheckArg(Idx, void*,        Ptr,    0x10);
            CheckArg(Idx, char*,        String, 0x40);
            CheckArg(Idx, double,       Float,  0x30);
            CheckArg(Idx, char32_t,     Char,   0x20);
            CheckArg(Idx, string_view,  String, 0x00);
            CheckArg(Idx, TypeA,        SInt,   0x10);
            CheckArg(Idx, TypeB,        Float,  0x20);
            CheckArg(Idx, TypeC,        Custom, 0x00);
            CheckArg(Idx, TypeD,        Custom, 0x00);
            CheckArg(Idx, std::tm,      Date,   TmExtra);
            CheckArg(Idx, time_point,   Date,   0x10);
            CheckArgFinish(Idx);
        }
    }
    {
        constexpr auto ret = ParseArgs(NAMEARG("x")(13), NAMEARG("y")("y"));
        EXPECT_EQ(ret.IdxArgCount, static_cast<uint8_t>(0));
        {
            uint8_t idx = 0;
            CheckArg(Named, int,         SInt,   0x20, "x");
            CheckArg(Named, const char*, String, 0x40, "y");
            CheckArgFinish(Named);
        }
    }
    {
        constexpr auto ret = ParseArgs(WithName("x", 13), WithName("y", "y"));
        EXPECT_EQ(ret.IdxArgCount, static_cast<uint8_t>(0));
        {
            uint8_t idx = 0;
            CheckArg(Named, int,         SInt,   0x20, "");
            CheckArg(Named, const char*, String, 0x40, "");
            CheckArgFinish(Named);
        }
    }
    {
        constexpr auto ret = ParseArgs(3, NAMEARG("x")(13), UINT64_MAX, NAMEARG("y")("y"), 
            u"x", WithName("x", 13), 1.0f, WithName("y", "y"), uint8_t(3));
        {
            uint8_t idx = 0;
            CheckArg(Idx, int,       SInt,   0x20);
            CheckArg(Idx, uint64_t,  UInt,   0x30);
            CheckArg(Idx, char16_t*, String, 0x50);
            CheckArg(Idx, float,     Float,  0x20);
            CheckArg(Idx, uint8_t,   UInt,   0x00);
            CheckArgFinish(Idx);
        }
        {
            uint8_t idx = 0;
            CheckArg(Named, int,         SInt,   0x20, "x");
            CheckArg(Named, const char*, String, 0x40, "y");
            CheckArg(Named, int,         SInt,   0x20, "");
            CheckArg(Named, const char*, String, 0x40, "");
            CheckArgFinish(Named);
        }
    }
}

using ArgTypePair = std::pair<ArgDispType, ArgRealType>;
TEST(Format, CheckArg)
{
    {
        ArgChecker::CheckSS(FmtString("{},{}"sv), 1234, "str");
        EXPECT_NO_THROW(ArgChecker::CheckDS(FmtString("{},{}"sv), 1234, "str"));
    }
    {
        //ArgChecker::CheckSS(FmtString("{3},{}"), 1234, "str");
        try
        {
            ArgChecker::CheckDS(FmtString("{3},{}"sv), 1234, "str");
            EXPECT_TRUE(false) << "should throw exception";
        }
        catch (const ArgMismatchException& ame)
        {
            EXPECT_TRUE(true) << common::str::to_string(ame.Message());
        }
    }
    {
        //ArgChecker::CheckSS(FmtString("{},{:f}"), 1234, "str");
        try
        {
            ArgChecker::CheckDS(FmtString("{},{:f}"sv), 1234, "str");
        }
        catch (const ArgMismatchException& ame)
        {
            EXPECT_TRUE(true) << common::str::to_string(ame.Message());
            EXPECT_EQ(ame.GetReason(), ArgMismatchException::Reasons::IndexArgTypeMismatch);
            EXPECT_EQ(ame.GetMismatchType(), (std::pair{ ArgDispType::Float, ArgRealType::String }));
            const auto target = ame.GetMismatchTarget();
            ASSERT_EQ(target.index(), 1u);
            EXPECT_EQ(std::get<1>(target), 1u);
        }
    }
    {
        //CheckSS(FmtString("{}{x}"), 1234, "str");
        try
        {
            ArgChecker::CheckDS(FmtString("{}{x}"sv), 1234, "str");
        }
        catch (const ArgMismatchException& ame)
        {
            EXPECT_TRUE(true) << common::str::to_string(ame.Message());
        }
    }
    {
        //CheckSS(FmtString("{}{x}"), 1234, "str");
        try
        {
            ArgChecker::CheckDS(FmtString("{}{x:s}"sv), 1234, "str", NAMEARG("x")(13));
        }
        catch (const ArgMismatchException& ame)
        {
            EXPECT_TRUE(true) << common::str::to_string(ame.Message());
            EXPECT_EQ(ame.GetReason(), ArgMismatchException::Reasons::NamedArgTypeMismatch);
            EXPECT_EQ(ame.GetMismatchType(), (std::pair{ ArgDispType::String, ArgRealType::SInt }));
            const auto target = ame.GetMismatchTarget();
            ASSERT_EQ(target.index(), 2u);
            EXPECT_EQ(std::get<2>(target), "x"sv);
        }
    }
    {
        ArgChecker::CheckSS(FmtString("{}{x}"sv), 1234, "str", NAMEARG("x")(13));
        EXPECT_NO_THROW(ArgChecker::CheckDS(FmtString("{}{x}"sv), 1234, "str", NAMEARG("x")(13)));
    }
}

template<typename T>
void CheckPackedArg(common::span<const uint16_t> argStore, uint16_t idx, const T& val)
{
    const auto offset = argStore[idx];
    const auto ptr = reinterpret_cast<const T*>(&argStore[offset]);
    EXPECT_EQ(*ptr, val);
}

TEST(Format, PackArg)
{
    std::string_view var0 = "x"sv;
    uint64_t var1 = 256;
    int16_t var2 = -8;
    const auto var3 = U"y";
    {
        const auto store = ArgInfo::PackArgsStatic(var0, var1, var2, var3);
        ASSERT_GT(store.ArgStore.size(), 4u);
        const auto var0_ = std::pair{var0.data(), var0.size()};
        CheckPackedArg(store.ArgStore, 0, var0_);
        CheckPackedArg(store.ArgStore, 1, var1);
        CheckPackedArg(store.ArgStore, 2, var2);
        CheckPackedArg(store.ArgStore, 3, var3);
    }
}


template<typename T, typename... Args>
auto ToString(T&& res, Args&&... args)
{
    using Char = typename std::decay_t<T>::CharType;
    Formatter<Char> formatter;
    return formatter.FormatStatic(res, std::forward<Args>(args)...);
}
TEST(Format, Formating)
{
    {
        const auto ref = fmt::format("{},{x},{:>6},{:_^7}", "hello", "Hello", "World"sv, fmt::arg("x", "world"));
        const auto ret = ToString(FmtString("{},{x},{:>6},{:_^7}"sv), "hello", NAMEARG("x")("world"sv), u"Hello", U"World"sv);
        EXPECT_EQ(ret, ref);
        EXPECT_EQ(ret, "hello,world, Hello,_World_");
    }
    {
        const auto ref = fmt::format("{},{x},{:b},{:#X},{:05o}", 13, uint8_t(64), int64_t(65535), 042, fmt::arg("x", -99));
        const auto ret = ToString(FmtString("{},{x},{:b},{:#X},{:05o}"sv), 13, NAMEARG("x")(-99), uint8_t(64), int64_t(65535), 042);
        EXPECT_EQ(ret, ref);
        EXPECT_EQ(ret, "13,-99,1000000,0XFFFF,00042");
    }
    {
        const auto ref = fmt::format("{},{x},{:g},{:f},{:+010.4g}", 0.0, 4.9014e6, -392.5f, 392.65, fmt::arg("x", 392.65));
        const auto ret = ToString(FmtString("{},{x},{:g},{:f},{:+010.4g}"sv), 0.0, NAMEARG("x")(392.65), 4.9014e6, -392.5f, 392.65);
        EXPECT_EQ(ret, ref);
        EXPECT_EQ(ret, "0,392.65,4.9014e+06,-392.500000,+0000392.6");
    }
    {
        const auto ptr = reinterpret_cast<const int*>(uintptr_t(1));
        const auto ref = fmt::format("{},{}", (void*)ptr, false);
        const auto ret = ToString(FmtString("{},{}"sv), ptr, false);
        EXPECT_EQ(ret, ref);
        EXPECT_EQ(ret, "0x1,false");
    }
    {
        const auto ref = fmt::format("{:d},{:d},{},{:#X}", true, (uint16_t)u'a', false, 'c');
        const auto ret = ToString(FmtString("{:d},{:d},{},{:#X}"sv), true, u'a', false, 'c');
        EXPECT_EQ(ret, ref);
        EXPECT_EQ(ret, "1,97,false,0X63");
    }
    {
        const auto ref = fmt::format(u"{},{},{},{},{x},{:g},{:f},{:+010.4g}"sv,
            u"hello", u"Hello", u"World"sv, 0.0, 4.9014e6, -392.5f, 392.65, fmt::arg(u"x", 392.65));
        const auto ret = ToString(FmtString(u"{},{},{},{},{x},{:g},{:f},{:+010.4g}"sv),
            "hello", u"Hello", U"World"sv, 0.0, NAMEARG("x")(392.65), 4.9014e6, -392.5f, 392.65);
        EXPECT_EQ(ret, ref);
        EXPECT_EQ(ret, u"hello,Hello,World,0,392.65,4.9014e+06,-392.500000,+0000392.6");
    }
    {
        const auto ret = ToString(FmtString("{},{}"sv), TypeC{}, TypeD{});
        EXPECT_EQ(ret, "5,7.5");
    }
    {
        const auto t1 = common::SimpleTimer::getCurLocalTime();
        const auto t2 = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
        constexpr auto GetFmtTime = [](const common::date::zoned_time<std::chrono::microseconds>& t)
            {
                const auto ticks = t.get_local_time().time_since_epoch();
                return std::chrono::sys_time<std::chrono::microseconds>{ ticks };
            };
        const auto t3 = common::date::zoned_time<std::chrono::microseconds>{ common::date::current_zone(), t2 }; 
        const auto ref = fmt::format(std::locale::classic(), "{0:%Y-%m-%dT%H:%M:%S}|{0:%Y-%m-%dT%H:%M:%S}|{1:%Y-%m-%d %H:%M:%S}|{2:%Y-%m-%d %j=%b %U-%w %a %H:%M:%S}"sv, t1, t2, GetFmtTime(t3));
        const auto ret = ToString(FmtString("{}|{0:T}|{1:T%Y-%m-%d %H:%M:%S}|{2:T%Y-%m-%d %j=%b %U-%w %a %H:%M:%S}"sv), t1, t2, t3);
        EXPECT_EQ(ret, ref);
    }
}


#if defined(NDEBUG)
TEST(Format, Perf)
{
    constexpr uint32_t times = 100000;
    common::SimpleTimer timer;
    std::string ref[4];
    {
        uint64_t timens = 0;
        for (uint32_t i = 0; i < times; ++i)
        {
            ref[0].clear();
            timer.Start();
            fmt::format_to(std::back_inserter(ref[0]), FMT_STRING("123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}"),
                "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765));
            timer.Stop();
            timens += timer.ElapseNs();
        }
        TestCout() << "[fmt-def] u8 use avg[" << (timens / times) << "]ns to finish\n";
    }
    {
        uint64_t timens = 0;
        for (uint32_t i = 0; i < times; ++i)
        {
            ref[1].clear();
            timer.Start();
            fmt::format_to(std::back_inserter(ref[1]), FMT_COMPILE("123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}"),
                "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765));
            timer.Stop();
            timens += timer.ElapseNs();
        }
        TestCout() << "[fmt-cpl] u8 use avg[" << (timens / times) << "]ns to finish\n";
        EXPECT_TRUE(ref[1] == ref[0]);
    }
    {
        uint64_t timens = 0;
        for (uint32_t i = 0; i < times; ++i)
        {
            ref[2].clear();
            timer.Start();
            fmt::format_to(std::back_inserter(ref[2]), fmt::runtime("123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}"),
                "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765));
            timer.Stop();
            timens += timer.ElapseNs();
        }
        TestCout() << "[fmt-rt ] u8 use avg[" << (timens / times) << "]ns to finish\n";
        EXPECT_TRUE(ref[2] == ref[0]);
    }
    {
        uint64_t timens = 0;
        for (uint32_t i = 0; i < times; ++i)
        {
            ref[3].clear();
            timer.Start();
            fmt::dynamic_format_arg_store<fmt::buffer_context<char>> store;
            store.push_back("hello");
            store.push_back(0.0);
            store.push_back(42);
            store.push_back(uint8_t(64));
            store.push_back(int64_t(65535));
            store.push_back(-70000);
            store.push_back(4.9014e6);
            store.push_back(-392.5f);
            store.push_back(392.65);
            store.push_back(uint64_t(765));
            fmt::vformat_to(std::back_inserter(ref[3]), "123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}", store);
            timer.Stop();
            timens += timer.ElapseNs();
        }
        TestCout() << "[fmt-dyn] u8 use avg[" << (timens / times) << "]ns to finish\n";
        EXPECT_TRUE(ref[3] == ref[0]);
    }


    std::string csf[3];
    {
        uint64_t timens = 0;
        for (uint32_t i = 0; i < times; ++i)
        {
            csf[0].clear();
            timer.Start();
            Formatter<char> fmter;
            fmter.FormatToDynamic(csf[0], "123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}",
                "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765));
            timer.Stop();
            timens += timer.ElapseNs();
        }
        TestCout() << "[csf-dyn] u8 use avg[" << (timens / times) << "]ns to finish\n";
        EXPECT_TRUE(csf[0] == ref[0]);
    }
    {
        uint64_t timens = 0;
        for (uint32_t i = 0; i < times; ++i)
        {
            csf[1].clear();
            timer.Start();
            Formatter<char> fmter;
            fmter.FormatToStatic(csf[1], FmtString("123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}"),
                "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765));
            timer.Stop();
            timens += timer.ElapseNs();
        }
        TestCout() << "[csf-sta] u8 use avg[" << (timens / times) << "]ns to finish\n";
        EXPECT_TRUE(csf[1] == csf[0]);
    }
    {
        uint64_t timens = 0;
        auto cachedFmter = FormatSpecCacher::CreateFrom(FmtString("123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}"),
            "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765));
        for (uint32_t i = 0; i < times; ++i)
        {
            csf[2].clear();
            timer.Start();
            cachedFmter.Format(csf[2], "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765));
            timer.Stop();
            timens += timer.ElapseNs();
        }
        TestCout() << "[csf-cah] u8 use avg[" << (timens / times) << "]ns to finish\n";
        EXPECT_TRUE(csf[2] == csf[0]);
    }

}
#endif
