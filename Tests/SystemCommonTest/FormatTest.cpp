#include "rely.h"
#include "common/StringLinq.hpp"
#include "common/simd/SIMD128.hpp" // to enable SIMD for Format
#include "SystemCommon/Format.h"
#include "SystemCommon/FormatExtra.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/StringFormat.h"
#include "3rdParty/fmt/include/fmt/compile.h"

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <random>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace common::str;
using common::BaseException;


static void TestValLen(const uint32_t val, const uint32_t encodeSize, const uint32_t encodeLen, const uint8_t byte0, const uint8_t byte1, const uint8_t byte2, const uint8_t byte3)
{
    uint8_t tmp[4] = { 0 };
    uint32_t idx = 0;
    const auto lenVal = FormatterParser::ArgOp::EncodeValLenTo(val, tmp, idx);
    EXPECT_EQ(idx, encodeSize);
    EXPECT_EQ(lenVal, encodeLen);
    EXPECT_THAT(tmp, testing::ElementsAre(byte0, byte1, byte2, byte3));
    SpecReader reader(tmp);
    const auto readVal = reader.ReadLengthedVal(lenVal, 0xabadbeefu);
    EXPECT_EQ(readVal, val);
    // EXPECT_EQ(reader.SpecSize, encodeSize);
}

