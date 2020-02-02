#include "pch.h"
#include "common/parser/ParserBase.hpp"
#include "common/Linq2.hpp"


using namespace common::parser;
using namespace std::string_view_literals;
using CharType = common::parser::ParserContext::CharType;


#define CHECK_POS(ctx, row, col) EXPECT_EQ(ctx.Row, row); EXPECT_EQ(ctx.Col, col)
#define CHECK_CHTYPE(ch, type) EXPECT_EQ(ParserContext::ParseType(ch), CharType::type)
#define CHECK_TK_TYPE(token, type) EXPECT_EQ(token.GetIDEnum<BaseToken>(), BaseToken::type)

#define TO_BASETOKN_TYPE(r, dummy, i, type) BOOST_PP_COMMA_IF(i) BaseToken::type
#define EXPEND_TKTYPES(...) BOOST_PP_SEQ_FOR_EACH_I(TO_BASETOKN_TYPE, "", BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define CHECK_TKS_TYPE(types, ...) EXPECT_THAT(types, testing::ElementsAre(EXPEND_TKTYPES(__VA_ARGS__)))


#define MAP_TOKENS(tokens, action) common::linq::FromIterable(tokens).Select([](const auto& token) { return token.action; }).ToVector()
#define MAP_TOKEN_TYPES(tokens) common::linq::FromIterable(tokens).Select([](const auto& token) { return token.template GetIDEnum<BaseToken>(); }).ToVector()

static constexpr auto ParseTokenOne = [](const std::u32string_view src)
{
    ParserContext context(src);
    ParserBase parser(context);
    return parser.GetToken(U""sv);
};
TEST(ParserBase, Number)
{
    {
        const auto token = ParseTokenOne(U"0"sv);
        CHECK_TK_TYPE(token, Uint);
        EXPECT_EQ(token.GetUInt(), 0);
        EXPECT_EQ(token.GetInt(), 0);
    }
    {
        const auto token = ParseTokenOne(U"123"sv);
        CHECK_TK_TYPE(token, Uint);
        EXPECT_EQ(token.GetUInt(), 123);
    }
    {
        const auto token = ParseTokenOne(U"-3"sv);
        CHECK_TK_TYPE(token, Int);
        EXPECT_EQ(token.GetInt(), -3);
    }
    {
        const auto token = ParseTokenOne(U"0.5"sv);
        CHECK_TK_TYPE(token, FP);
        EXPECT_EQ(token.GetDouble(), 0.5);
    }
    {
        const auto token = ParseTokenOne(U".5"sv);
        CHECK_TK_TYPE(token, FP);
        EXPECT_EQ(token.GetDouble(), 0.5);
    }
    {
        const auto token = ParseTokenOne(U"-0.5"sv);
        CHECK_TK_TYPE(token, FP);
        EXPECT_EQ(token.GetDouble(), -0.5);
    }
    {
        const auto token = ParseTokenOne(U"-1."sv);
        CHECK_TK_TYPE(token, FP);
        EXPECT_EQ(token.GetDouble(), -1.);
    }
}


TEST(ParserBase, WrongNum)
{
    {
        const auto token = ParseTokenOne(U"1x"sv);
        CHECK_TK_TYPE(token, Error);
        EXPECT_EQ(token.GetString(), U"1x"sv);
    }
    {
        const auto token = ParseTokenOne(U"-1x"sv);
        CHECK_TK_TYPE(token, Error);
        EXPECT_EQ(token.GetString(), U"-1x"sv);
    }
    {
        const auto token = ParseTokenOne(U"1.x"sv);
        CHECK_TK_TYPE(token, Error);
        EXPECT_EQ(token.GetString(), U"1.x"sv);
    }
    {
        const auto token = ParseTokenOne(U"--3"sv);
        CHECK_TK_TYPE(token, Error);
        EXPECT_EQ(token.GetString(), U"--"sv);
    }
}


TEST(ParserBase, Tokens)
{
    constexpr auto test = [](const std::u32string_view src)
    {
        ParserContext context(src);
        ParserBase parser(context);
        std::vector<ParserToken> tokens;
        while (context.PeekNext() != ParserContext::CharEnd)
        {
            tokens.emplace_back(parser.GetToken(U","sv));
            if (tokens.back().GetIDEnum<BaseToken>() == BaseToken::End)
                break;
            context.GetNext();
            parser.IgnoreWhiteSpace();
        }
        return tokens;
    };
    {
        const auto tokens = test(U"0,1,2"sv);
        const auto types = MAP_TOKEN_TYPES(tokens);
        const auto vals = MAP_TOKENS(tokens, GetUInt());
        CHECK_TKS_TYPE(types, Uint, Uint, Uint);
        EXPECT_THAT(vals, testing::ElementsAre(0,1,2));
    }
}


