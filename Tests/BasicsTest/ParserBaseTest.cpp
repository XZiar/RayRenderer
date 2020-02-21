#include "pch.h"
#include "common/parser/ParserBase.hpp"


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


struct FuncParser : ParserBase
{
public:
    FuncParser(ParserContext& context) : ParserBase(context) { }
    std::pair<std::u32string_view, std::vector<ParserToken>> GetFunc()
    {
        return {};
    }
};

struct DummyParser : ParserBase
{
    DummyParser(ParserContext& context) : ParserBase(context) { }
    using ParserBase::GetNextToken;
    using ParserBase::ExpectNextToken;
    using ParserBase::IgnoreComment;
};

struct DummyTokenizer : TokenizerBase
{
    using TokenizerBase::GenerateToken;
};


TEST(ParserBase, GetNext)
{
    constexpr ParserLexerBase<CommentTokenizer, DelimTokenizer, ASCIIRawTokenizer> Lexer;
    constexpr ASCIIChecker ignore = " \t\r\n\v"sv;
    constexpr auto source = U"/*Empty*/Hello,There"sv;
    {
        ParserContext context(source);
        DummyParser parser(context);
        auto lexer = Lexer;

        {
            const auto token = parser.GetNextToken(lexer, ignore);
            CHECK_TK(token, Comment, GetString, U"Empty"sv);
        }
        {
            const auto token = parser.GetNextToken(lexer, ignore);
            CHECK_TK(token, Raw, GetString, U"Hello"sv);
        }
    }
    {
        ParserContext context(source);
        DummyParser parser(context);
        auto lexer = Lexer;

        {
            const auto token = parser.GetNextToken(lexer, ignore, parser.IgnoreComment);
            CHECK_TK(token, Raw, GetString, U"Hello"sv);
        }
        {
            const auto token = parser.GetNextToken(lexer, ignore, parser.IgnoreComment);
            CHECK_TK(token, Delim, GetChar, U',');
        }
        {
            const auto token = parser.GetNextToken(lexer, ignore, parser.IgnoreComment);
            CHECK_TK(token, Raw, GetString, U"There"sv);
        }
    }
    {
        constexpr auto ExpectDelim = detail::TokenMatcherHelper::GetMatcher
            (detail::EmptyTokenArray{}, BaseToken::Delim);
        const auto TokenHello = DummyTokenizer::GenerateToken(BaseToken::Raw, U"Hello"sv);
        const auto ExpectHello = detail::TokenMatcherHelper::GetMatcher(std::array{ TokenHello });
        const auto TokenThere = DummyTokenizer::GenerateToken(BaseToken::Raw, U"There"sv);
        const auto ExpectThere = detail::TokenMatcherHelper::GetMatcher(std::array{ TokenThere });
        ParserContext context(source);
        DummyParser parser(context);
        auto lexer = Lexer;

        {
            const auto token = parser.ExpectNextToken(lexer, ignore, parser.IgnoreComment, ExpectHello);
            CHECK_TK(token, Raw, GetString, U"Hello"sv);
        }
        {
            const auto token = parser.ExpectNextToken(lexer, ignore, parser.IgnoreComment, ExpectDelim);
            CHECK_TK(token, Delim, GetChar, U',');
        }
        {
            const auto token = parser.ExpectNextToken(lexer, ignore, parser.IgnoreComment, ExpectThere);
            CHECK_TK(token, Raw, GetString, U"There"sv);
        }
        {
            EXPECT_THROW(parser.ExpectNextToken(lexer, ignore, parser.IgnoreComment, ExpectThere), detail::ParsingError);
        }
    }
}
