#pragma once
#include "SectorsStruct.h"
#include "common/parser/ParserBase.hpp"


namespace xziar::sectorlang
{


class MetaFuncParser : public common::parser::ParserBase
{
private:
    MetaFuncParser(common::parser::ParserContext& context) :
        ParserBase(context) { }
    void FillFuncBody(MetaFunc& func);
public:
    static MetaFunc ParseFuncBody(std::u32string_view funcName, common::parser::ParserContext& context);

};


}
