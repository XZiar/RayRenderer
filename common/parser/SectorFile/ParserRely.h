#pragma once

#include "common/parser/ParserBase.hpp"
#include "common/StringEx.hpp"

namespace xziar::sectorlang
{
using namespace std::string_view_literals;
using common::parser::tokenizer::TokenizerResult;
using common::parser::tokenizer::StateData;
using common::parser::tokenizer::ASCIIRawTokenizer;
using common::parser::tokenizer::CommentTokenizer;
using common::parser::tokenizer::DelimTokenizer;
using common::parser::tokenizer::StringTokenizer;
using common::parser::tokenizer::IntTokenizer;
using common::parser::tokenizer::FPTokenizer;
using common::parser::tokenizer::BoolTokenizer;
using common::parser::tokenizer::ASCII2PartTokenizer;
using common::parser::ASCIIChecker;
using common::parser::BaseToken;
using common::parser::ParserToken;
using common::parser::ContextReader;
using common::parser::ParserBase;
using common::parser::ParserLexerBase;


namespace tokenizer
{

enum class SectorLangToken : uint16_t
{
    __RangeMin = common::enum_cast(BaseToken::__RangeMax),

    Sector, Block, MetaFunc, Func, Var,

    __RangeMax = 192
};


template<char32_t Pfx>
class PrefixedTokenizer : public common::parser::tokenizer::TokenizerBase
{
protected:
    static forceinline constexpr std::u32string_view ReadFullName(ContextReader& reader) noexcept
    {
        constexpr auto FullnameMatcher = ASCIIChecker<false>("\r\n\v\t (");
        return reader.ReadWhile(FullnameMatcher);
    }
public:
    constexpr TokenizerResult OnChar(StateData, const char32_t ch, const size_t idx) const noexcept
    {
        if (idx == 0 && ch == Pfx)
            return TokenizerResult::FullMatch;
        return TokenizerResult::NotMatch;
    }
};

class BlockPrefixTokenizer : public PrefixedTokenizer<U'$'>
{
public:
    forceinline constexpr ParserToken GetToken(StateData, ContextReader& reader, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() == 1);
        const auto fullname = ReadFullName(reader);

        if (common::str::IsBeginWith(fullname, U"Sector."sv))
        {
            const auto typeName = fullname.substr(7);
            if (typeName.size() > 0)
                return ParserToken(SectorLangToken::Sector, typeName);
        }
        else if (common::str::IsBeginWith(fullname, U"Block."sv))
        {
            const auto typeName = fullname.substr(6);
            if (typeName.size() > 0)
                return ParserToken(SectorLangToken::Block, typeName);
        }
        return ParserToken(BaseToken::Error, fullname);
    }
};

class MetaFuncPrefixTokenizer : public PrefixedTokenizer<U'@'>
{
public:
    forceinline constexpr ParserToken GetToken(StateData, ContextReader& reader, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() == 1);
        const auto funcname = ReadFullName(reader);
        if (funcname.empty())
            return ParserToken(BaseToken::Error, funcname);
        return ParserToken(SectorLangToken::MetaFunc, funcname);

    }
};

class VariableTokenizer : public ASCII2PartTokenizer
{
public:
    constexpr VariableTokenizer() : ASCII2PartTokenizer(SectorLangToken::Var, 1, 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_", 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.")
    { }
};


}

constexpr static auto IgnoreBlank = ASCIIChecker("\r\n\v\t ");


}
