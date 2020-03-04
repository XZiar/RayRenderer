#pragma once

#include "ParserContext.hpp"
#include "ParserTokenizer.hpp"
#include "ParserLexer.hpp"
#include "../EnumEx.hpp"


namespace common::parser
{


namespace detail
{

template<size_t IDCount, size_t TKCount>
struct TokenMatcher
{
    std::array<uint16_t, IDCount> IDs;
    std::array<ParserToken, TKCount> TKs;
    constexpr bool Match(const ParserToken token) const noexcept
    {
        for (const auto id : IDs)
            if (token.GetID() == id)
                return true;
        for (const auto& tk : TKs)
            if (token == tk)
                return true;
        return false;
    }
};
template<size_t IDCount>
struct TokenMatcher<IDCount, 0>
{
    std::array<uint16_t, IDCount> IDs;
    constexpr bool Match(const ParserToken token) const noexcept
    {
        for (const auto id : IDs)
            if (token.GetID() == id)
                return true;
        return false;
    }
};
template<size_t TKCount>
struct TokenMatcher<0, TKCount>
{
    std::array<ParserToken, TKCount> TKs;
    constexpr bool Match(const ParserToken token) const noexcept
    {
        for (const auto& tk : TKs)
            if (token == tk)
                return true;
        return false;
    }
};
template<>
struct TokenMatcher<0, 0>
{
    constexpr bool Match(const ParserToken) const noexcept
    {
        return false;
    }
};
struct EmptyTokenArray {};

struct TokenMatcherHelper
{
    template<size_t I, size_t N, typename T, typename... Args>
    forceinline static constexpr std::array<uint16_t, N> GenerateIDArray(std::array<uint16_t, N> ids, const T id, const Args... args) noexcept
    {
        ids[I] = common::enum_cast(id);
        if constexpr (I + 1 < N)
            return GenerateIDArray<I + 1>(ids, args...);
        else
            return ids;
    }
    template<typename... IDs>
    static constexpr auto GetMatcher(EmptyTokenArray, IDs... ids) noexcept
    {
        constexpr auto IDCount = sizeof...(IDs);
        if constexpr (IDCount > 0)
            return TokenMatcher<IDCount, 0>{ GenerateIDArray<0>(std::array<uint16_t, IDCount>{0}, ids...) };
        else
            return TokenMatcher<0, 0>{};
    }

    template<size_t TKCount, typename... IDs>
    static constexpr auto GetMatcher(std::array<ParserToken, TKCount> tokens, IDs... ids) noexcept
    {
        constexpr auto IDCount = sizeof...(IDs);
        if constexpr (IDCount > 0)
        {
            constexpr auto idarray = GenerateIDArray<0>(std::array<uint16_t, IDCount>{0}, ids...);
            if constexpr (TKCount > 0)
                return TokenMatcher<IDCount, TKCount>{ idarray, tokens };
            else
                return TokenMatcher<IDCount, 0>{ idarray };
        }
        else
        {
            if constexpr (TKCount == 0)
                return TokenMatcher<0, 0>{};
            else
                return TokenMatcher<0, TKCount>{tokens};
        }
    }
};


struct ParsingError : std::exception
{
    std::pair<size_t, size_t> Position;
    ParserToken Token;
    std::u16string Notice;
    ParsingError(const ParserContext& context, const ParserToken token, std::u16string notice) :
        Position({ context.Row, context.Col }), Token(token), Notice(std::move(notice)) { }
};

}


class ParserBase
{
private:
    
protected:
    template<size_t IDCount, size_t TKCount>
    using TokenMatcher = detail::TokenMatcher<IDCount, TKCount>;

    ParserContext& Context;

    constexpr ParserBase(ParserContext& context) : Context(context) 
    { }

    virtual std::u16string DescribeTokenID(const uint16_t tid) const noexcept
    {
#define RET_TK_ID(type) case BaseToken::type:        return u ## #type
        switch (static_cast<BaseToken>(tid))
        {
        RET_TK_ID(End);
        RET_TK_ID(Error);
        RET_TK_ID(Unknown);
        RET_TK_ID(Delim);
        RET_TK_ID(Comment);
        RET_TK_ID(Raw);
        RET_TK_ID(Bool);
        RET_TK_ID(Uint);
        RET_TK_ID(Int);
        RET_TK_ID(FP);
        RET_TK_ID(String);
        default:
        RET_TK_ID(Custom);
        }
#undef RET_TK_ID
    }
    virtual std::u16string DescribeToken(const ParserToken& token) const noexcept
    {
        return DescribeTokenID(token.GetID());
    }

    virtual ParserToken OnUnExpectedToken(const ParserToken& token) const
    {
        throw detail::ParsingError(Context, token, u"Unexpected token [" + DescribeToken(token) + u"]");
    }
    
    static inline constexpr auto IgnoreCommentToken = detail::TokenMatcherHelper::GetMatcher
        (detail::EmptyTokenArray{}, BaseToken::Comment);
    static inline constexpr TokenMatcher<0, 0> IgnoreNoneToken = {};

    template<typename Lex, typename Ignore>
    constexpr ParserToken GetNextToken(Lex&& lexer, Ignore&& ignore)
    {
        return lexer.GetTokenBy(Context, ignore);
    }
    template<typename Lex, typename Ignore, size_t IDCount, size_t TKCount>
    constexpr ParserToken GetNextToken(Lex&& lexer, Ignore&& ignore, const TokenMatcher<IDCount, TKCount>& ignoreMatcher)
    {
        while (true)
        {
            const auto token = lexer.GetTokenBy(Context, ignore);
            if (!ignoreMatcher.Match(token))
                return token;
        }
    }
    template<typename Lex, typename Ignore, size_t IDCount1, size_t TKCount1, size_t IDCount2, size_t TKCount2>
    ParserToken ExpectNextToken(Lex&& lexer, Ignore&& ignore, 
        const TokenMatcher<IDCount1, TKCount1>& ignoreMatcher, const TokenMatcher<IDCount2, TKCount2>& expectMatcher)
    {
        const auto token = GetNextToken(lexer, ignore, ignoreMatcher);
        if (expectMatcher.Match(token))
            return token;
        else
            return OnUnExpectedToken(token);
    }
public:

};


}

