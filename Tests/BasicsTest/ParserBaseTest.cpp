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
        const auto [token, dChar] = parser.GetDelimTokenBy(context, delim, ignore);
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
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Comment);
        EXPECT_EQ(vals[0], U"hello"sv);
    }
    {
        const auto tokens = TKParse<CommentTokenizer>(U"/*hello*/"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Comment);
        EXPECT_EQ(vals[0], U"hello"sv);
    }
    {
        const auto tokens = TKParse<CommentTokenizer>(U"//hello there"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Comment);
        EXPECT_EQ(vals[0], U"hello there"sv);
    }
    {
        const auto tokens = TKParse<CommentTokenizer>(U"/*hello\nthere*/"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Comment);
        EXPECT_EQ(vals[0], U"hello\nthere"sv);
    }
    {
        const auto tokens = TKParse<CommentTokenizer>(U"/* hello\nthere */\n// hello"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Comment, Comment);
        EXPECT_EQ(vals[0], U" hello\nthere "sv);
        EXPECT_EQ(vals[1], U" hello"sv);
    }
}


TEST(ParserBase, ParserString)
{

    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello")"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, String);
        EXPECT_EQ(vals[0], U"hello"sv);
    }
    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello there")"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, String);
        EXPECT_EQ(vals[0], U"hello there"sv);
    }
    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello\"there")"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, String);
        EXPECT_EQ(vals[0], UR"(hello\"there)"sv);
    }
    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello)"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Error);
        EXPECT_EQ(vals[0], U"hello"sv);
    }
    {
        const auto tokens = TKParse<StringTokenizer>(UR"("hello"," ","there")"sv, ","sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, String, String, String);
        EXPECT_EQ(vals[0], U"hello"sv);
        EXPECT_EQ(vals[1], U" "sv);
        EXPECT_EQ(vals[2], U"there"sv);
    }
}


TEST(ParserBase, ParserInt)
{

    {
        const auto tokens = TKParse<IntTokenizer>(U"123"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetUInt());
        CHECK_TKS_TYPE(types, Uint);
        EXPECT_EQ(vals[0], 123);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"-123"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetInt());
        CHECK_TKS_TYPE(types, Int);
        EXPECT_EQ(vals[0], -123);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0x13579bdf"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetUInt());
        CHECK_TKS_TYPE(types, Uint);
        EXPECT_EQ(vals[0], 0x13579bdf);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0b1010010100111110"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetUInt());
        CHECK_TKS_TYPE(types, Uint);
        EXPECT_EQ(vals[0], 0b1010010100111110);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"-0"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetInt());
        CHECK_TKS_TYPE(types, Int);
        EXPECT_EQ(vals[0], 0);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"-0X12"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Unknown);
        EXPECT_EQ(vals[0], U"-0X12"sv);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0b123"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Unknown);
        EXPECT_EQ(vals[0], U"0b123"sv);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0x1ax3"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Unknown);
        EXPECT_EQ(vals[0], U"0x1ax3"sv);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0x12345678901234567890"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Error);
        EXPECT_EQ(vals[0], U"0x12345678901234567890"sv);
    }
    {
        const auto tokens = TKParse<IntTokenizer>(U"0b10101010101010101010101010101010101010101010101010101010101010101"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Error);
        EXPECT_EQ(vals[0], U"0b10101010101010101010101010101010101010101010101010101010101010101"sv);
    }
}


