#pragma once
#include "NailangStruct.h"
#include "ParserBase.h"


namespace xziar::nailang
{


class BlockParser : public SectorFileParser
{
protected:
    void EatSemiColon();
    RawBlock FillBlock(const std::u32string_view name);

    Assignment ParseAssignment(const std::u32string_view var);
    void ParseBlockContent(Block& block);
public:
    using SectorFileParser::SectorFileParser;
    
    RawBlockWithMeta GetNextBlock();
    std::vector<RawBlockWithMeta> GetAllBlocks();

    static Block ParseBlockRaw(const RawBlock& block, MemoryPool& pool);
};


}
