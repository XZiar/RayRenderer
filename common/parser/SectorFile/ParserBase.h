#pragma once
#include "SectorFileStruct.h"
#include "common/parser/ParserBase.hpp"


namespace xziar::sectorlang
{


class SectorFileParser : public common::parser::ParserBase
{
private:
    template<typename ExpectBracket, typename... TKs>
    void EatBracket();
protected:
    MemoryPool& MemPool;
    std::u16string SubScopeName;

    std::u16string DescribeTokenID(const uint16_t tid) const noexcept override;
    common::SharedString<char16_t> GetCurrentFileName() const noexcept override;

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