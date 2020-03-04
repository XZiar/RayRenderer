#pragma once
#include "SectorFileStruct.h"
#include "common/parser/ParserBase.hpp"


namespace xziar::sectorlang
{


class SectorParser : public common::parser::ParserBase
{
private:
    MemoryPool& MemPool;
public:
    SectorParser(MemoryPool& pool, common::parser::ParserContext& context) :
        ParserBase(context), MemPool(pool) { }
    SectorRaw ParseNextSector();
    std::vector<SectorRaw> ParseAllSectors();
};


}
