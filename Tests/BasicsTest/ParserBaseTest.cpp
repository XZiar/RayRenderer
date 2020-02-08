#include "pch.h"
#include "common/parser/ParserBase.hpp"
#include "common/Linq2.hpp"


using namespace common::parser;
using namespace std::string_view_literals;
using CharType = common::parser::ParserContext::CharType;


#define CHECK_POS(ctx, row, col) EXPECT_EQ(ctx.Row, row); EXPECT_EQ(ctx.Col, col)
#define CHECK_CHTYPE(ch, type) EXPECT_EQ(ParserContext::ParseType(ch), CharType::type)
#define CHECK_TK_TYPE(token, type) EXPECT_EQ(token.GetIDEnum<BaseToken>(), BaseToken::type)

#define CHECK_TK(token, type, action, val) do { CHECK_TK_TYPE(token, type); EXPECT_EQ(token.action(), val); } while(0)

#define TO_BASETOKN_TYPE(r, dummy, i, type) BOOST_PP_COMMA_IF(i) BaseToken::type
#define EXPEND_TKTYPES(...) BOOST_PP_SEQ_FOR_EACH_I(TO_BASETOKN_TYPE, "", BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define CHECK_TKS_TYPE(types, ...) EXPECT_THAT(types, testing::ElementsAre(EXPEND_TKTYPES(__VA_ARGS__)))


#define MAP_TOKENS(tokens, action) common::linq::FromIterable(tokens).Select([](const auto& token) { return token.action; }).ToVector()
#define MAP_TOKEN_TYPES(tokens) common::linq::FromIterable(tokens).Select([](const auto& token) { return token.template GetIDEnum<BaseToken>(); }).ToVector()


template<typename... TKs>
auto TKParse(const std::u32string_view src, const std::u32string_view delim = U" \t\r\n\v"sv)
{
    ParserContext context(src);
    ParserBase<TKs...> parser(context);
    std::vector<ParserToken> tokens;
    while (true)
    {
        const auto [token, dChar] = parser.GetToken(delim, U" \t\r\n\v"sv);
        const auto type = token.GetIDEnum<BaseToken>();
        if (type != BaseToken::End)
            tokens.emplace_back(token);
        if (type == BaseToken::Error || type == BaseToken::Unknown || type == BaseToken::End)
            break;
    }
    return tokens;
}
auto ParseAll(const std::u32string_view src, const std::u32string_view delim = U" \t\r\n\v"sv)
{
    using namespace common::parser::tokenizer;
    return TKParse<CommentTokenizer, StringTokenizer, IntTokenizer, FPTokenizer, BoolTokenizer>(src, delim);
}


TEST(ParserBase, ParserComment)
{
    {
        const auto tokens = ParseAll(U"//hello"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Comment);
        EXPECT_EQ(vals[0], U"hello"sv);
    }
    {
        const auto tokens = ParseAll(U"/*hello*/"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Comment);
        EXPECT_EQ(vals[0], U"hello"sv);
    }
    {
        const auto tokens = ParseAll(U"//hello there"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Comment);
        EXPECT_EQ(vals[0], U"hello there"sv);
    }
    {
        const auto tokens = ParseAll(U"/*hello\nthere*/"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Comment);
        EXPECT_EQ(vals[0], U"hello\nthere"sv);
    }
    {
        const auto tokens = ParseAll(U"/* hello\nthere */\n// hello"sv);
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
        const auto tokens = ParseAll(UR"("hello")"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, String);
        EXPECT_EQ(vals[0], U"hello"sv);
    }
    {
        const auto tokens = ParseAll(UR"("hello there")"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, String);
        EXPECT_EQ(vals[0], U"hello there"sv);
    }
    {
        const auto tokens = ParseAll(UR"("hello\"there")"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, String);
        EXPECT_EQ(vals[0], UR"(hello\"there)"sv);
    }
    {
        const auto tokens = ParseAll(UR"("hello)"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Error);
        EXPECT_EQ(vals[0], U"hello"sv);
    }
    {
        const auto tokens = ParseAll(UR"("hello"," ","there")"sv, U","sv);
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
        const auto tokens = ParseAll(U"123"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetUInt());
        CHECK_TKS_TYPE(types, Uint);
        EXPECT_EQ(vals[0], 123);
    }
    {
        const auto tokens = ParseAll(U"-123"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetInt());
        CHECK_TKS_TYPE(types, Int);
        EXPECT_EQ(vals[0], -123);
    }
    {
        const auto tokens = ParseAll(U"0x13579bdf"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetUInt());
        CHECK_TKS_TYPE(types, Uint);
        EXPECT_EQ(vals[0], 0x13579bdf);
    }
    {
        const auto tokens = ParseAll(U"0b1010010100111110"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetUInt());
        CHECK_TKS_TYPE(types, Uint);
        EXPECT_EQ(vals[0], 0b1010010100111110);
    }
    {
        const auto tokens = ParseAll(U"-0"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetInt());
        CHECK_TKS_TYPE(types, Int);
        EXPECT_EQ(vals[0], 0);
    }
    {
        const auto tokens = ParseAll(U"-0X12"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Unknown);
        EXPECT_EQ(vals[0], U"-0X12"sv);
    }
    {
        const auto tokens = ParseAll(U"0b123"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Unknown);
        EXPECT_EQ(vals[0], U"0b123"sv);
    }
    {
        const auto tokens = ParseAll(U"0x1ax3"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Unknown);
        EXPECT_EQ(vals[0], U"0x1ax3"sv);
    }
    {
        const auto tokens = ParseAll(U"0x12345678901234567890"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Error);
        EXPECT_EQ(vals[0], U"0x12345678901234567890"sv);
    }
    {
        const auto tokens = ParseAll(U"0b10101010101010101010101010101010101010101010101010101010101010101"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetString());
        CHECK_TKS_TYPE(types, Error);
        EXPECT_EQ(vals[0], U"0b10101010101010101010101010101010101010101010101010101010101010101"sv);
    }
}


TEST(ParserBase, ParserFP)
{
    using namespace common::parser::tokenizer;
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
    using namespace common::parser::tokenizer;
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


TEST(ParserBase, ParserArgs)
{
    using namespace common::parser::tokenizer;
    {
        const auto tokens = ParseAll(UR"(123,456,789)"sv, U" ,\t\r\n\v"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetUInt());
        CHECK_TKS_TYPE(types, Uint, Uint, Uint);
        EXPECT_THAT(vals, testing::ElementsAre(123,456,789));
    }
    {
        const auto tokens = ParseAll(UR"(-123,"hello",3.5,0xff)"sv, U" ,\t\r\n\v"sv);
        CHECK_TK(tokens[0], Int, GetInt, -123);
        CHECK_TK(tokens[1], String, GetString, U"hello"sv);
        CHECK_TK(tokens[2], FP, GetDouble, 3.5);
        CHECK_TK(tokens[3], Uint, GetUInt, 0xff);
    }


}

