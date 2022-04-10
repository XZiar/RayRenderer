#include "rely.h"
#include <algorithm>
#include "SystemCommon/Format.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/StringConvert.h"

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace common::str::exp;
using common::BaseException;


#define CheckSuc() EXPECT_EQ(ret.ErrorPos, UINT16_MAX)

#define CheckFail(pos, err) do                                                      \
{                                                                                   \
    EXPECT_EQ(ret.ErrorPos,  static_cast<uint16_t>(pos));                           \
    EXPECT_EQ(ret.OpCount,   static_cast<uint16_t>(ParseResult::ErrorCode::err));   \
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

#define EXPECT_EX_RES(ex, type, key, val) do    \
{                                               \
    const auto dat = ex.GetResource<type>(key); \
    EXPECT_NE(dat, nullptr);                    \
    if (dat) { EXPECT_EQ(*dat, val); }          \
} while(0)

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

template<typename Char>
static void CheckFmtStrOp_(const ParseResult& ret, uint16_t& idx, std::basic_string_view<Char> fmtStr, uint16_t offset, uint16_t length, std::basic_string_view<Char> str)
{
    ASSERT_EQ(fmtStr.substr(offset, length), str);
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
        const auto opcnt = ParseResult::ArgOp::EncodeSpec(*spec, data);
        common::span<const uint8_t> ref{ data, opcnt }, src{ ret.Opcodes + idx, opcnt };
        EXPECT_THAT(src, testing::ContainerEq(ref));
        idx += opcnt;
    }
}

