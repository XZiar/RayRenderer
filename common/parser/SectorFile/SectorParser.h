#pragma once
#include "SectorFileStruct.h"
#include "ParserBase.h"


namespace xziar::sectorlang
{


class SectorParser : public SectorFileParser
{
public:
    using SectorFileParser::SectorFileParser;
    
    SectorRaw ParseNextSector();
    std::vector<SectorRaw> ParseAllSectors();
};


}
