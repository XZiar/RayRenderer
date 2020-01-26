#pragma once

#include "ParserContext.hpp"


namespace common::parser
{


class ParserBase
{
protected:
    ParserContext& Context;
    ParserBase(ParserContext& ctx) : Context(ctx) { }
};


}

