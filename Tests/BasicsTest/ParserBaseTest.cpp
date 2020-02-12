#include "pch.h"
#include "common/parser/ParserBase.hpp"
#include "common/Linq2.hpp"


using namespace common::parser;
using namespace common::parser::tokenizer;
using namespace std::string_view_literals;
using CharType = common::parser::ParserContext::CharType;


#define CHECK_POS(ctx, row, col) EXPECT_EQ(ctx.Row, row); EXPECT_EQ(ctx.Col, col)
#define CHECK_CHTYPE(ch, type) EXPECT_EQ(ParserContext::ParseType(ch), CharType::type)
#define CHECK_TK_TYPE(token, type) EXPECT_EQ(token.GetIDEnum(), BaseToken::type)

#define CHECK_TK(token, type, action, val) do { CHECK_TK_TYPE(token, type); EXPECT_EQ(token.action(), val); } while(0)

#define TO_BASETOKN_TYPE(r, dummy, i, type) BOOST_PP_COMMA_IF(i) BaseToken::type
#define EXPEND_TKTYPES(...) BOOST_PP_SEQ_FOR_EACH_I(TO_BASETOKN_TYPE, "", BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define CHECK_TKS_TYPE(types, ...) EXPECT_THAT(types, testing::ElementsAre(EXPEND_TKTYPES(__VA_ARGS__)))


#define MAP_TOKENS(tokens, action) common::linq::FromIterable(tokens).Select([](const auto& token) { return token.action; }).ToVector()
#define MAP_TOKEN_TYPES(tokens) common::linq::FromIterable(tokens).Select([](const auto& token) { return token.GetIDEnum(); }).ToVector()


template<typename... TKs>
auto TKParse(const std::u32string_view src, ASCIIChecker<true> delim = " \t\r\n\v"sv)
{
    constexpr ASCIIChecker ignore = " \t\r\n\v"sv;
    constexpr ParserBase<TKs...> parser_;
    auto parser = parser_;
    ParserContext context(src);
    std::vector<ParserToken> tokens;
    while (true)
    {
        const auto token = parser.GetDelimTokenBy(context, delim, ignore);
        const auto type = token.GetIDEnum();
        if (type != BaseToken::End)
            tokens.emplace_back(token);
        if (type == BaseToken::Error || type == BaseToken::Unknown || type == BaseToken::End)
            break;
    }
    return tokens;
}



TEST(ParserBase, ASCIIChecker)
{
    std::string str;
    for (uint32_t i = 0; i < 128; i += 2)
        str.push_back(static_cast<char>(i));
    ASCIIChecker checker(str);

    for (uint16_t i = 0; i < 128; i++)
        EXPECT_EQ(checker(i), (i & 1) == 0 ? true : false);

    for (uint16_t i = 128; i < 512; i++)
        EXPECT_EQ(checker(i), false);
}


TEST(ParserBase, ParserComment)
{
    {
        const auto tokens = TKParse<CommentTokenizer>(U"//hello"sv);
        CHECK_TK(tokens[0], Comment, GetString, U"hello"sv);
    }
    {
        const auto tokens = TKParse<CommentTokenizer>(U"/*hello*/"sv);
        CHECK_TK(tokens[0], Comment, GetString, U"hello"sv);
    }
    {
        const auto tokens = TKParse<CommentTokenizer>(U"//hello there"sv);
        CHECK_TK(tokens[0], Comment, GetString, U"hello there"sv);
    }
    {
        const auto tokens = TKParse<CommentTokenizer>(U"/*hello\nthere*/"sv);
        CHECK_TK(tokens[0], Comment, GetString, U"hello\nthere"sv);
    }
    {
        const auto tokens = TKParse<CommentTokenizer>(U"/* hello\nthere */\n// hello"sv);
        CHECK_TK(tokens[0], Comment, GetString, U" hello\nthere "sv);
        CHECK_TK(tokens[1], Comment, GetString, U" hello"sv);
    }
}


