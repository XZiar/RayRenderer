#pragma once
#include "SectorFileStruct.h"
#include "ParserBase.h"


namespace xziar::sectorlang
{


class BlockParser : public SectorFileParser
{
protected:
    void EatSemiColon();
    RawBlockWithMeta FillBlock(const std::u32string_view name, const std::vector<FuncCall>& metaFuncs);

    AssignmentWithMeta ParseAssignment(const std::u32string_view var);
    std::vector<BlockContent> ParseBlockContent();
public:
    using SectorFileParser::SectorFileParser;
    
    RawBlockWithMeta GetNextBlock();
    std::vector<RawBlockWithMeta> GetAllBlocks();

    static Block ParseBlockRaw(const RawBlock& block, MemoryPool& pool);
};


}
