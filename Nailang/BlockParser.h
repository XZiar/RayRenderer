#pragma once
#include "NailangStruct.h"
#include "ParserBase.h"


namespace xziar::nailang
{


class BlockParser : public NailangParser
{
protected:
    void EatSemiColon();
    void FillBlockName(RawBlock& block);
    void FillBlockInfo(RawBlock& block);
    RawBlock FillBlock(const std::u32string_view name);

    Assignment ParseAssignment(const std::u32string_view var);
    void ParseBlockContent(Block& block, const bool tillTheEnd = true);
public:
    using NailangParser::NailangParser;
    
    RawBlockWithMeta GetNextBlock();
    std::vector<RawBlockWithMeta> GetAllBlocks();

    static Block ParseBlockRaw(const RawBlock& block, MemoryPool& pool);
};


}
