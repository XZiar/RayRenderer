#pragma once
#include "SectorFileStruct.h"
#include "common/parser/ParserBase.hpp"


namespace xziar::sectorlang
{


class SectorFileParser : public common::parser::ParserBase
{
private:
protected:
    MemoryPool& MemPool;
    std::u16string SubScopeName;

    std::u16string DescribeTokenID(const uint16_t tid) const noexcept override;
    common::SharedString<char16_t> GetCurrentFileName() const noexcept override;

    template<typename ExpectHolder, typename... TKs>
    void EatSingleToken()
    {
        //constexpr common::parser::ParserToken ExpectToken = ExpectHolder::Token;
        constexpr auto ExpectMatcher = common::parser::detail::TokenMatcherHelper::GetMatcher(std::array{ ExpectHolder::Token });
        constexpr auto IgnoreChar = common::parser::ASCIIChecker("\r\n\v\t ");
        constexpr auto Lexer = common::parser::ParserLexerBase<common::parser::tokenizer::CommentTokenizer, TKs...>();
        ExpectNextToken(Lexer, IgnoreChar, IgnoreCommentToken, ExpectMatcher);
    }

    void EatLeftParenthese();
    void EatRightParenthese();
    void EatLeftCurlyBrace();
    void EatRightCurlyBrace();
public:
    SectorFileParser(MemoryPool& pool, common::parser::ParserContext& context, std::u16string subScope = u"") :
        ParserBase(context), MemPool(pool), SubScopeName(std::move(subScope)) { }
    virtual ~SectorFileParser() { }
};


}