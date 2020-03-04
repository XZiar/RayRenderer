#include "ParserBase.h"
#include "ParserRely.h"

namespace xziar::sectorlang
{
using tokenizer::SectorLangToken;

template<typename ExpectBracket, typename... TKs>
void SectorFileParser::EatBracket()
{
    using common::parser::detail::TokenMatcherHelper;
    using common::parser::detail::EmptyTokenArray;

    constexpr ParserToken ExpectToken = ExpectBracket::Token;
    constexpr auto ExpectMatcher = TokenMatcherHelper::GetMatcher(std::array{ ExpectToken });

    constexpr auto Lexer = ParserLexerBase<CommentTokenizer, TKs...>();
    ExpectNextToken(Lexer, IgnoreBlank, IgnoreCommentToken, ExpectMatcher);
}

struct ExpectParentheseL
{
    static constexpr ParserToken Token = ParserToken(SectorLangToken::Parenthese, U'(');
};
struct ExpectParentheseR
{
    static constexpr ParserToken Token = ParserToken(SectorLangToken::Parenthese, U')');
};
struct ExpectCurlyBraceL
{
    static constexpr ParserToken Token = ParserToken(SectorLangToken::CurlyBrace, U'{');
};
struct ExpectCurlyBraceR
{
    static constexpr ParserToken Token = ParserToken(SectorLangToken::CurlyBrace, U'}');
};


std::u16string SectorFileParser::DescribeTokenID(const uint16_t tid) const noexcept
{
#define RET_TK_ID(type) case SectorLangToken::type:        return u ## #type
    switch (static_cast<SectorLangToken>(tid))
    {
        RET_TK_ID(Sector);
        RET_TK_ID(Block);
        RET_TK_ID(MetaFunc);
        RET_TK_ID(Func);
        RET_TK_ID(Var);
        RET_TK_ID(EmbedOp);
        RET_TK_ID(Parenthese);
        RET_TK_ID(CurlyBrace);
#undef RET_TK_ID
    default:
        return ParserBase::DescribeTokenID(tid);
    }
}

void SectorFileParser::EatLeftParenthese()
{
    EatBracket<ExpectParentheseL, tokenizer::ParentheseTokenizer>();
}
void SectorFileParser::EatRightParenthese()
{
    EatBracket<ExpectParentheseR, tokenizer::ParentheseTokenizer>();
}
void SectorFileParser::EatLeftCurlyBrace()
{
    EatBracket<ExpectCurlyBraceL, tokenizer::CurlyBraceTokenizer>();
}
void SectorFileParser::EatRightCurlyBrace()
{
    EatBracket<ExpectCurlyBraceR, tokenizer::CurlyBraceTokenizer>();
}


}