#pragma once
#include "SectorFileStruct.h"
#include "common/parser/ParserBase.hpp"
#include <optional>


namespace xziar::sectorlang
{

class ComplexArgParser : public common::parser::ParserBase
{
private:
    MemoryPool& MemPool;
    ComplexArgParser(MemoryPool& pool, common::parser::ParserContext& context) :
        ParserBase(context), MemPool(pool) { }
    template<typename StopDelimer>
    std::pair<std::optional<FuncArgRaw>, char32_t> ParseArg();
    void EatLeftBracket();
public:
    static FuncCall ParseFuncBody(std::u32string_view funcName, MemoryPool& pool, common::parser::ParserContext& context);
};


}