TEST(ParserBase, ParserFP)
{
    {
        const auto tokens = TKParse<FPTokenizer>(U"123"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetDouble());
        CHECK_TKS_TYPE(types, FP);
        EXPECT_EQ(vals[0], 123.0);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"123.0"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetDouble());
        CHECK_TKS_TYPE(types, FP);
        EXPECT_EQ(vals[0], 123.0);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"-123.0"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetDouble());
        CHECK_TKS_TYPE(types, FP);
        EXPECT_EQ(vals[0], -123.0);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U".5"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetDouble());
        CHECK_TKS_TYPE(types, FP);
        EXPECT_EQ(vals[0], .5);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"-.5"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetDouble());
        CHECK_TKS_TYPE(types, FP);
        EXPECT_EQ(vals[0], -.5);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"1e5"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetDouble());
        CHECK_TKS_TYPE(types, FP);
        EXPECT_EQ(vals[0], 1e5);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"1.5E-2"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetDouble());
        CHECK_TKS_TYPE(types, FP);
        EXPECT_EQ(vals[0], 1.5e-2);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"."sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Unknown);
        EXPECT_EQ(vals[0], U"."sv);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"--123"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Unknown);
        EXPECT_EQ(vals[0], U"--123"sv);
    }
    {
        const auto tokens = TKParse<FPTokenizer>(U"1e.5"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Unknown);
        EXPECT_EQ(vals[0], U"1e.5"sv);
    }
}


TEST(ParserBase, ParserBool)
{
    {
        const auto tokens = TKParse<BoolTokenizer>(U"TRUE"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetBool());
        CHECK_TKS_TYPE(types, Bool);
        EXPECT_EQ(vals[0], true);
    }
    {
        const auto tokens = TKParse<BoolTokenizer>(U"false"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetBool());
        CHECK_TKS_TYPE(types, Bool);
        EXPECT_EQ(vals[0], false);
    }
    {
        const auto tokens = TKParse<BoolTokenizer>(U"TrUe"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetBool());
        CHECK_TKS_TYPE(types, Bool);
        EXPECT_EQ(vals[0], true);
    }
    {
        const auto tokens = TKParse<BoolTokenizer>(U"Fals3"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Unknown);
        EXPECT_EQ(vals[0], U"Fals3"sv);
    }
}


TEST(ParserBase, ParserASCIIRaw)
{
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"func"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Raw);
        EXPECT_EQ(vals[0], U"func"sv);
    }
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"fu1nc"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Raw);
        EXPECT_EQ(vals[0], U"fu1nc"sv);
    }
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"1func"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Raw);
        EXPECT_EQ(vals[0], U"1func"sv);
    }
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"fu_nc"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Raw);
        EXPECT_EQ(vals[0], U"fu_nc"sv);
    }
    {
        const auto tokens = TKParse<ASCIIRawTokenizer>(U"fu_nc("sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Unknown);
        EXPECT_EQ(vals[0], U"fu_nc("sv);
    }
}


TEST(ParserBase, ParserASCII2Part)
{
    {
        const auto tokens = TKParse<ASCII2PartTokenizer>(U"func"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Raw);
        EXPECT_EQ(vals[0], U"func"sv);
    }
    {
        const auto tokens = TKParse<ASCII2PartTokenizer>(U"fu1nc"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Raw);
        EXPECT_EQ(vals[0], U"fu1nc"sv);
    }
    {
        const auto tokens = TKParse<ASCII2PartTokenizer>(U"1func"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Unknown);
        EXPECT_EQ(vals[0], U"1func"sv);
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
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetUInt());
        CHECK_TKS_TYPE(types, Uint, Uint, Uint);
        EXPECT_THAT(vals, testing::ElementsAre(123,456,789));
    }
    {
        const auto tokens = ParseAll(UR"(-123,"hello",3.5,0xff)"sv);
        CHECK_TK(tokens[0], Int, GetInt, -123);
        CHECK_TK(tokens[1], String, GetString, U"hello"sv);
        CHECK_TK(tokens[2], FP, GetDouble, 3.5);
        CHECK_TK(tokens[3], Uint, GetUInt, 0xff);
    }
    {
        const auto tokens = ParseAll(UR"(-123, "hello",3.5 ,0xff)"sv);
        CHECK_TK(tokens[0], Int, GetInt, -123);
        CHECK_TK(tokens[1], String, GetString, U"hello"sv);
        CHECK_TK(tokens[2], FP, GetDouble, 3.5);
        CHECK_TK(tokens[3], Uint, GetUInt, 0xff);
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