TEST(ParserBase, ParserString)
{

    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello")"sv);
        CHECK_TK(tokens[0], String, GetString, U"hello"sv);
    }
    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello there")"sv);
        CHECK_TK(tokens[0], String, GetString, U"hello there"sv);
    }
    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello\"there")"sv);
        CHECK_TK(tokens[0], String, GetString, UR"(hello\"there)"sv);
    }
    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello)"sv);
        CHECK_TK(tokens[0], Error,  GetString, U"hello"sv);
    }
    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello"," ","there")"sv, ","sv);
        CHECK_TK(tokens[0], String, GetString, U"hello"sv);
        CHECK_TK(tokens[1], Delim,  GetChar,   U',');
        CHECK_TK(tokens[2], String, GetString, U" "sv);
        CHECK_TK(tokens[3], Delim,  GetChar,   U',');
        CHECK_TK(tokens[4], String, GetString, U"there"sv);
    }
}


TEST(ParserBase, ParserInt)
{

    {
        const auto tokens = TKParse<IntTokenizer>(U"123"sv);
        CHECK_TK(tokens[0], Uint, GetUInt, 123);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"-123"sv);
        CHECK_TK(tokens[0], Int, GetInt, -123);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0x13579bdf"sv);
        CHECK_TK(tokens[0], Uint, GetUInt, 0x13579bdf);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0b1010010100111110"sv);
        CHECK_TK(tokens[0], Uint, GetUInt, 0b1010010100111110);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"-0"sv);
        CHECK_TK(tokens[0], Int, GetInt, 0);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"-0X12"sv);
        CHECK_TK(tokens[0], Unknown, GetString, U"-0X12"sv);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0b123"sv);
        CHECK_TK(tokens[0], Unknown, GetString, U"0b123"sv);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0x1ax3"sv);
        CHECK_TK(tokens[0], Unknown, GetString, U"0x1ax3"sv);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0x12345678901234567890"sv);
        CHECK_TK(tokens[0], Error, GetString, U"0x12345678901234567890"sv);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0b10101010101010101010101010101010101010101010101010101010101010101"sv);
        CHECK_TK(tokens[0], Error, GetString, U"0b10101010101010101010101010101010101010101010101010101010101010101"sv);
    }
}


TEST(ParserBase, ParserFP)
{
    {
        const auto tokens = TKParse<FPTokenizer>(U"123"sv);
        CHECK_TK(tokens[0], FP, GetDouble, 123.0);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"123.0"sv);
        CHECK_TK(tokens[0], FP, GetDouble, 123.0);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"-123.0"sv);
        CHECK_TK(tokens[0], FP, GetDouble, -123.0);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U".5"sv);
        CHECK_TK(tokens[0], FP, GetDouble, .5);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"-.5"sv);
        CHECK_TK(tokens[0], FP, GetDouble, -.5);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"1e5"sv);
        CHECK_TK(tokens[0], FP, GetDouble, 1e5);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"1.5E-2"sv);
        CHECK_TK(tokens[0], FP, GetDouble, 1.5e-2);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"."sv);
        CHECK_TK(tokens[0], Unknown, GetString, U"."sv);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"--123"sv);
        CHECK_TK(tokens[0], Unknown, GetString, U"--123"sv);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"1e.5"sv);
        CHECK_TK(tokens[0], Unknown, GetString, U"1e.5"sv);
    }
}


TEST(ParserBase, ParserBool)
{
    {
        const auto tokens = TKParse<BoolTokenizer>(U"TRUE"sv);
        CHECK_TK(tokens[0], Bool, GetBool, true);
    }
    {
        const auto tokens = TKParse<BoolTokenizer>(U"false"sv);
        CHECK_TK(tokens[0], Bool, GetBool, false);
    }
    {
        const auto tokens = TKParse<BoolTokenizer>(U"TrUe"sv);
        CHECK_TK(tokens[0], Bool, GetBool, true);
    }
    {
        const auto tokens = TKParse<BoolTokenizer>(U"Fals3"sv);
        CHECK_TK(tokens[0], Unknown, GetString, U"Fals3"sv);
    }
}


TEST(ParserBase, ParserASCIIRaw)
{
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"func"sv);
        CHECK_TK(tokens[0], Raw, GetString, U"func"sv);
    }
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"fu1nc"sv);
        CHECK_TK(tokens[0], Raw, GetString, U"fu1nc"sv);
    }
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"1func"sv);
        CHECK_TK(tokens[0], Raw, GetString, U"1func"sv);
    }
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"fu_nc"sv);
        CHECK_TK(tokens[0], Raw, GetString, U"fu_nc"sv);
    }
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"fu_nc("sv);
        CHECK_TK(tokens[0], Unknown, GetString, U"fu_nc("sv);
    }
}


