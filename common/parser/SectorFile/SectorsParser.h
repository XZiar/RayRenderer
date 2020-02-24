#pragma once
#include "SectorsStruct.h"
#include "common/parser/ParserBase.hpp"


namespace xziar::sectorlang
{


class SectorsParser : public common::parser::ParserBase
{
public:
    SectorsParser(common::parser::ParserContext& context) : ParserBase(context) { }
    SectorRaw ParseNextSector();
    std::vector<SectorRaw> ParseAllSectors();
};


}
