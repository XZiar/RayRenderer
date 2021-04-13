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
enum class AssignOps : uint16_t { Assign = 384, AddAssign, SubAssign, MulAssign, DivAssign, RemAssign, NilAssign, NewCreate };

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
    forceinline static constexpr ParserToken GetToken(ContextReader& reader, std::u32string_view txt) noexcept
    {
        Expects(txt.size() == 1);
        const auto fullname = ReadFullName(reader);
        constexpr auto GetTypeName = [](const auto name, const size_t prefix)
        {
            return name.size() > prefix && name[prefix] == U'.' ?
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
    forceinline static constexpr ParserToken GetToken(ContextReader& reader, std::u32string_view txt) noexcept
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
            { '=', enum_cast(TokenizerResult::Waitlist) },
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
    static constexpr ASCIIChecker<true> AllowedBeforeEqual = "=!<>+-*/%?:"sv;
#define ENUM_PAIR(name, op) U"" name ""sv, static_cast<uint16_t>(op)
    static constexpr auto ResultParser = PARSE_PACK(
        ENUM_PAIR("==", EmbedOps::Equal),
        ENUM_PAIR("!=", EmbedOps::NotEqual),
        ENUM_PAIR("<",  EmbedOps::Less),
        ENUM_PAIR("<=", EmbedOps::LessEqual),
        ENUM_PAIR(">",  EmbedOps::Greater),
        ENUM_PAIR(">=", EmbedOps::GreaterEqual),
        ENUM_PAIR("&&", EmbedOps::And),
        ENUM_PAIR("||", EmbedOps::Or),
        ENUM_PAIR("+",  EmbedOps::Add),
        ENUM_PAIR("-",  EmbedOps::Sub),
        ENUM_PAIR("*",  EmbedOps::Mul),
        ENUM_PAIR("/",  EmbedOps::Div),
        ENUM_PAIR("%",  EmbedOps::Rem),
        ENUM_PAIR("??", EmbedOps::ValueOr),
        ENUM_PAIR("!",  EmbedOps::Not),
        ENUM_PAIR("?",  ExtraOps::Quest),
        ENUM_PAIR(":",  ExtraOps::Colon),
        ENUM_PAIR("=",  AssignOps::   Assign),
        ENUM_PAIR("+=", AssignOps::AddAssign),
        ENUM_PAIR("-=", AssignOps::SubAssign),
        ENUM_PAIR("*=", AssignOps::MulAssign),
        ENUM_PAIR("/=", AssignOps::DivAssign),
        ENUM_PAIR("%=", AssignOps::RemAssign),
        ENUM_PAIR("?=", AssignOps::NilAssign),
        ENUM_PAIR(":=", AssignOps::NewCreate)
        );
#undef ENUM_PAIR
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
            if (ch == U'=')
                return { ch, AllowedBeforeEqual(prevChar) ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
            else
            {
                switch (prevChar)
                {
                case U'&': return { ch, ch == U'&' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
                case U'|': return { ch, ch == U'|' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
                case U'?': return { ch, ch == U'?' ? TokenizerResult::Waitlist : TokenizerResult::NotMatch };
                default:   return { ch, TokenizerResult::NotMatch };
                }
            }
        default:
            return { ch, TokenizerResult::NotMatch };
        }
    }
    forceinline constexpr ParserToken GetToken(char32_t, ContextReader&, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() == 1 || txt.size() == 2);
        const auto ret = ResultParser(txt);
        if (ret.has_value())
            return ParserToken(NailangToken::OpSymbol, ret.value());
        else
            return ParserToken(BaseToken::Error, txt);
    }
};

//class AssignOpTokenizer
//{
//    static constexpr ASCIICheckerNBit<2> FirstChecker = []() -> ASCIICheckerNBit<2>
//    {
//        using common::enum_cast;
//        static_assert(enum_cast(TokenizerResult::Pending)  <= 0b11);
//        static_assert(enum_cast(TokenizerResult::Waitlist) <= 0b11);
//        static_assert(enum_cast(TokenizerResult::NotMatch) <= 0b11);
//        std::pair<char, uint8_t> mappings[] = 
//        {
//            { '=', enum_cast(TokenizerResult::Waitlist) },
//            { '&', enum_cast(TokenizerResult::Pending ) },
//            { '|', enum_cast(TokenizerResult::Pending ) },
//            { '+', enum_cast(TokenizerResult::Pending ) },
//            { '-', enum_cast(TokenizerResult::Pending ) },
//            { '*', enum_cast(TokenizerResult::Pending ) },
//            { '/', enum_cast(TokenizerResult::Pending ) },
//            { '%', enum_cast(TokenizerResult::Pending ) },
//            { '?', enum_cast(TokenizerResult::Pending ) },
//            { ':', enum_cast(TokenizerResult::Pending ) },
//        };
//        return { enum_cast(TokenizerResult::NotMatch), common::to_span(mappings) };
//    }();
//public:
//    using StateData = char32_t;
//    forceinline constexpr std::pair<char32_t, TokenizerResult> OnChar(const char32_t prevChar, const char32_t ch, const size_t idx) const noexcept
//    {
//        Expects((idx == 0) == (prevChar == 0));
//        switch (idx)
//        {
//        case 0: // just begin
//            return { ch, static_cast<TokenizerResult>(FirstChecker(ch, common::enum_cast(TokenizerResult::NotMatch))) };
//        case 1:
//            if (prevChar != U'=' && ch == U'=')
//                return { ch, TokenizerResult::Waitlist };
//        [[fallthrough]];
//        default:
//            return { ch, TokenizerResult::NotMatch };
//        }
//    }
//    forceinline constexpr ParserToken GetToken(char32_t, ContextReader&, std::u32string_view txt) const noexcept
//    {
//        Expects(txt.size() == 1 || txt.size() == 2);
//#define RET_OP(str, op) case str ## _hash: return ParserToken( NailangToken::Assign, common::enum_cast(AssignOps::op))
//        switch (common::DJBHash::HashC(txt))
//        {
//        RET_OP("=",     Assign);
//        RET_OP("&=", AndAssign);
//        RET_OP("|=",  OrAssign);
//        RET_OP("+=", AddAssign);
//        RET_OP("-=", SubAssign);
//        RET_OP("*=", MulAssign);
//        RET_OP("/=", DivAssign);
//        RET_OP("%=", RemAssign);
//        RET_OP("?=", NilAssign);
//        RET_OP(":=", NewCreate);
//        default:     return ParserToken(BaseToken::Error, txt);
//        }
//#undef RET_OP
//    }
//};


}

constexpr static auto IgnoreBlank = ASCIIChecker("\r\n\v\t ");


}
