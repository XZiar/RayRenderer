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


#if COMMON_OS_WIN && _MSC_VER <= 1914
#   pragma message("ASCIIChecker may not be supported due to lack of constexpr support before VS 15.7")
#endif

template<bool Result = true>
struct ASCIIChecker
{
    std::array<uint32_t, 4> LUT = { 0 };
    constexpr ASCIIChecker(const std::string_view str)
    {
        for (auto& ele : LUT)
            ele = Result ? 0x0 : 0xffffffff;
        for (const auto ch : str)
        {
            if (ch >= 0 && ch < 128)
            {
                auto& ele = LUT[ch / 32];
                const uint32_t obj = 0x1u << (ch % 32);
                if constexpr (Result)
                    ele |= obj;
                else
                    ele &= (~obj);
            }
        }
    }

    constexpr bool operator()(const char32_t ch) const noexcept
    {
        if (ch >= 0 && ch < 128)
        {
            const auto ele = LUT[ch / 32];
            const auto ret = ele >> (ch % 32);
            return ret & 0x1 ? true : false;
        }
        else
            return !Result;
    }
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

protected:
    template<typename Char>
    forceinline static auto ToChecker(const std::basic_string_view<Char> str) noexcept
    {
        if constexpr (std::is_same_v<Char, char>)
        {
            return ASCIIChecker(str);
        }
        else if constexpr (std::is_same_v<Char, char32_t>)
        {
            return [=](const char32_t ch) { return str.find_first_of(ch) != std::u32string_view::npos; };
        }
        else
        {
            static_assert(!AlwaysTrue<Char>, "only accept char and char32_t");
        }
    }
public:
    ParserBase(ParserContext& ctx) : Context(ctx), Status({ tokenizer::TokenizerResult::Pending })
    { }

    template<typename Delim, typename Ignore>
    forceinline TokenResult GetToken(Delim&& delim, Ignore&& ignore = std::string_view(" \t")) noexcept
    {
        return GetTokenBy(ToChecker(delim), ToChecker(ignore));
    }

    template<typename Delim, typename Ignore>
    TokenResult GetTokenBy(Delim&& isDelim, Ignore&& isIgnore) noexcept
    {
        static_assert(std::is_invocable_r_v<bool, Delim, char32_t>);
        static_assert(std::is_invocable_r_v<bool, Ignore, char32_t>);
        using tokenizer::TokenizerResult;
        constexpr char32_t EmptyChar = '\0';

        Context.TryGetWhile(isIgnore);
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
            if (isDelim(ch))
            {
                idxEnd = Context.Index - 1; break;
            }
            if (isIgnore(ch))
            {
                idxEnd = Context.Index - 1;
                Context.TryGetWhile(isIgnore);
                if (auto ch_ = Context.TryGetNext(); !isDelim(ch_))
                    mth = MatchResults::Wrong;
                else
                    ch_.Accept();
                break;
            }
            
            mth = InvokeTokenizer(ch, count);
            
            if (mth == MatchResults::FullMatch)
            {
                const auto tokenTxt = Context.Source.substr(idxBegin, Context.Index - idxBegin);
                const auto token = OutputToken(Context, tokenTxt, TokenizerResult::FullMatch);
                
                Context.TryGetWhile(isIgnore);
                if (auto ch_ = Context.TryGetNext(); !isDelim(ch_))
                    return { token, EmptyChar };
                else
                    return { token, ch_.Accept() };
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

