#pragma once
#include "SectorsStruct.h"
#include "common/parser/ParserBase.hpp"
#include <optional>


namespace xziar::sectorlang
{

class ComplexArgParser : public common::parser::ParserBase
{
private:
    ComplexArgParser(common::parser::ParserContext& context) :
        ParserBase(context) { }
    template<typename StopDelimer>
    std::optional<FuncArgRaw> ParseArg();
public:
    static FuncCall ParseFuncBody(std::u32string_view funcName, common::parser::ParserContext& context);
};

class FuncBodyParser : public common::parser::ParserBase
{
private:
    FuncBodyParser(common::parser::ParserContext& context) :
        ParserBase(context) { }
    void FillFuncBody(MetaFunc& func);
public:
    static MetaFunc ParseFuncBody(std::u32string_view funcName, common::parser::ParserContext& context);

};


}
