#pragma once

#include "common/parser/ParserBase.hpp"
#include "common/StringEx.hpp"
#include "SectorFileStruct.h"

namespace xziar::sectorlang
{
using namespace std::string_view_literals;
using common::parser::tokenizer::TokenizerResult;
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

    Block, MetaFunc, Func, Var, EmbedOp, Parenthese, CurlyBrace,

    __RangeMax = 192
};

template<auto Tid, char32_t... Char>
class KeyCharTokenizer : public common::parser::tokenizer::TokenizerBase
{
public:
    using StateData = void;
    forceinline constexpr TokenizerResult OnChar(const char32_t ch, const size_t idx) const noexcept
    {
        Expects(idx == 0);
        if ((... || (ch == Char)))
            return TokenizerResult::FullMatch;
        else
            return TokenizerResult::NotMatch;
    }
    forceinline constexpr ParserToken GetToken(ContextReader&, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() == 1);
        return ParserToken(Tid, txt[0]);
    }
};

class ParentheseTokenizer : public KeyCharTokenizer<SectorLangToken::Parenthese, U'(', U')'>
{ };
class CurlyBraceTokenizer : public KeyCharTokenizer<SectorLangToken::CurlyBrace, U'{', U'}'>
{ };

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
    using StateData = void;
    forceinline constexpr TokenizerResult OnChar(const char32_t ch, const size_t idx) const noexcept
    {
        Expects(idx == 0);
        if (ch == Pfx)
            return TokenizerResult::FullMatch;
        else
            return TokenizerResult::NotMatch;
    }
};

class BlockPrefixTokenizer : public PrefixedTokenizer<U'#'>
{
public:
    forceinline constexpr ParserToken GetToken(ContextReader& reader, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() == 1);
        const auto fullname = ReadFullName(reader);

        if (common::str::IsBeginWith(fullname, U"Block."sv))
        {
            const auto typeName = fullname.substr(6);
            if (typeName.size() > 0)
                return ParserToken(SectorLangToken::Block, typeName);
        }
        return ParserToken(BaseToken::Error, fullname);
    }
};

template<char32_t Pfx, SectorLangToken Type>
class FuncPrefixTokenizer : public PrefixedTokenizer<Pfx>
{
public:
    forceinline constexpr ParserToken GetToken(ContextReader& reader, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() == 1);
        const auto funcname = PrefixedTokenizer<Pfx>::ReadFullName(reader);
        if (funcname.empty())
            return ParserToken(BaseToken::Error, funcname);
        return ParserToken(Type, funcname);
    }
};

class MetaFuncPrefixTokenizer : public FuncPrefixTokenizer<U'@', SectorLangToken::MetaFunc>
{ };

class NormalFuncPrefixTokenizer : public FuncPrefixTokenizer<U'$', SectorLangToken::Func>
{ };


class VariableTokenizer : public ASCII2PartTokenizer
{
public:
    constexpr VariableTokenizer() : ASCII2PartTokenizer(SectorLangToken::Var, 1, 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_", 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.")
    { }
};

class EmbedOpTokenizer
{
public:
    using StateData = char32_t;
    constexpr std::pair<char32_t, TokenizerResult> OnChar(const char32_t prevChar, const char32_t ch, const size_t idx) const noexcept
    {
        Expects((idx == 0) == (prevChar == 0));
        switch (idx)
        {
        case 0: // just begin
            switch (ch)
            {
            case U'=':  return { ch, TokenizerResult::Pending  };
            case U'!':  return { ch, TokenizerResult::Waitlist };
            case U'<':  return { ch, TokenizerResult::Waitlist };
            case U'>':  return { ch, TokenizerResult::Waitlist };
            case U'&':  return { ch, TokenizerResult::Pending  };
            case U'|':  return { ch, TokenizerResult::Pending  };
            case U'+':  return { ch, TokenizerResult::Waitlist };
            case U'-':  return { ch, TokenizerResult::Waitlist };
            case U'*':  return { ch, TokenizerResult::Waitlist };
            case U'/':  return { ch, TokenizerResult::Waitlist };
            case U'%':  return { ch, TokenizerResult::Waitlist };
            default:    return { ch, TokenizerResult::NotMatch };
            }
        case 1:
            switch (prevChar)
            {
            case U'=': return { ch, ch == U'=' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            case U'!': return { ch, ch == U'=' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            case U'<': return { ch, ch == U'=' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            case U'>': return { ch, ch == U'=' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            case U'&': return { ch, ch == U'&' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            case U'|': return { ch, ch == U'|' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            default:   return { ch, TokenizerResult::NotMatch };
            }
        default:
            return { ch, TokenizerResult::NotMatch };
        }
    }
    forceinline constexpr ParserToken GetToken(char32_t, ContextReader&, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() == 1 || txt.size() == 2);
#define RET_OP(str, op) case str ## _hash: return ParserToken( SectorLangToken::EmbedOp, common::enum_cast(EmbedOps::op))
        switch (hash_(txt))
        {
        RET_OP("==", Equal);
        RET_OP("!=", NotEqual);
        RET_OP("<",  Less);
        RET_OP("<=", LessEqual);
        RET_OP(">",  Greater);
        RET_OP(">=", GreaterEqual);
        RET_OP("&&", And);
        RET_OP("||", Or);
        RET_OP("!",  Not);
        RET_OP("+",  Add);
        RET_OP("-",  Sub);
        RET_OP("*",  Mul);
        RET_OP("/",  Div);
        RET_OP("%",  Rem);
        default:     return ParserToken(BaseToken::Error, txt);
        }
#undef RET_OP
    }
};


}

constexpr static auto IgnoreBlank = ASCIIChecker("\r\n\v\t ");


}