TEST(ParserBase, ParserASCII2Part)
{
    {
        const auto tokens = TKParse<ASCII2PartTokenizer>(U"func"sv);
        CHECK_TK(tokens[0], Raw, GetString, U"func"sv);
    }
    {
        const auto tokens = TKParse<ASCII2PartTokenizer>(U"fu1nc"sv);
        CHECK_TK(tokens[0], Raw, GetString, U"fu1nc"sv);
    }
    {
        const auto tokens = TKParse<ASCII2PartTokenizer>(U"1func"sv);
        CHECK_TK(tokens[0], Unknown, GetString, U"1func"sv);
    }
}


TEST(ParserBase, ParserArgs)
{
    constexpr auto ParseAll = [](const std::u32string_view src)
    {
        constexpr ASCIIChecker<true> DelimChecker(","sv);
        return TKParse<CommentTokenizer, StringTokenizer, IntTokenizer, FPTokenizer, BoolTokenizer>(src, DelimChecker);
    };
    {
        const auto tokens = ParseAll(UR"(123,456,789)"sv);
        CHECK_TK(tokens[0], Uint,  GetUInt, 123);
        CHECK_TK(tokens[1], Delim, GetChar, U',');
        CHECK_TK(tokens[2], Uint,  GetUInt, 456);
        CHECK_TK(tokens[3], Delim, GetChar, U',');
        CHECK_TK(tokens[4], Uint,  GetUInt, 789);
    }
    {
        const auto tokens = ParseAll(UR"(-123,"hello",3.5,0xff)"sv);
        CHECK_TK(tokens[0], Int,    GetInt,     -123);
        CHECK_TK(tokens[1], Delim,  GetChar,    U',');
        CHECK_TK(tokens[2], String, GetString,  U"hello"sv);
        CHECK_TK(tokens[3], Delim,  GetChar,    U',');
        CHECK_TK(tokens[4], FP,     GetDouble,  3.5);
        CHECK_TK(tokens[5], Delim,  GetChar,    U',');
        CHECK_TK(tokens[6], Uint,   GetUInt,    0xff);
    }
    {
        const auto tokens = ParseAll(UR"(-123, "hello",3.5 ,0xff)"sv);
        CHECK_TK(tokens[0], Int,    GetInt,     -123);
        CHECK_TK(tokens[1], Delim,  GetChar,    U',');
        CHECK_TK(tokens[2], String, GetString,  U"hello"sv);
        CHECK_TK(tokens[3], Delim,  GetChar,    U',');
        CHECK_TK(tokens[4], FP,     GetDouble,  3.5);
        CHECK_TK(tokens[5], Delim,  GetChar,    U',');
        CHECK_TK(tokens[6], Uint,   GetUInt,    0xff);
    }
}


//TEST(ParserBase, ParserFunc)
//{
//    constexpr ASCIIChecker ignore = " \t\r\n\v"sv;
//    constexpr ASCIIChecker<true> delim("(,)"sv);
//    constexpr ASCII2PartTokenizer MetaFuncTK(BaseToken::Raw, 1, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_@"sv);
//    constexpr ParserBase<ASCII2PartTokenizer> parser_(MetaFuncTK);
//    {
//        ParserContext context(UR"(func())"sv);
//        auto parser = parser_;
//        const auto tokens = ParseAll(UR"(123,456,789)"sv);
//        const auto types = MAP_TOKEN_TYPES(tokens);
//        const auto vals = MAP_TOKENS(tokens, GetUInt());
//        CHECK_TKS_TYPE(types, Uint, Uint, Uint);
//        EXPECT_THAT(vals, testing::ElementsAre(123, 456, 789));
//    }
//    {
//        const auto tokens = ParseAll(UR"(-123,"hello",3.5,0xff)"sv);
//        CHECK_TK(tokens[0], Int, GetInt, -123);
//        CHECK_TK(tokens[1], String, GetString, U"hello"sv);
//        CHECK_TK(tokens[2], FP, GetDouble, 3.5);
//        CHECK_TK(tokens[3], Uint, GetUInt, 0xff);
//    }
//    {
//        const auto tokens = ParseAll(UR"(-123, "hello",3.5 ,0xff)"sv);
//        CHECK_TK(tokens[0], Int, GetInt, -123);
//        CHECK_TK(tokens[1], String, GetString, U"hello"sv);
//        CHECK_TK(tokens[2], FP, GetDouble, 3.5);
//        CHECK_TK(tokens[3], Uint, GetUInt, 0xff);
//    }
//}