static void CheckNamedArgOp_(const ParseResult& ret, uint16_t& idx, uint8_t argidx, const ParseResult::FormatSpec* spec)
{
    const uint8_t field = ParseResult::ArgOp::FieldNamed | (spec ? ParseResult::ArgOp::FieldHasSpec : 0);
    CheckOpCode(ret.Opcodes[idx++], ParseResult::ArgOp::Op | field);
    EXPECT_EQ(ret.Opcodes[idx++], argidx);
    if (spec)
    {
        uint8_t data[12] = { 0 };
        const auto opcnt = ParseResult::ArgOp::EncodeSpec(*spec, data);
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
        constexpr auto ret = ParseResult::ParseString("{:0s}"sv);
        CheckFail(4, IncompNumSpec);
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
        constexpr auto fmtStr = "Hello"sv;
        constexpr auto ret = ParseResult::ParseString(fmtStr);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(FmtStr, fmtStr, 0, 5, "Hello"sv);
        CheckOpFinish();
    }
    {
        constexpr auto fmtStr = "Hello{{"sv;
        constexpr auto ret = ParseResult::ParseString(fmtStr);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(FmtStr, fmtStr, 0, 6, "Hello{"sv);
        CheckOpFinish();
    }
    {
        constexpr auto fmtStr = "Hello{}"sv;
        constexpr auto ret = ParseResult::ParseString(fmtStr);
        CheckSuc();
        CheckIdxArgType(ret, 1, Any);
        uint16_t idx = 0;
        CheckOp(FmtStr, fmtStr, 0, 5, "Hello"sv);
        CheckOp(IdxArg, 0, nullptr);
        CheckOpFinish();
    }
    {
        constexpr auto fmtStr = "Hello{}{}"sv;
        constexpr auto ret = ParseResult::ParseString(fmtStr);
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
        constexpr auto ret = ParseResult::ParseString(fmtStr);
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
        constexpr auto ret = ParseResult::ParseString("{@<default}{@>black}{@<blue+}{@>bff}{@<b06}{@>badbad}"sv);
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
        constexpr auto ret = ParseResult::ParseString("{@}"sv);
        CheckFail(1, InvalidColor);
        CheckIdxArgCount(ret, 0, 0);
    }
    {
        constexpr auto ret = ParseResult::ParseString("{@<kred}"sv);
        CheckFail(1, InvalidColor);
        CheckIdxArgCount(ret, 0, 0);
    }
    {
        constexpr auto ret = ParseResult::ParseString("{@<kblack}"sv);
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
    {
        constexpr auto fmtStr = "{3:p}xyz{@<black+}"sv;
        constexpr auto ret = ParseResult::ParseString(fmtStr);
        CheckSuc();
        CheckIdxArgType(ret, 0, Any, Any, Any, Pointer);
        uint16_t idx = 0;
        ParseResult::FormatSpec spec0
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
    [[maybe_unused]] constexpr auto parse0 = FmtString("{}"sv);
    [[maybe_unused]] constexpr auto parse1 = FmtString("here"sv);
    [[maybe_unused]] constexpr auto parse2 = FmtString("{3:p}xyz{@<black+}"sv);
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
TEST(Format, ParseArg)
{
    {
        constexpr auto ret = ArgInfo::ParseArgs<>();
        EXPECT_EQ(ret.IdxArgCount, static_cast<uint8_t>(0));
        EXPECT_EQ(ret.NamedArgCount, static_cast<uint8_t>(0));
    }
    {
        constexpr auto ret = ArgInfo::ParseArgs<int, uint64_t, void*, char*, double, char32_t, std::string_view>();
        EXPECT_EQ(ret.NamedArgCount, static_cast<uint8_t>(0));
        {
            uint8_t idx = 0;
            CheckArg(Idx, int,          SInt,   0x2);
            CheckArg(Idx, uint64_t,     UInt,   0x3);
            CheckArg(Idx, void*,        Ptr,    0x1);
            CheckArg(Idx, char*,        String, 0x4);
            CheckArg(Idx, double,       Float,  0x3);
            CheckArg(Idx, char32_t,     Char,   0x2);
            CheckArg(Idx, string_view,  String, 0x0);
            CheckArgFinish(Idx);
        }
    }
    {
        constexpr void* ptr = nullptr;
        constexpr auto ret = ParseArgs(3, UINT64_MAX, ptr, "x", 1.0, U'x', "x"sv);
        EXPECT_EQ(ret.NamedArgCount, static_cast<uint8_t>(0));
        {
            uint8_t idx = 0;
            CheckArg(Idx, int,          SInt,   0x2);
            CheckArg(Idx, uint64_t,     UInt,   0x3);
            CheckArg(Idx, void*,        Ptr,    0x1);
            CheckArg(Idx, char*,        String, 0x4);
            CheckArg(Idx, double,       Float,  0x3);
            CheckArg(Idx, char32_t,     Char,   0x2);
            CheckArg(Idx, string_view,  String, 0x0);
            CheckArgFinish(Idx);
        }
    }
    {
        constexpr auto ret = ParseArgs(NAMEARG("x")(13), NAMEARG("y")("y"));
        EXPECT_EQ(ret.IdxArgCount, static_cast<uint8_t>(0));
        {
            uint8_t idx = 0;
            CheckArg(Named, int,         SInt,   0x2, "x");
            CheckArg(Named, const char*, String, 0x4, "y");
            CheckArgFinish(Named);
        }
    }
    {
        constexpr auto ret = ParseArgs(WithName("x", 13), WithName("y", "y"));
        EXPECT_EQ(ret.IdxArgCount, static_cast<uint8_t>(0));
        {
            uint8_t idx = 0;
            CheckArg(Named, int,         SInt,   0x2, "");
            CheckArg(Named, const char*, String, 0x4, "");
            CheckArgFinish(Named);
        }
    }
    {
        constexpr auto ret = ParseArgs(3, NAMEARG("x")(13), UINT64_MAX, NAMEARG("y")("y"), 
            u"x", WithName("x", 13), 1.0f, WithName("y", "y"), uint8_t(3));
        {
            uint8_t idx = 0;
            CheckArg(Idx, int,       SInt,   0x2);
            CheckArg(Idx, uint64_t,  UInt,   0x3);
            CheckArg(Idx, char16_t*, String, 0x5);
            CheckArg(Idx, float,     Float,  0x2);
            CheckArg(Idx, uint8_t,   UInt,   0x0);
            CheckArgFinish(Idx);
        }
        {
            uint8_t idx = 0;
            CheckArg(Named, int,         SInt,   0x2, "x");
            CheckArg(Named, const char*, String, 0x4, "y");
            CheckArg(Named, int,         SInt,   0x2, "");
            CheckArg(Named, const char*, String, 0x4, "");
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
        catch (const BaseException& be)
        {
            EXPECT_TRUE(true) << common::str::to_string(be.Message());
        }
    }
    {
        //ArgChecker::CheckSS(FmtString("{},{:f}"), 1234, "str");
        try
        {
            ArgChecker::CheckDS(FmtString("{},{:f}"sv), 1234, "str");
        }
        catch (const BaseException& be)
        {
            EXPECT_TRUE(true) << common::str::to_string(be.Message());
            EXPECT_EX_RES(be, uint32_t, "arg"sv, 1u);
            EXPECT_EX_RES(be, ArgTypePair, "argType"sv, (std::pair{ ArgDispType::Float, ArgRealType::String }));
        }
    }
    {
        //CheckSS(FmtString("{}{x}"), 1234, "str");
        try
        {
            ArgChecker::CheckDS(FmtString("{}{x}"sv), 1234, "str");
        }
        catch (const BaseException& be)
        {
            EXPECT_TRUE(true) << common::str::to_string(be.Message());
        }
    }
    {
        //CheckSS(FmtString("{}{x}"), 1234, "str");
        try
        {
            ArgChecker::CheckDS(FmtString("{}{x:s}"sv), 1234, "str", NAMEARG("x")(13));
        }
        catch (const BaseException& be)
        {
            EXPECT_TRUE(true) << common::str::to_string(be.Message());
            EXPECT_EX_RES(be, std::string_view, "arg"sv, "x"sv);
            EXPECT_EX_RES(be, ArgTypePair, "argType"sv, (std::pair{ ArgDispType::String, ArgRealType::SInt }));
        }
    }
    {
        ArgChecker::CheckSS(FmtString("{}{x}"sv), 1234, "str", NAMEARG("x")(13));
        EXPECT_NO_THROW(ArgChecker::CheckDS(FmtString("{}{x}"sv), 1234, "str", NAMEARG("x")(13)));
    }
}

template<typename T>
void CheckPackedArg(const ArgPack& pack, uint16_t idx, const T& val)
{
    const auto offset = pack.Args[idx];
    const auto ptr = reinterpret_cast<const T*>(&pack.Args[offset]);
    EXPECT_EQ(*ptr, val);
}

TEST(Format, PackArg)
{
    std::string_view var0 = "x"sv;
    uint64_t var1 = 256;
    int16_t var2 = -8;
    const auto var3 = U"y";
    {
        const auto pack = ArgInfo::PackArgs(var0, var1, var2, var3);
        ASSERT_GT(pack.Args.size(), 4u);
        const auto var0_ = std::pair{var0.data(), var0.size()};
        CheckPackedArg(pack, 0, var0_);
        CheckPackedArg(pack, 1, var1);
        CheckPackedArg(pack, 2, var2);
        CheckPackedArg(pack, 3, var3);
    }
}


template<typename T, typename... Args>
std::string ToString(T&& strInfo, Args&&... args)
{
    constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
    const auto mapping = ArgChecker::CheckDD(strInfo, ArgsInfo);
    auto argPack = ArgInfo::PackArgs(std::forward<Args>(args)...);
    argPack.Mapper = mapping;
    std::string ret;
    FormatterBase::FormatTo(Formatter{}, ret, strInfo, ArgsInfo, argPack);
    return ret;
}
TEST(Format, Formating)
{
    {
        const auto ret = ToString(FmtString("{},{x},{:>6},{:_^7}"sv), "hello", NAMEARG("x")("world"sv), u"Hello", U"World"sv);
        EXPECT_EQ(ret, "hello,world, Hello,_World_");
    }
    {
        const auto ret = ToString(FmtString("{},{x},{:b},{:#X},{:05o}"sv), 13, NAMEARG("x")(-99), uint8_t(64), int64_t(65535), 042);
        EXPECT_EQ(ret, "13,-99,1000000,0XFFFF,00042");
    }
    {
        const auto ret = ToString(FmtString("{},{x},{:g},{:f},{:+010.4g}"sv), 0.0, NAMEARG("x")(392.65), 4.9014e6, -392.5f, 392.65);
        EXPECT_EQ(ret, "0,392.65,4.9014e+06,-392.500000,+0000392.6");
    }
    {
        const auto ptr = reinterpret_cast<const int*>(uintptr_t(1));
        const auto ret = ToString(FmtString("{},{}"sv), ptr, false);
        EXPECT_EQ(ret, "0x1,false");
    }
}

