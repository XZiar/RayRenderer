#pragma once

#include "ParserContext.hpp"
#include "ParserTokenizer.hpp"
#include "../CharConvs.hpp"


namespace common::parser
{


struct TokenResult
{
    ParserToken Token;
    char32_t Delim;
    constexpr TokenResult(ParserToken token, char32_t delim) 
        : Token(token), Delim(delim) { }
};

template<typename... TKs>
class ParserBase : public tokenizer::TokenizerBase
{
private:
    enum class MatchResults { NotBegin, FullMatch, Waitlist, Pending, NoMatch, Wrong };

    static constexpr auto TKCount = sizeof...(TKs);
    static constexpr auto Indexes = std::make_index_sequence<sizeof...(TKs)>{};
    using ResultArray = std::array<tokenizer::TokenizerResult, TKCount>;

    template<size_t N = 0>
    void InitTokenizer()
    {
        auto& tokenizer = std::get<N>(Tokenizers);
        tokenizer.OnInitialize();
        auto& result = Status[N];
        result = tokenizer::TokenizerResult::Pending;

        if constexpr (N + 1 < TKCount)
            return InitTokenizer<N + 1>();
    }

    template<size_t N = 0>
    MatchResults InvokeTokenizer(const char32_t ch, const size_t idx, size_t pendings = 0, size_t waitlist = 0) noexcept
    {
        using tokenizer::TokenizerResult;
        auto& result = Status[N];
        if (result == TokenizerResult::Pending || result == TokenizerResult::Waitlist)
        {
            auto& tokenizer = std::get<N>(Tokenizers);
            result = tokenizer.OnChar(ch, idx);
            switch (result)
            {
            case TokenizerResult::FullMatch:
                return MatchResults::FullMatch;
            case TokenizerResult::Waitlist:
                waitlist++; break;
            case TokenizerResult::Pending:
                pendings++; break;
            default:
                break;
            }
        }
        if constexpr (N + 1 < TKCount)
            return InvokeTokenizer<N + 1>(ch, idx, pendings, waitlist);
        else
        {
            if (waitlist > 0) return MatchResults::Waitlist;
            if (pendings > 0) return MatchResults::Pending;
            return MatchResults::NoMatch;
        }
    }

    template<size_t N = 0>
    ParserToken OutputToken(ParserContext& ctx, std::u32string_view tksv, const tokenizer::TokenizerResult target) noexcept
    {
        const auto result = Status[N];
        if (result == target)
        {
            auto& tokenizer = std::get<N>(Tokenizers);
            return tokenizer.GetToken(ctx, tksv);
        }
        if constexpr (N + 1 < TKCount)
            return OutputToken<N + 1>(ctx, tksv, target);
        else
            return GenerateToken(BaseToken::Error);
    }

public:
    ParserBase(ParserContext& ctx) : Context(ctx), Status({ tokenizer::TokenizerResult::Pending })
    { }
    constexpr void IgnoreBlank() noexcept
    {
        Context.TryGetWhile([](const char32_t ch)
            {
                return ch == U' ' || ch == U'\t';
            });
    }
    constexpr void IgnoreWhiteSpace() noexcept
    {
        Context.TryGetWhile([](const char32_t ch)
            {
                return ch == U' ' || ch == U'\t' || ch == '\r' || ch == '\n';
            });
    }
    TokenResult GetToken(const std::u32string_view delim, const std::u32string_view ignore = U" \t") noexcept
    {
        using tokenizer::TokenizerResult;
        constexpr char32_t EmptyChar = '\0';

        const auto checkIgnore = [=](const char32_t ch)
        {
            return ignore.find_first_of(ch) != std::u32string_view::npos;
        };

        Context.TryGetWhile(checkIgnore);
        InitTokenizer();

        size_t count = 0;
        MatchResults mth = MatchResults::NotBegin;
        const auto idxBegin = Context.Index;
        auto idxEnd = idxBegin;
        char32_t ch = '\0';

        while (true)
        {
            ch = Context.GetNext();
            if (ch == ParserContext::CharEnd)
            {
                idxEnd = Context.Index; break;
            }
            if (delim.find_first_of(ch) != std::u32string_view::npos)
            {
                idxEnd = Context.Index - 1; break;
            }
            if (ignore.find_first_of(ch) != std::u32string_view::npos)
            {
                idxEnd = Context.Index - 1;
                Context.TryGetWhile(checkIgnore);
                if (delim.find_first_of(Context.PeekNext()) == std::u32string_view::npos)
                    mth = MatchResults::Wrong;
                else
                    Context.GetNext();
                break;
            }
            
            mth = InvokeTokenizer(ch, count);
            
            if (mth == MatchResults::FullMatch)
            {
                const auto tokenTxt = Context.Source.substr(idxBegin, Context.Index - idxBegin);
                const auto token = OutputToken(Context, tokenTxt, TokenizerResult::FullMatch);
                
                Context.TryGetWhile(checkIgnore);
                if (delim.find_first_of(Context.PeekNext()) == std::u32string_view::npos)
                    return { token, EmptyChar };
                else
                    return { token, Context.GetNext() };
            }
            count++;
        }
        
        const auto tokenTxt = Context.Source.substr(idxBegin, idxEnd - idxBegin);

        switch (mth)
        {
        case MatchResults::Waitlist: // always be terminated by delim/end
            return { OutputToken(Context, tokenTxt, TokenizerResult::Waitlist), ch };
        case MatchResults::NotBegin:
            return { GenerateToken(BaseToken::End), EmptyChar };
        case MatchResults::FullMatch: // should not be here
        case MatchResults::Wrong:
            return { GenerateToken(BaseToken::Error, tokenTxt), ch };
        default:
            return { GenerateToken(BaseToken::Unknown, tokenTxt), ch };
        }
    }
private:
    ParserContext& Context;
    std::tuple<TKs...> Tokenizers;
    ResultArray Status;
};


}

