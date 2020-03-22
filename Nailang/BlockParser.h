#pragma once
#include "NailangStruct.h"
#include "ParserBase.h"


namespace xziar::nailang
{


class RawBlockParser : public NailangParser
{
protected:
    void EatSemiColon();
    void FillBlockName(RawBlock& block);
    void FillBlockInfo(RawBlock& block);
    RawBlock FillRawBlock(const std::u32string_view name);
public:
    using NailangParser::NailangParser;
    
    RawBlockWithMeta GetNextRawBlock();
    std::vector<RawBlockWithMeta> GetAllRawBlocks();
};

class BlockParser : public RawBlockParser
{
protected:
    void ThrowNonSupport(common::parser::ParserToken token, std::u16string_view detail);
    Assignment ParseAssignment(const std::u32string_view var);
    template<bool AllowNonBlock>
    void ParseContentIntoBlock(Block& block, const bool tillTheEnd = true);
    using RawBlockParser::RawBlockParser;
public:
    static Block ParseRawBlock(const RawBlock& block, MemoryPool& pool);
    static Block ParseAllAsBlock(MemoryPool& pool, common::parser::ParserContext& context);
};


}
