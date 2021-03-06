#pragma once
#include "rely.h"
#include "common/parser/ParserLexer.hpp"

template<typename Lexer>
auto TKParse_(const std::u32string_view src, const Lexer& lexer)
{
    using namespace common::parser;
    using namespace std::string_view_literals;
    constexpr ASCIIChecker ignore = " \t\r\n\v"sv;
    ParserContext context(src);
    std::vector<DetailToken> tokens;
    while (true)
    {
        const auto token = lexer.GetTokenBy(context, ignore);
        const auto type = token.GetIDEnum();
        if (type != BaseToken::End)
            tokens.emplace_back(token);
        if (type == BaseToken::Error || type == BaseToken::Unknown || type == BaseToken::End)
            break;
    }
    return tokens;
}

template<typename... TKs>
auto TKParse(const std::u32string_view src)
{
    constexpr common::parser::ParserLexerBase<TKs...> lexer;
    return TKParse_(src, lexer);
}

