#pragma once
#include "NailangStruct.h"
#include "ParserBase.h"
#include <optional>


namespace xziar::nailang
{

class ComplexArgParser : public SectorFileParser
{
private:
    using SectorFileParser::SectorFileParser;

    template<typename StopDelimer>
    std::pair<std::optional<RawArg>, char32_t> ParseArg();
public:
    static FuncCall ParseFuncBody(std::u32string_view funcName, MemoryPool& pool, common::parser::ParserContext& context);
    static std::optional<RawArg> ParseSingleStatement(MemoryPool& pool, common::parser::ParserContext& context);
};


}
