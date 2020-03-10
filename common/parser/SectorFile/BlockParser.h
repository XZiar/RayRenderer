#pragma once
#include "SectorFileStruct.h"
#include "ParserBase.h"


namespace xziar::sectorlang
{


class BlockParser : public SectorFileParser
{
protected:
    RawBlockWithMeta FillBlock(const std::u32string_view name, const std::vector<FuncCall>& metaFuncs);

    AssignmentWithMeta ParseAssignment(const std::u32string_view var);
    Block ParseBlockContent();
public:
    using SectorFileParser::SectorFileParser;
    
    RawBlockWithMeta GetNextBlock();
    std::vector<RawBlockWithMeta> GetAllBlocks();

    static Block ParseBlockRaw(const RawBlock& block, MemoryPool& pool);
};


}
