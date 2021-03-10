#pragma once

#include "common/parser/ParserBase.hpp"
#include "common/StringEx.hpp"
#include "NailangStruct.h"

namespace xziar::nailang
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
using common::parser::ASCIICheckerNBit;
using common::parser::ASCIIChecker;
using common::parser::BaseToken;
using common::parser::ParserToken;
using common::parser::ContextReader;
using common::parser::ParserBase;
using common::parser::ParserLexerBase;


enum class ExtraOps : uint16_t { Quest = 256, Colon };

namespace tokenizer
{

enum class NailangToken : uint16_t
{
    __RangeMin = common::enum_cast(BaseToken::__RangeMax),

    Raw, Block, MetaFunc, Func, Var, SubField, OpSymbol, Parenthese, SquareBracket, CurlyBrace, Assign,

    __RangeMax = 192
};

template<bool FullMatch, auto Tid, char32_t... Char>
class KeyCharTokenizer : public common::parser::tokenizer::TokenizerBase
{
public:
    using StateData = void;
    forceinline constexpr TokenizerResult OnChar(const char32_t ch, const size_t idx) const noexcept
    {
        if constexpr (FullMatch)
        {
            Expects(idx == 0);
            if ((... || (ch == Char)))
                return TokenizerResult::FullMatch;
        }
        else
        {
            if (idx == 0 && (... || (ch == Char)))
                return TokenizerResult::Waitlist;
        }
        return TokenizerResult::NotMatch;
    }
    forceinline constexpr ParserToken GetToken(ContextReader&, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() == 1);
        return ParserToken(Tid, txt[0]);
    }
};

class ParentheseTokenizer    : public KeyCharTokenizer<true,  NailangToken::Parenthese,    U'(', U')'>
{ };
class SquareBracketTokenizer : public KeyCharTokenizer<true,  NailangToken::SquareBracket, U'[', U']'>
{ };
class CurlyBraceTokenizer    : public KeyCharTokenizer<true,  NailangToken::CurlyBrace,    U'{', U'}'>
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
        constexpr auto GetTypeName = [](const auto name, const size_t prefix)
        {
            return name.size() > prefix&& name[prefix] == U'.' ?
                name.substr(prefix + 1) : U""sv;
        };
        if (common::str::IsBeginWith(fullname, U"Block"sv))
        {
            const auto typeName = GetTypeName(fullname, 5);
            return ParserToken(NailangToken::Block, typeName);
        }
        else if (common::str::IsBeginWith(fullname, U"Raw"sv))
        {
            const auto typeName = GetTypeName(fullname, 3);
            return ParserToken(NailangToken::Raw, typeName);
        }
        return ParserToken(BaseToken::Error, fullname);
    }
};

template<char32_t Pfx, NailangToken Type>
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

class MetaFuncPrefixTokenizer : public FuncPrefixTokenizer<U'@', NailangToken::MetaFunc>
{ };

class NormalFuncPrefixTokenizer : public FuncPrefixTokenizer<U'$', NailangToken::Func>
{ };


class VariableTokenizer
{
protected:
    static constexpr ASCIIChecker<true> HeadChecker = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_"sv;
    static constexpr ASCIIChecker<true> TailChecker = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_"sv;
public:
    using StateData = uint32_t;
    [[nodiscard]] forceinline constexpr std::pair<char32_t, TokenizerResult> 
        OnChar(const uint32_t state, const char32_t ch, const size_t idx) const noexcept
    {
        if (idx == 0)
        {
            if (ch == '`' || ch == ':')
                return { 0, TokenizerResult::Pending };
            if (HeadChecker(ch))
                return { 1, TokenizerResult::Waitlist };
        }
        else if (state == 0)
        {
            if (HeadChecker(ch))
                return { 1, TokenizerResult::Waitlist };
        }
        else
        {
            if (TailChecker(ch))
                return { 1, TokenizerResult::Waitlist };
        }
        return { 0, TokenizerResult::NotMatch };
    }
    [[nodiscard]] forceinline ParserToken GetToken(const uint32_t state, ContextReader&, std::u32string_view txt) const noexcept
    {
        Expects(state == 1);
        return ParserToken(NailangToken::Var, txt);
    }
    static constexpr std::optional<size_t> CheckName(const std::u32string_view name) noexcept
    {
        constexpr VariableTokenizer Self = {};
        if (name.empty()) 
            return SIZE_MAX;
        uint32_t state = 0;
        for (size_t i = 0; i < name.size(); ++i)
        {
            const auto [newState, ret] = Self.OnChar(state, name[i], i);
            if (ret == TokenizerResult::NotMatch)
                return i;
            state = newState;
        }
        if (state == 0)
            return name.size() - 1;
        return {};
    }
};