TEST(Format, Utility)
{
    EXPECT_EQ(FormatterParser::CheckAlign('<'), FormatSpec::Align::Left);
    EXPECT_EQ(FormatterParser::CheckAlign('>'), FormatSpec::Align::Right);
    EXPECT_EQ(FormatterParser::CheckAlign('^'), FormatSpec::Align::Middle);
    for (uint64_t i = 0; i <= 0x10ffffu; ++i)
    {
        switch (i)
        {
        case '<': [[fallthrough]];
        case '>': [[fallthrough]];
        case '^': continue;
        default: break;
        }
        if (FormatterParser::CheckAlign(static_cast<uint32_t>(i)) != FormatSpec::Align::None)
        {
            EXPECT_EQ(FormatterParser::CheckAlign(static_cast<uint32_t>(i)), FormatSpec::Align::None) << "at" << i;
        }
    }

    EXPECT_EQ(FormatterParser::CheckSign('+'), FormatSpec::Sign::Pos);
    EXPECT_EQ(FormatterParser::CheckSign('-'), FormatSpec::Sign::Neg);
    EXPECT_EQ(FormatterParser::CheckSign(' '), FormatSpec::Sign::Space);
    for (uint64_t i = 0; i <= 0x10ffffu; ++i)
    {
        switch (i)
        {
        case '+': [[fallthrough]];
        case '-': [[fallthrough]];
        case ' ': continue;
        default: break;
        }
        if (FormatterParser::CheckSign(static_cast<uint32_t>(i)) != FormatSpec::Sign::None)
        {
            EXPECT_EQ(FormatterParser::CheckSign(static_cast<uint32_t>(i)), FormatSpec::Sign::None) << "at" << i;
        }
    }

    {
#define EncodeValLenTest(val, size, out, val0, val1, val2, val3) \
do { SCOPED_TRACE("TestValLen 0x" #val); TestValLen(0x##val##u, size, out, 0x##val0##u, 0x##val1##u, 0x##val2##u, 0x##val3##u); } while(0)

        EncodeValLenTest(       0, 1u, 1u, 00, 00, 00, 00);
        EncodeValLenTest(       1, 1u, 1u, 01, 00, 00, 00);
        EncodeValLenTest(      ff, 1u, 1u, ff, 00, 00, 00);
        EncodeValLenTest(     100, 2u, 2u, 00, 01, 00, 00);
        EncodeValLenTest(     103, 2u, 2u, 03, 01, 00, 00);
        EncodeValLenTest(    c90f, 2u, 2u, 0f, c9, 00, 00);
        EncodeValLenTest(   1c09f, 4u, 3u, 9f, c0, 01, 00);
        EncodeValLenTest(  fecba9, 4u, 3u, a9, cb, fe, 00);
        EncodeValLenTest(78fecba9, 4u, 3u, a9, cb, fe, 78);

#undef EncodeValLenTest
    }

#define ParseDecTailTest(str, limit, suc, output, err) do                               \
{                                                                                       \
    std::string_view txt = str;                                                         \
    uint32_t val = txt[0] - '0';                                                        \
    const auto [errIdx, inRange] = FormatterParserCh<char>::template ParseDecTail<limit>\
        (val, txt.data(), static_cast<uint32_t>(txt.size()));                           \
    EXPECT_EQ(inRange, suc);                                                            \
    EXPECT_EQ(val, output);                                                             \
    EXPECT_EQ(errIdx, err);                                                             \
} while(0)

    ParseDecTailTest("6553", 65536,  true, 6553u, 4u);
    ParseDecTailTest("6553", 6553,   true, 6553u, 4u);
    ParseDecTailTest("6553", 6552,  false,  655u, 2u);
    ParseDecTailTest("6553", 655,   false,  655u, 2u);
    ParseDecTailTest("6553", 654,   false,   65u, 1u);
    ParseDecTailTest("6553", 65,    false,   65u, 1u);
    ParseDecTailTest("6553", 64,    false,    6u, 0u);
    ParseDecTailTest("655x", 6552,   true,  655u, 3u);
    ParseDecTailTest("6x53", 6553,   true,    6u, 1u);
    ParseDecTailTest("65x3", 65536,  true,   65u, 2u);

#undef ParseDecTailTest
}


template<typename T>
static void TestSearchFirstBrace(std::mt19937& gen)
{
    constexpr uint32_t size = 65536u;
    static constexpr T checker[2] = { '{', '}' };
    constexpr std::basic_string_view<T> target{ checker, 2 };
    std::vector<T> data;
    data.reserve(size);
    for (uint32_t i = 0; i < size; ++i)
    {
        auto val = gen() % 256; // only test within 256
        // force a near search to happen
        if (i == size - 3)
            val = '}';
        data.push_back(static_cast<T>(val)); 
    }
    std::basic_string_view<T> txt{ data.data(), size };
    for (uint32_t idx = 0; idx < size;)
    {
        const auto beginOffset = idx;
        auto idx2 = idx;
        const auto pos = txt.find_first_of(target, idx);
        bool isLB = false, isLB2 = false;
        const auto findBrace  = FormatterParserCh<T>::template LocateFirstBrace<true >(data.data(), idx, isLB, size);
        const auto findBrace2 = FormatterParserCh<T>::template LocateFirstBrace<false>(data.data(), idx2, isLB2, size);
        EXPECT_EQ(findBrace, findBrace2) << "at: " << beginOffset;
        EXPECT_EQ(idx, idx2) << "at: " << beginOffset;
        if (findBrace)
        {
            EXPECT_EQ(isLB, isLB2) << "at: " << beginOffset;
        }
        EXPECT_LE(idx, size) << "mush not exceed size";
        if (pos != std::string_view::npos) // find it
        {
            EXPECT_TRUE(findBrace) << "at: " << beginOffset;
            EXPECT_EQ(idx, pos) << "at: " << beginOffset;
            EXPECT_EQ(isLB, txt[pos] == '{') << "at: " << beginOffset;
            idx++; // for next search
        }
        else
        {
            EXPECT_FALSE(findBrace) << "at: " << beginOffset;
            EXPECT_EQ(idx, size) << "not found should give idx reach to size";
        }
    }
}
template<typename T>
static void TestSearchColonRB(std::mt19937& gen)
{
    constexpr uint32_t size = 65536u;
    std::vector<T> data;
    data.reserve(size);
    for (uint32_t i = 0; i < size; ++i)
    {
        auto val = gen() % 256; // only test within 256
        // force a near search to happen
        if (i == size - 3)
            val = ':';
        else if (i == size - 2)
            val = '}';
        data.push_back(static_cast<T>(val));
    }
    std::basic_string_view<T> txt{ data.data(), size };
    bool setSplit = false;
    for (uint32_t idx = 0; idx < size;)
    {
        const auto beginOffset = idx;
        auto idx2 = idx;
        const auto posColon = txt.find_first_of(static_cast<T>(':'), idx);
        const auto pos = txt.find_first_of(static_cast<T>('}'), idx);
        uint32_t splitOffset = setSplit ? 1u : 0u;
        uint32_t splitOffset2 = splitOffset;
        const auto findRB  = FormatterParserCh<T>::template LocateColonAndRightBrace<true> (data.data(), idx, splitOffset, size);
        const auto findRB2 = FormatterParserCh<T>::template LocateColonAndRightBrace<false>(data.data(), idx2, splitOffset2, size);
        EXPECT_EQ(findRB, findRB2) << "at: " << beginOffset;
        EXPECT_EQ(idx, idx2) << "at: " << beginOffset;
        if (findRB)
        {
            EXPECT_EQ(splitOffset, splitOffset2) << "at: " << beginOffset;
        }
        EXPECT_LE(idx, size) << "mush not exceed size";
        if (pos != std::string_view::npos) // find it
        {
            EXPECT_TRUE(findRB) << "at: " << beginOffset;
            EXPECT_EQ(idx, pos) << "at: " << beginOffset;
            if (!setSplit)
            {
                if (posColon < pos) // get colon
                    EXPECT_EQ(splitOffset, posColon) << "at: " << beginOffset;
                else // no colon, should be idx
                    EXPECT_EQ(splitOffset, idx) << "at: " << beginOffset;
            }
            idx++; // for next search
        }
        else
        {
            EXPECT_FALSE(findRB) << "at: " << beginOffset;
            EXPECT_EQ(idx, size) << "not found should give idx reach to size";
        }
        setSplit = !setSplit;
    }
}
TEST(Format, ParseSearch)
{
    std::mt19937 gen{}; // use default constructor
    {
        SCOPED_TRACE("Search 1B");
        TestSearchFirstBrace<char>(gen);
        TestSearchColonRB<char>(gen);
    }
    {
        SCOPED_TRACE("Search 2B");
        TestSearchFirstBrace<char16_t>(gen);
        TestSearchColonRB<char16_t>(gen);
    }
    {
        SCOPED_TRACE("Search 4B");
        TestSearchFirstBrace<char32_t>(gen);
        TestSearchColonRB<char32_t>(gen);
    }
}


#define ArgIdTesterArg [[maybe_unused]] ParseResultCommon& result, [[maybe_unused]] std::string_view str, [[maybe_unused]] std::string_view part, uint32_t retcode, uint8_t argIdx, ArgDispType* dstType
using TestArgIdTester = void(*)(ArgIdTesterArg);
template<size_t N>
static void TestArgId(std::string_view str, const TestArgIdTester(&tester)[N])
{
    ParseResultCommon result;
    uint32_t partIdx = 0;
    for (const auto part : common::str::SplitStream(str, ',', false))
    {
        if (partIdx >= N)
        {
            EXPECT_TRUE(partIdx < N) << "too few tester for given string: " << str;
            break;
        }
        auto msg = std::string("for part [").append(std::to_string(partIdx)).append("]: [").append(part).append("]");
        testing::ScopedTrace trace("", 0, msg);
        const auto firstCh = part[0];
        uint8_t argIdx = 0;
        ArgDispType* dstType = nullptr;
        result.ErrorPos = result.ErrorNum = 0; // clear error
        const auto ret = FormatterParserCh<char>::ParseArgId(result, firstCh, argIdx, dstType,
            str.data(), static_cast<uint32_t>(part.data() - str.data()), static_cast<uint32_t>(part.size()));
        tester[partIdx++](result, str, part, ret, argIdx, dstType);
    }
}

TEST(Format, ParseArgId)
{
    static constexpr auto IdxTester = [](uint32_t idx, ArgIdTesterArg)
    {
        EXPECT_EQ(retcode, 0u);
        EXPECT_EQ(argIdx, idx);
        EXPECT_EQ(dstType, nullptr);
        EXPECT_EQ(result.ErrorPos, static_cast<uint16_t>(0));
        EXPECT_EQ(result.ErrorNum, static_cast<uint16_t>(0));
    };
    static constexpr auto NamedTester = [](uint32_t idx, std::string_view name, uint32_t namedCount, ArgIdTesterArg)
    {
        ASSERT_EQ(part, name);
        EXPECT_EQ(retcode, 0u);
        EXPECT_EQ(argIdx, idx);
        EXPECT_EQ(dstType, &result.NamedTypes[argIdx].Type);
        EXPECT_EQ(part, str.substr(result.NamedTypes[argIdx].Offset, result.NamedTypes[argIdx].Length));
        EXPECT_EQ(result.NamedArgCount, namedCount);
        EXPECT_EQ(result.ErrorPos, static_cast<uint16_t>(0));
        EXPECT_EQ(result.ErrorNum, static_cast<uint16_t>(0));
    };
    static constexpr auto FailedTester = [](uint32_t pos, ParseResultBase::ErrorCode err, ArgIdTesterArg)
    {
        EXPECT_EQ(retcode, 1u);
        EXPECT_EQ(argIdx, 0u);
        EXPECT_EQ(dstType, nullptr);
        EXPECT_EQ(result.ErrorPos, static_cast<uint16_t>(pos + part.data() - str.data()));
        EXPECT_EQ(result.ErrorNum, static_cast<uint16_t>(err));
    };
    static constexpr auto SkipTester = [](ArgIdTesterArg)
    {
        EXPECT_EQ(retcode, 2u);
        EXPECT_EQ(argIdx, 0u);
        EXPECT_EQ(dstType, nullptr);
        EXPECT_EQ(result.ErrorPos, static_cast<uint16_t>(0));
        EXPECT_EQ(result.ErrorNum, static_cast<uint16_t>(0));
    };
#define CheckIdxArg(idx) IdxTester(idx, result, str, part, retcode, argIdx, dstType)
#define CheckNamedArg(idx, name, cnt) NamedTester(idx, name, cnt, result, str, part, retcode, argIdx, dstType)
#define CheckFailed(pos, err) FailedTester(pos, ParseResultBase::ErrorCode::err, result, str, part, retcode, argIdx, dstType)
#define CheckSkip() SkipTester(result, str, part, retcode, argIdx, dstType)
    constexpr auto str = "0,02,3,45,618,9x,_,x,x1,pt9x,x1,@8"sv;
    constexpr TestArgIdTester testers[] =
    {
        [](ArgIdTesterArg) { CheckIdxArg(0); },
        [](ArgIdTesterArg) { CheckFailed(0, InvalidArgIdx); },
        [](ArgIdTesterArg) { CheckIdxArg(3); },
        [](ArgIdTesterArg) { CheckIdxArg(45); },
        [](ArgIdTesterArg) { CheckFailed(1, ArgIdxTooLarge); },
        [](ArgIdTesterArg) { CheckFailed(1, InvalidArgIdx); },
        [](ArgIdTesterArg) { CheckNamedArg(0, "_", 1); },
        [](ArgIdTesterArg) { CheckNamedArg(1, "x", 2); },
        [](ArgIdTesterArg) { CheckNamedArg(2, "x1", 3); },
        [](ArgIdTesterArg) { CheckNamedArg(3, "pt9x", 4); },
        [](ArgIdTesterArg) { CheckNamedArg(2, "x1", 4); },
        [](ArgIdTesterArg) { CheckSkip(); },
    };
    TestArgId(str, testers);
#undef CheckIdxArg
#undef CheckNamedArg
#undef CheckFailed
#undef CheckSkip
}

#undef ArgIdTesterArg


TEST(Format, ReadUTF)
{
#define SingleCharTest(type, txt, ch, cnt) do                           \
{                                                                       \
    std::basic_string_view<type> src = txt;                             \
    uint32_t idx = 0;                                                   \
    const auto [ret, suc] = FormatterParserCh<type>::ReadWholeUtf32(    \
        src.data(), idx, static_cast<uint32_t>(src.size()));            \
    EXPECT_TRUE(suc);                                                   \
    EXPECT_EQ(ret, ch);                                                 \
    EXPECT_EQ(idx, cnt);                                                \
} while(false)

    SingleCharTest(char, "hi", U'h', 1u);
    SingleCharTest(char, "\xc2\xb1", U'\xb1', 2u);
    SingleCharTest(char, "\xcb\xa5", U'\x2e5', 2u);
    SingleCharTest(char, "\xe6\x88\x91", U'\x6211', 3u);
    SingleCharTest(char, "\xf0\xa4\xad\xa2", U'\x24b62', 4u);

    SingleCharTest(char16_t, u"hi", U'h', 1u);
    SingleCharTest(char16_t, u"±", U'±', 1u);
    SingleCharTest(char16_t, u"˥", U'˥', 1u);
    SingleCharTest(char16_t, u"我", U'我', 1u);
    SingleCharTest(char16_t, u"𤭢", U'𤭢', 2u);

    SingleCharTest(char32_t, U"hi", U'h', 1u);
    SingleCharTest(char32_t, U"±", U'±', 1u);
    SingleCharTest(char32_t, U"˥", U'˥', 1u);
    SingleCharTest(char32_t, U"我", U'我', 1u);
    SingleCharTest(char32_t, U"𤭢", U'𤭢', 1u);

    SingleCharTest(wchar_t, L"hi", U'h', 1u);
    SingleCharTest(wchar_t, L"±", U'±', 1u);
    SingleCharTest(wchar_t, L"˥", U'˥', 1u);
    SingleCharTest(wchar_t, L"我", U'我', 1u);
    SingleCharTest(wchar_t, L"𤭢", U'𤭢', sizeof(wchar_t) == sizeof(char16_t) ? 2u : 1u);

#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    SingleCharTest(char8_t, u8"hi", U'h', 1u);
    SingleCharTest(char8_t, u8"±", U'±', 2u);
    SingleCharTest(char8_t, u8"˥", U'˥', 2u);
    SingleCharTest(char8_t, u8"我", U'我', 3u);
    SingleCharTest(char8_t, u8"𤭢", U'𤭢', 4u);
#endif

#undef SingleCharTest


#define SingleCharFail(type, txt, pos) do                               \
{                                                                       \
    std::basic_string_view<type> src = txt;                             \
    uint32_t idx = 0;                                                   \
    const auto [ret, suc] = FormatterParserCh<type>::ReadWholeUtf32(    \
        src.data(), idx, static_cast<uint32_t>(src.size()));            \
    EXPECT_FALSE(suc);                                                  \
    EXPECT_EQ(ret, pos);                                                \
} while(false)

    SingleCharFail(char, "\x88\x91", 0u);           // 0b10xxxxxx
    SingleCharFail(char, "\xc2", 0u);               // 1B for 2B
    SingleCharFail(char, "\xe6\x88", 0u);           // 2B for 3B
    SingleCharFail(char, "\xf0\xa4\xad", 0u);       // 3B for 4B
    SingleCharFail(char, "\xc2\x03", 1u);           // 0b10xxxxxx
    SingleCharFail(char, "\xf0\xa4\x0d\xa2", 2u);   // 0b10xxxxxx
    SingleCharFail(char, "\xf0\xa4\xad\x72", 3u);   // 0b10xxxxxx

    SingleCharFail(char16_t, u"\xdf01", 0u);        // surrogate low
    SingleCharFail(char16_t, u"\xd800", 0u);        // surrogate 1B for 2B
    SingleCharFail(char16_t, u"\xd800\x7550", 1u);  // surrogate low

#undef SingleCharFail
}



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

static void CheckOpCode(const uint8_t* opcodes, const uint32_t idx, uint8_t ref)
{
    const auto opcode = opcodes[idx];
    EXPECT_EQ(opcode, ref) << "opcode is " << OpToString(opcode) << " at " << idx;
}

template<uint16_t Size, typename Char>
static void CheckFmtStrOp_(const ParseResult<Size>& ret, uint16_t& idx, std::basic_string_view<Char> fmtStr, uint16_t offset, uint16_t length, std::basic_string_view<Char> str)
{
    ASSERT_EQ(fmtStr.substr(offset, length), str);
    uint8_t op = FormatterParser::BuiltinOp::Op | FormatterParser::BuiltinOp::FieldFmtStr;
    if (offset > UINT8_MAX) op |= FormatterParser::BuiltinOp::DataOffset16;
    if (length > UINT8_MAX) op |= FormatterParser::BuiltinOp::DataLength16;
    CheckOpCode(ret.Opcodes, idx++, op);
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

template<uint16_t Size>
static void CheckBraceOp_(const ParseResult<Size>& ret, uint16_t& idx, const char ch)
{
    CheckOpCode(ret.Opcodes, idx++, FormatterParser::BuiltinOp::Op | FormatterParser::BuiltinOp::FieldBrace | (ch == '{' ? 0x0 : 0x1));
}

using ColorArgData = std::variant<std::monostate, common::CommonColor, uint8_t, std::array<uint8_t, 3>>;
template<uint16_t Size>
static void CheckColorArgOp_(const ParseResult<Size>& ret, uint16_t& idx, bool isFG, ColorArgData data)
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
    CheckOpCode(ret.Opcodes, idx++, op);
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

template<uint16_t Size>
static void CheckIdxArgOp_(const ParseResult<Size>& ret, uint16_t& idx, uint8_t argidx, const FormatterParser::FormatSpec* spec)
{
    const uint8_t field = FormatterParser::ArgOp::FieldIndexed | (spec ? FormatterParser::ArgOp::FieldHasSpec : 0);
    CheckOpCode(ret.Opcodes, idx++, FormatterParser::ArgOp::Op | field);
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

template<uint16_t Size>
static void CheckNamedArgOp_(const ParseResult<Size>& ret, uint16_t& idx, uint8_t argidx, const FormatterParser::FormatSpec* spec)
{
    const uint8_t field = FormatterParser::ArgOp::FieldNamed | (spec ? FormatterParser::ArgOp::FieldHasSpec : 0);
    CheckOpCode(ret.Opcodes, idx++, FormatterParser::ArgOp::Op | field);
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
        constexpr auto ret = FormatterParser::ParseString("{:~s}"sv);
        CheckFail(2, FillWithoutAlign);
        CheckIdxArgCount(ret, 1, 1);
    }
    {
        const auto ret = FormatterParser::ParseString("{:\x88\x91<s}"sv);
        CheckFail(2, InvalidCodepoint);
        CheckIdxArgCount(ret, 1, 1);
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{:\x88<s}"sv);
        CheckFail(2, InvalidCodepoint);
        CheckIdxArgCount(ret, 1, 1);
    }
    {
        constexpr auto ret = FormatterParser::ParseString("{:\xf0\xa4\xad\xa2<s}"sv);
        CheckSuc();
        CheckIdxArgType(ret, 1, String);
        uint16_t idx = 0;
        FormatterParser::FormatSpec spec
        {
            .Fill = U'𤭢',
            .Type = 's',
            .Alignment = FormatterParser::FormatSpec::Align::Left,
        };
        CheckOp(IdxArg, 0, &spec);
        CheckOpFinish();
    }
    {
        constexpr auto ret = FormatterParser::ParseString(u"{:\xd852\xdf62<s}"sv);
        CheckSuc();
        CheckIdxArgType(ret, 1, String);
        uint16_t idx = 0;
        FormatterParser::FormatSpec spec
        {
            .Fill = U'𤭢',
            .Type = 's',
            .Alignment = FormatterParser::FormatSpec::Align::Left,
        };
        CheckOp(IdxArg, 0, &spec);
        CheckOpFinish();
    }
    {
        constexpr auto ret = FormatterParser::ParseString(u"{:\xdf52<s}"sv);
        CheckFail(2, InvalidCodepoint);
        CheckIdxArgCount(ret, 1, 1);
    }
    {
        constexpr auto ret = FormatterParser::ParseString(u"{:\xdf52\xd862<s}"sv);
        CheckFail(2, InvalidCodepoint);
        CheckIdxArgCount(ret, 1, 1);
    }
    {
        constexpr auto ret = FormatterParser::ParseString(U"{:𤭢<s}"sv);
        CheckSuc();
        CheckIdxArgType(ret, 1, String);
        uint16_t idx = 0;
        FormatterParser::FormatSpec spec
        {
            .Fill = U'𤭢',
            .Type = 's',
            .Alignment = FormatterParser::FormatSpec::Align::Left,
        };
        CheckOp(IdxArg, 0, &spec);
        CheckOpFinish();
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
        constexpr auto fmtStr = "H{{"sv;
        constexpr auto ret = FormatterParser::ParseString(fmtStr);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(FmtStr, fmtStr, 0, 2, "H{"sv);
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
        const auto ret = FormatterParser::ParseString(fmtStr);
        CheckSuc();
        CheckIdxArgCount(ret, 0, 0);
        uint16_t idx = 0;
        CheckOp(FmtStr, fmtStr, 0, 5, "Hello"sv);
        CheckOpFinish();
    }
    {
        constexpr auto fmtStr = "Hello{{"sv;
        const auto ret = FormatterParser::ParseString(fmtStr);
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
        constexpr auto fmtStr = "H}}{x1}{{o"sv;
        constexpr auto ret = FormatterParser::ParseString(fmtStr);
        CheckSuc();
        CheckNamedArgs(ret, fmtStr, (Any, "x1"));
        uint16_t idx = 0;
        CheckOp(FmtStr, fmtStr, 0, 2, "H}"sv);
        CheckOp(NamedArg, 0, nullptr);
        CheckOp(Brace, '{');
        CheckOp(FmtStr, fmtStr, 9, 1, "o"sv);
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
        constexpr auto ret = FormatterParser::ParseString("{:0<d}{:<>d}"sv);
        CheckSuc();
        CheckIdxArgType(ret, 2, Integer, Integer);
        uint16_t idx = 0;
        FormatterParser::FormatSpec spec0
        {
            .Fill = '0',
            .Type = 'd',
            .Alignment = FormatterParser::FormatSpec::Align::Left,
        };
        CheckOp(IdxArg, 0, &spec0);
        FormatterParser::FormatSpec spec1
        {
            .Fill = '<',
            .Type = 'd',
            .Alignment = FormatterParser::FormatSpec::Align::Right,
        };
        CheckOp(IdxArg, 1, &spec1);
        CheckOpFinish();
    }
    { // utf parse fill check
        constexpr auto ret = FormatterParser::ParseString("{:\xe6\x88\x91^d}"sv);
        CheckSuc();
        CheckIdxArgType(ret, 1, Integer);
        uint16_t idx = 0;
        FormatterParser::FormatSpec spec0
        {
            .Fill = 0x6211u,
            .Type = 'd',
            .Alignment = FormatterParser::FormatSpec::Align::Middle,
        };
        CheckOp(IdxArg, 0, &spec0);
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
    //[[maybe_unused]] constexpr auto parse0 = FmtString("{}"sv);
    //[[maybe_unused]] constexpr auto parse1 = FmtString("here"sv);
    //[[maybe_unused]] constexpr auto parse2 = FmtString("{3:p}xyz{@<K}"sv);
    //constexpr auto klp = []()
    //{
    //    constexpr auto Result = ParseResult::ParseString("");
    //    constexpr auto OpCount = Result.OpCount;
    //    constexpr auto NamedArgCount = Result.NamedArgCount;
    //    constexpr auto IdxArgCount = Result.IdxArgCount;
    //    ParseResult::CheckErrorCompile<Result.ErrorPos, OpCount>();
    //    TrimedResult<OpCount, NamedArgCount, IdxArgCount> ret(Result);
    //    return ret;
    //}();

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
    void FormatWith(common::str::FormatterHost& host, common::str::FormatterContext& context, const common::str::FormatSpec*) const
    {
        host.PutString(context, "5"sv, nullptr);
    }
};
struct TypeD
{};
namespace common::str
{
template<> inline auto FormatAs<TypeB>(const TypeB&) { return 3.5f; }
template<> inline auto FormatWith<TypeD>(const TypeD&, FormatterHost& host, FormatterContext& context, const FormatSpec*)
{ 
    host.PutString(context, "7.5"sv, nullptr);
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
template<typename Char, typename... Args>
auto ToString2(std::basic_string_view<Char> res, Args&&... args)
{
    Formatter<Char> formatter;
    return formatter.FormatDynamic(res, std::forward<Args>(args)...);
}
TEST(Format, Formatting)
{
    {
        const auto ref = fmt::format("{},{:>6},{:_^7},{x}", "hello", "Hello", "World"sv, fmt::arg("x", "world"));
        const auto ret = ToString(FmtString("{},{:>6},{:_^7},{x}"sv), "hello", u"Hello", U"World"sv, NAMEARG("x")("world"sv));
        //const auto ret = ToString2("{},{:>6},{:_^7},{x}"sv, "hello", u"Hello", U"World"sv, NAMEARG("x")("world"sv));
        EXPECT_EQ(ref, "hello, Hello,_World_,world");
        EXPECT_EQ(ret, ref);
    }
    {
        const auto ref = fmt::format("{},{:b},{:#X},{:05o},{x}", 13, uint8_t(64), int64_t(65535), 042, fmt::arg("x", -99));
        const auto ret = ToString(FmtString("{},{:b},{:#X},{:05o},{x}"sv), 13, uint8_t(64), int64_t(65535), 042, NAMEARG("x")(-99));
        EXPECT_EQ(ref, "13,1000000,0XFFFF,00042,-99");
        EXPECT_EQ(ret, ref);
    }
    {
        const auto ref = fmt::format("{},{:g},{:f},{:+010.4g},{x}", 0.0, 4.9014e6, -392.5f, 392.65, fmt::arg("x", 392.65));
        const auto ret = ToString(FmtString("{},{:g},{:f},{:+010.4g},{x}"sv), 0.0, 4.9014e6, -392.5f, 392.65, NAMEARG("x")(392.65));
        //const auto ret = ToString2("{},{:g},{:f},{:+010.4g},{x}"sv, 0.0, 4.9014e6, -392.5f, 392.65, NAMEARG("x")(392.65));
        EXPECT_EQ(ref, "0,4.9014e+06,-392.500000,+0000392.6,392.65");
        EXPECT_EQ(ret, ref);
    }
    {
        const auto ptr = reinterpret_cast<const int*>(uintptr_t(1));
        const auto ref = fmt::format("{},{}", (void*)ptr, false);
        const auto ret = ToString(FmtString("{},{}"sv), ptr, false);
        EXPECT_EQ(ref, "0x1,false");
        EXPECT_EQ(ret, ref);
    }
    {
        const auto ref = fmt::format(u"{},{},{},{},{:g},{:f},{:+010.4g},{x}"sv,
            u"hello", u"Hello", u"World"sv, 0.0, 4.9014e6, -392.5f, 392.65, fmt::arg(u"x", 392.65));
        const auto ret = ToString(FmtString(u"{},{},{},{},{:g},{:f},{:+010.4g},{x}"sv),
            "hello", u"Hello", U"World"sv, 0.0, 4.9014e6, -392.5f, 392.65, NAMEARG("x")(392.65));
        EXPECT_EQ(ref, u"hello,Hello,World,0,4.9014e+06,-392.500000,+0000392.6,392.65");
        EXPECT_EQ(ret, ref);
    }
    {
        int32_t datI32[4] = { 1,-3,9,0 };
        uint64_t datU64[4] = { 996, 510, UINT32_MAX, 0 };
        float datF32[3] = { 1.0f, -3.0f, -0.0f };
        double datF64[3] = { 0.0, 10e50, -2e-50 };
        common::span<const int32_t> spanI32(datI32);
        common::span<const uint64_t> spanU64(datU64);
        common::span<const float> spanF32(datF32);
        common::span<const double> spanF64(datF64);
        const auto ref = fmt::format(u"{},{},{},{}"sv, spanI32, spanU64, spanF32, spanF64);
        const auto ret = ToString(FmtString(u"{},{},{},{}"sv), spanI32, spanU64, spanF32, spanF64);
        EXPECT_EQ(ref, u"[1, -3, 9, 0],[996, 510, 4294967295, 0],[1, -3, -0],[0, 1e+51, -2e-50]");
        EXPECT_EQ(ret, ref);
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
                return std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>{ ticks };
            };
        const auto t3 = common::date::zoned_time<std::chrono::microseconds>{ common::date::current_zone(), t2 }; 
        const auto ref = fmt::format(std::locale::classic(), "{0:%Y-%m-%dT%H:%M:%S}|{0:%Y-%m-%dT%H:%M:%S}|{1:%Y-%m-%d %H:%M:%S}|{2:%Y-%m-%d %j=%b %U-%w %a %H:%M:%S}"sv, t1, t2, GetFmtTime(t3));
        const auto ret = ToString(FmtString("{}|{0:T}|{1:T%Y-%m-%d %H:%M:%S}|{2:T%Y-%m-%d %j=%b %U-%w %a %H:%M:%S}"sv), t1, t2, t3);
        EXPECT_EQ(ret, ref);
    }
}

TEST(Format, FormatUTF)
{
    {
        const auto ref = fmt::format("{:<<3d},{:\xe6\x88\x91^4d},{:\xf0\xa4\xad\xa2<4d},{},{:#X}", true, (uint16_t)u'a', -1, false, 'c');
        const auto ret = ToString(FmtString("{:<<3d},{:\xe6\x88\x91^4d},{:\xf0\xa4\xad\xa2<4d},{},{:#X}"sv), true, (uint16_t)u'a', -1, false, 'c');
        auto cachedFmter = FormatSpecCacher::CreateFrom(FmtString("{:<<3d},{:\xe6\x88\x91^4d},{:\xf0\xa4\xad\xa2<4d},{},{:#X}"sv), true, (uint16_t)u'a', -1, false, 'c');
        std::string ret2;
        cachedFmter.Format(ret2, true, (uint16_t)u'a', -1, false, 'c');
        EXPECT_EQ(ref, "1<<,我97我,-1𤭢𤭢,false,0X63");
        EXPECT_EQ(ret, ref);
        EXPECT_EQ(ret2, ref);
    }
    {
        const auto ref = fmt::format(U"{:我^4d},{:𤭢<4d},{},{:#X}", (uint16_t)u'a', -1, false, (uint8_t)'c');
        const auto ret = ToString(FmtString(U"{:我^4d},{:𤭢<4d},{},{:#X}"sv), (uint16_t)u'a', -1, false, (uint8_t)'c');
        auto cachedFmter = FormatSpecCacher::CreateFrom(FmtString(U"{:我^4d},{:𤭢<4d},{},{:#X}"), (uint16_t)u'a', -1, false, (uint8_t)'c');
        std::u32string ret2;
        cachedFmter.Format(ret2, (uint16_t)u'a', -1, false, (uint8_t)'c');
        EXPECT_EQ(ref, U"我97我,-1𤭢𤭢,false,0X63");
        EXPECT_EQ(ret, ref);
        EXPECT_EQ(ret2, ref);
    }
    {
        const auto ref = fmt::format(u"{:我^4d},{},{:#X}", (uint16_t)u'a', false, (uint8_t)'c');
        const auto ret = ToString(FmtString(u"{:我^4d},{},{:#X}"sv), (uint16_t)u'a', false, (uint8_t)'c');
        EXPECT_EQ(ref, u"我97我,false,0X63");
        EXPECT_EQ(ret, ref);
    }
    {
        //const auto ref = fmt::format(u"{:𤭢^4d},{},{:#X}", (uint16_t)u'a', false, (uint8_t)'c');
        const auto ret = ToString(FmtString(u"{:𤭢^4d},{},{:#X}"sv), (uint16_t)u'a', false, (uint8_t)'c');
        EXPECT_EQ(ret, u"𤭢97𤭢,false,0X63");
        //EXPECT_EQ(ret, ref);
    }
    {
        auto cachedFmter = FormatSpecCacher::CreateFrom(FmtString(u"{:我^4d},{:𤭢<4d},{},{:#X}"), (uint16_t)u'a', -1, false, (uint8_t)'c');
        std::u16string ret;
        cachedFmter.Format(ret, (uint16_t)u'a', -1, false, (uint8_t)'c');
        EXPECT_EQ(ret, u"我97我,-1𤭢𤭢,false,0X63");
    }
}

TEST(Format, MultiFormat)
{
    Formatter<char> fmter{};
    {
        const auto& syntax = FormatterCombiner::Combine(FmtString("{0},{1}"sv), FmtString("{1},{0}"sv));
        const auto ret0 = syntax(0)(fmter, 1, 2);
        EXPECT_EQ(ret0, "1,2");
        const auto ret1 = syntax(1)(fmter, 1, 2);
        EXPECT_EQ(ret1, "2,1");
    }
    {
        const auto& syntax = FormatterCombiner::Combine(FmtString("{t1}"sv), FmtString("{t2}"sv));
        const auto ret0 = syntax(0)(fmter, NAMEARG("t1")("t1"), NAMEARG("t2")("t2"));
        EXPECT_EQ(ret0, "t1");
        const auto ret1 = syntax(1)(fmter, NAMEARG("t1")("t1"), NAMEARG("t2")("t2"));
        EXPECT_EQ(ret1, "t2");
    }
    {
        const auto& syntax = FormatterCombiner::Combine(FmtString("{0:d},{t1}"sv), FmtString("{1:s},{t2}"sv), FmtString("{},{},{t1:s},{t2}"sv));
        const auto ret0 = syntax(0)(fmter, 1, "2", NAMEARG("t1")("t1"), NAMEARG("t2")("t2"));
        EXPECT_EQ(ret0, "1,t1");
        const auto ret1 = syntax(1)(fmter, 1, "2", NAMEARG("t1")("t1"), NAMEARG("t2")("t2"));
        EXPECT_EQ(ret1, "2,t2");
        const auto ret2 = syntax(2)(fmter, 1, "2", NAMEARG("t1")("t1"), NAMEARG("t2")("t2"));
        EXPECT_EQ(ret2, "1,2,t1,t2");
    }
}



#if CM_DEBUG == 0
TEST(Format, Perf)
{
    [[maybe_unused]] PerfTester tester("FormatPerf");
    const uint64_t datU64[4] = { 996, 510, UINT32_MAX, 0 };
    common::span<const uint64_t> spanU64(datU64);

    std::string ref[5];
    const auto fmtdef = [&]()
    {
        ref[0].clear();
        fmt::format_to(std::back_inserter(ref[0]), FMT_STRING("123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}{}"),
            "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765), spanU64);
    };
    const auto fmtdir = [&]()
    {
        ref[1].clear();
        fmt::format_to(std::back_inserter(ref[1]), "123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}{}",
            "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765), spanU64);
    };
    const auto fmtcpl = [&]()
    {
        ref[2].clear();
        fmt::format_to(std::back_inserter(ref[2]), FMT_COMPILE("123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}{}"),
            "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765), spanU64);
    };
    const auto fmtrt = [&]()
    {
        ref[3].clear();
        fmt::format_to(std::back_inserter(ref[3]), fmt::runtime("123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}{}"),
            "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765), spanU64);
    };
    const auto fmtdyn = [&]()
    {
        ref[4].clear();
        fmt::dynamic_format_arg_store<fmt::buffered_context<char>> store;
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
        store.push_back(spanU64);
        fmt::vformat_to(std::back_inserter(ref[4]), "123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}{}", store);
    };

    std::string csf[5];
    const auto csfdyn = [&]()
    {
        csf[0].clear();
        Formatter<char> fmter;
        fmter.FormatToDynamic(csf[0], "123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}{}",
            "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765), spanU64);
    };
    const auto csfsta = [&]()
    {
        csf[1].clear();
        Formatter<char> fmter;
        fmter.FormatToStatic(csf[1], FmtString("123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}{}"),
            "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765), spanU64);
    };
    auto cachedFmter = FormatSpecCacher::CreateFrom(FmtString("123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}{}"),
        "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765), spanU64);
    const auto csfcah = [&]()
    {
        csf[2].clear();
        cachedFmter.Format(csf[2], "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765), spanU64);
    };
    const auto csfexe = [&]()
    {
        csf[3].clear();
        BasicExecutor<char> executor;
        executor.FormatToStatic(csf[3], FmtString("123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}{}"),
            "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765), spanU64);
    };
    struct TestHolder
    {
        common::span<const uint64_t> SpanU64;
        inline void FormatWith(FormatterHost& host, FormatterContext& context, const FormatSpec*) const
        {
            common::str::FormatterBase::NestedFormat(host, context, FmtString("123{},456{},{},{:b},{:#X},{:09o},12345678901234567890{:g},{:f},{:+010.4g}abcdefg{:_^9}{}"),
                "hello", 0.0, 42, uint8_t(64), int64_t(65535), -70000, 4.9014e6, -392.5f, 392.65, uint64_t(765), SpanU64);
        }
    };
    const auto csfnest = [&]()
    {
        csf[4].clear();
        common::str::Formatter<char>{}.DirectFormatTo(csf[4], TestHolder{ datU64 }, nullptr);
    };
    
    tester.ManaulTest(
        "fmt-def", fmtdef,
        "fmt-dir", fmtdir,
        "fmt-cpl", fmtcpl,
        "fmt-rt ", fmtrt,
        "fmt-dyn", fmtdyn,
        "csf-dyn", csfdyn,
        "csf-sta", csfsta,
        "csf-cah", csfcah,
        "csf-exe", csfexe,
        "csf-nst", csfnest
    );
    
    EXPECT_EQ(ref[1], ref[0]);
    EXPECT_EQ(ref[2], ref[0]);
    EXPECT_EQ(ref[3], ref[0]);
    EXPECT_EQ(ref[4], ref[0]);
    EXPECT_EQ(csf[0], ref[0]);
    EXPECT_EQ(csf[1], csf[0]);
    EXPECT_EQ(csf[2], csf[0]);
    EXPECT_EQ(csf[3], csf[0]);
    EXPECT_EQ(csf[4], csf[0]);
    
    //PerfTester("FormatPerf", 1, 3000).ManaulTest("csf-exe", csfexe);
}
#endif
