#include "ParserBase.h"
#include "ParserRely.h"

namespace xziar::nailang
{
using tokenizer::SectorLangToken;


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


std::u16string NailangParser::DescribeTokenID(const uint16_t tid) const noexcept
{
#define RET_TK_ID(type) case SectorLangToken::type:        return u ## #type
    switch (static_cast<SectorLangToken>(tid))
    {
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

common::SharedString<char16_t> NailangParser::GetCurrentFileName() const noexcept
{
    if (SubScopeName.empty())
        return ParserBase::GetCurrentFileName();
    
    std::u16string fileName;
    fileName.reserve(Context.SourceName.size() + SubScopeName.size() + 3);
    fileName.append(Context.SourceName).append(u" ["sv).append(SubScopeName).append(u"]");
    return fileName;
}

void NailangParser::EatLeftParenthese()
{
    EatSingleToken<ExpectParentheseL, tokenizer::ParentheseTokenizer>();
}
void NailangParser::EatRightParenthese()
{
    EatSingleToken<ExpectParentheseR, tokenizer::ParentheseTokenizer>();
}
void NailangParser::EatLeftCurlyBrace()
{
    EatSingleToken<ExpectCurlyBraceL, tokenizer::CurlyBraceTokenizer>();
}
void NailangParser::EatRightCurlyBrace()
{
    EatSingleToken<ExpectCurlyBraceR, tokenizer::CurlyBraceTokenizer>();
}


}