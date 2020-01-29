#pragma once

#include "ParserContext.hpp"


namespace common::parser
{


class ParserBase
{
public:
protected:
    ParserContext& Context;
    ParserBase(ParserContext& ctx) : Context(ctx) { }
};


}

