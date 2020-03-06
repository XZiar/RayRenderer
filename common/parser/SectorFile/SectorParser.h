#pragma once
#include "SectorFileStruct.h"
#include "ParserBase.h"


namespace xziar::sectorlang
{


class SectorsParser : public SectorFileParser
{
public:
    using SectorFileParser::SectorFileParser;
    
    SectorRaw ParseNextSector();
    std::vector<SectorRaw> ParseAllSectors();
};


class SectorParser : public SectorFileParser
{
private:
    const SectorRaw& Source;
    SectorParser(MemoryPool& pool, common::parser::ParserContext& context, const SectorRaw& source) :
        SectorFileParser(pool, context), Source(source) { }
    void ParseSectorRaw();
public:
    virtual ~SectorParser() { }
    static void ParseSector(const SectorRaw& sector, MemoryPool& pool);
};


}
