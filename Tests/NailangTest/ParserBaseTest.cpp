#include "rely.h"
#include "common/parser/ParserBase.hpp"


using namespace common::parser;
using namespace common::parser::tokenizer;
using namespace std::string_view_literals;
using common::parser::detail::TokenMatcherHelper;
using CharType = common::parser::ParserContext::CharType;


#define CHECK_POS(ctx, row, col) EXPECT_EQ(ctx.Row, row); EXPECT_EQ(ctx.Col, col)
#define CHECK_CHTYPE(ch, type) EXPECT_EQ(ParserContext::ParseType(ch), CharType::type)
#define CHECK_TK_TYPE(token, type) EXPECT_EQ(token.GetIDEnum(), BaseToken::type)

#define CHECK_TK(token, type, action, val) do { CHECK_TK_TYPE(token, type); EXPECT_EQ(token.action(), val); } while(0)


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
    using ParserBase::IgnoreCommentToken;
};


TEST(ParserBase, GetNext)
{
    constexpr ParserLexerBase<CommentTokenizer, DelimTokenizer, ASCIIRawTokenizer> lexer;
    constexpr common::ASCIIChecker ignore = " \t\r\n\v"sv;
    constexpr auto source = U"/*Empty*/Hello,There"sv;
    {
        ParserContext context(source);
        DummyParser parser(context);
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
        {
            const auto token = parser.GetNextToken(lexer, ignore, parser.IgnoreCommentToken);
            CHECK_TK(token, Raw, GetString, U"Hello"sv);
        }
        {
            const auto token = parser.GetNextToken(lexer, ignore, parser.IgnoreCommentToken);
            CHECK_TK(token, Delim, GetChar, U',');
        }
        {
            const auto token = parser.GetNextToken(lexer, ignore, parser.IgnoreCommentToken);
            CHECK_TK(token, Raw, GetString, U"There"sv);
        }
    }
    {
        constexpr auto ExpectDelim = TokenMatcherHelper::GetMatcher
            (detail::EmptyTokenArray{}, BaseToken::Delim);
        constexpr ParserToken TokenHello(BaseToken::Raw, U"Hello"sv);
        constexpr auto ExpectHello = TokenMatcherHelper::GetMatcher(std::array{ TokenHello });
        constexpr ParserToken TokenThere(BaseToken::Raw, U"There"sv);
        constexpr auto ExpectThere = TokenMatcherHelper::GetMatcher(std::array{ TokenThere });
        ParserContext context(source);
        DummyParser parser(context);
        {
            const auto token = parser.ExpectNextToken(lexer, ignore, parser.IgnoreCommentToken, ExpectHello);
            CHECK_TK(token, Raw, GetString, U"Hello"sv);
        }
        {
            const auto token = parser.ExpectNextToken(lexer, ignore, parser.IgnoreCommentToken, ExpectDelim);
            CHECK_TK(token, Delim, GetChar, U',');
        }
        {
            const auto token = parser.ExpectNextToken(lexer, ignore, parser.IgnoreCommentToken, ExpectThere);
            CHECK_TK(token, Raw, GetString, U"There"sv);
        }
        {
            EXPECT_THROW(parser.ExpectNextToken(lexer, ignore, parser.IgnoreCommentToken, ExpectThere), detail::ParsingError);
        }
    }
}