class SubFieldTokenizer : public common::parser::tokenizer::TokenizerBase
{
public:
    using StateData = void;
    [[nodiscard]] forceinline constexpr TokenizerResult OnChar(const char32_t ch, const size_t idx) const noexcept
    {
        using namespace std::string_view_literals;
        constexpr ASCIIChecker<true>  FirstChecker = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_"sv;
        constexpr ASCIIChecker<true> SecondChecker = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_"sv;
        switch (idx)
        {
        case 0:  return ch == '.' ? TokenizerResult::Pending : TokenizerResult::NotMatch;
        case 1:  return  FirstChecker(ch) ? TokenizerResult::Waitlist : TokenizerResult::NotMatch;
        default: return SecondChecker(ch) ? TokenizerResult::Waitlist : TokenizerResult::NotMatch;
        }
    }
    [[nodiscard]] forceinline ParserToken GetToken(ContextReader&, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() > 1);
        return ParserToken(NailangToken::SubField, txt.substr(1));
    }
};


class OpSymbolTokenizer
{
    static constexpr ASCIICheckerNBit<2> FirstChecker = []() -> ASCIICheckerNBit<2>
    {
        using common::enum_cast;
        static_assert(enum_cast(TokenizerResult::Pending)  <= 0b11);
        static_assert(enum_cast(TokenizerResult::Waitlist) <= 0b11);
        static_assert(enum_cast(TokenizerResult::NotMatch) <= 0b11);
        std::pair<char, uint8_t> mappings[] = 
        {
            { '=', enum_cast(TokenizerResult::Pending ) },
            { '!', enum_cast(TokenizerResult::Waitlist) },
            { '<', enum_cast(TokenizerResult::Waitlist) },
            { '>', enum_cast(TokenizerResult::Waitlist) },
            { '&', enum_cast(TokenizerResult::Pending ) },
            { '|', enum_cast(TokenizerResult::Pending ) },
            { '+', enum_cast(TokenizerResult::Waitlist) },
            { '-', enum_cast(TokenizerResult::Waitlist) },
            { '*', enum_cast(TokenizerResult::Waitlist) },
            { '/', enum_cast(TokenizerResult::Waitlist) },
            { '%', enum_cast(TokenizerResult::Waitlist) },
            { '?', enum_cast(TokenizerResult::Waitlist) },
            { ':', enum_cast(TokenizerResult::Waitlist) },
        };
        return { enum_cast(TokenizerResult::NotMatch), common::to_span(mappings) };
    }();
public:
    using StateData = char32_t;
    forceinline constexpr std::pair<char32_t, TokenizerResult> OnChar(const char32_t prevChar, const char32_t ch, const size_t idx) const noexcept
    {
        Expects((idx == 0) == (prevChar == 0));
        switch (idx)
        {
        case 0: // just begin
            return { ch, static_cast<TokenizerResult>(FirstChecker(ch, common::enum_cast(TokenizerResult::NotMatch))) };
        case 1:
            switch (prevChar)
            {
            case U'=': return { ch, ch == U'=' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            case U'!': return { ch, ch == U'=' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            case U'<': return { ch, ch == U'=' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            case U'>': return { ch, ch == U'=' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            case U'&': return { ch, ch == U'&' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            case U'|': return { ch, ch == U'|' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            case U'?': return { ch, ch == U'?' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            default:   return { ch, TokenizerResult::NotMatch };
            }
        default:
            return { ch, TokenizerResult::NotMatch };
        }
    }
    forceinline constexpr ParserToken GetToken(char32_t, ContextReader&, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() == 1 || txt.size() == 2);
#define RET_OP(str, op) case str ## _hash: return ParserToken( NailangToken::OpSymbol, common::enum_cast(op))
        switch (common::DJBHash::HashC(txt))
        {
        RET_OP("==", EmbedOps::Equal);
        RET_OP("!=", EmbedOps::NotEqual);
        RET_OP("<",  EmbedOps::Less);
        RET_OP("<=", EmbedOps::LessEqual);
        RET_OP(">",  EmbedOps::Greater);
        RET_OP(">=", EmbedOps::GreaterEqual);
        RET_OP("&&", EmbedOps::And);
        RET_OP("||", EmbedOps::Or);
        RET_OP("+",  EmbedOps::Add);
        RET_OP("-",  EmbedOps::Sub);
        RET_OP("*",  EmbedOps::Mul);
        RET_OP("/",  EmbedOps::Div);
        RET_OP("%",  EmbedOps::Rem);
        RET_OP("??", EmbedOps::ValueOr);
        RET_OP("!",  EmbedOps::Not);
        RET_OP("?",  ExtraOps::Quest);
        RET_OP(":",  ExtraOps::Colon);
        default:     return ParserToken(BaseToken::Error, txt);
        }
#undef RET_OP
    }
};

enum class AssignOps : uint8_t { Assign = 0, AndAssign, OrAssign, AddAssign, SubAssign, MulAssign, DivAssign, RemAssign, NilAssign, NewCreate };
class AssignOpTokenizer
{
    static constexpr ASCIICheckerNBit<2> FirstChecker = []() -> ASCIICheckerNBit<2>
    {
        using common::enum_cast;
        static_assert(enum_cast(TokenizerResult::Pending)  <= 0b11);
        static_assert(enum_cast(TokenizerResult::Waitlist) <= 0b11);
        static_assert(enum_cast(TokenizerResult::NotMatch) <= 0b11);
        std::pair<char, uint8_t> mappings[] = 
        {
            { '=', enum_cast(TokenizerResult::Waitlist) },
            { '&', enum_cast(TokenizerResult::Pending ) },
            { '|', enum_cast(TokenizerResult::Pending ) },
            { '+', enum_cast(TokenizerResult::Pending ) },
            { '-', enum_cast(TokenizerResult::Pending ) },
            { '*', enum_cast(TokenizerResult::Pending ) },
            { '/', enum_cast(TokenizerResult::Pending ) },
            { '%', enum_cast(TokenizerResult::Pending ) },
            { '?', enum_cast(TokenizerResult::Pending ) },
            { ':', enum_cast(TokenizerResult::Pending ) },
        };
        return { enum_cast(TokenizerResult::NotMatch), common::to_span(mappings) };
    }();
public:
    using StateData = char32_t;
    forceinline constexpr std::pair<char32_t, TokenizerResult> OnChar(const char32_t prevChar, const char32_t ch, const size_t idx) const noexcept
    {
        Expects((idx == 0) == (prevChar == 0));
        switch (idx)
        {
        case 0: // just begin
            return { ch, static_cast<TokenizerResult>(FirstChecker(ch, common::enum_cast(TokenizerResult::NotMatch))) };
        case 1:
            if (prevChar != U'=' && ch == U'=')
                return { ch, TokenizerResult::Waitlist };
        [[fallthrough]];
        default:
            return { ch, TokenizerResult::NotMatch };
        }
    }
    forceinline constexpr ParserToken GetToken(char32_t, ContextReader&, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() == 1 || txt.size() == 2);
#define RET_OP(str, op) case str ## _hash: return ParserToken( NailangToken::Assign, common::enum_cast(AssignOps::op))
        switch (common::DJBHash::HashC(txt))
        {
        RET_OP("=",     Assign);
        RET_OP("&=", AndAssign);
        RET_OP("|=",  OrAssign);
        RET_OP("+=", AddAssign);
        RET_OP("-=", SubAssign);
        RET_OP("*=", MulAssign);
        RET_OP("/=", DivAssign);
        RET_OP("%=", RemAssign);
        RET_OP("?=", NilAssign);
        RET_OP(":=", NewCreate);
        default:     return ParserToken(BaseToken::Error, txt);
        }
#undef RET_OP
    }
};


}

constexpr static auto IgnoreBlank = ASCIIChecker("\r\n\v\t ");


}
