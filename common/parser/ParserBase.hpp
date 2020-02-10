#pragma once

#include "ParserContext.hpp"
#include "ParserTokenizer.hpp"
#include "../CharConvs.hpp"



namespace common::parser
{

template<typename... Ts>
struct RepeatTypeChecker;
template<typename T1, typename T2, typename... Ts>
struct RepeatTypeChecker<T1, T2, Ts...>
{
    static constexpr bool IsUnique = RepeatTypeChecker<T1, T2>::IsUnique && 
        RepeatTypeChecker<T1, Ts...>::IsUnique && RepeatTypeChecker<T2, Ts...>::IsUnique;
};
template<typename T1, typename T2>
struct RepeatTypeChecker<T1, T2>
{
    static constexpr bool IsUnique = !std::is_same_v<T1, T2>;
};
template<typename... Ts>
inline constexpr bool CheckRepeatTypes() noexcept
{
    if constexpr (sizeof...(Ts) == 1)
        return true;
    else
        return RepeatTypeChecker<Ts...>::IsUnique;
}


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
    static_assert(CheckRepeatTypes<TKs...>(), "tokenizer types should be unique");
private:
    enum class MatchResults { NotBegin, FullMatch, Waitlist, Pending, NoMatch, Wrong };

    static constexpr auto TKCount = sizeof...(TKs);
    static constexpr auto Indexes = std::make_index_sequence<sizeof...(TKs)>{};
    using ResultArray = std::array<tokenizer::TokenizerResult, TKCount>;

    template<size_t N, typename T>
    forceinline constexpr void SetTokenizer_(T&& val) noexcept
    {
        auto& tokenizer = std::get<N>(Tokenizers);
        if constexpr (std::is_same_v<std::decay_t<decltype(tokenizer)>, std::decay_t<T>>)
            tokenizer = val;
    }
    template<typename T, size_t... I>
    forceinline constexpr void SetTokenizer(T&& val, std::index_sequence<I...>) noexcept
    {
        (SetTokenizer_<I>(std::forward<T>(val)), ...);
    }
    template<typename... Args, size_t... I>
    forceinline constexpr void SetTokenizers(std::index_sequence<I...>, Args&&... args) noexcept
    {
        (SetTokenizer(std::forward<Args>(args), Indexes), ...);
    }

    template<size_t N = 0>
    forceinline constexpr void InitTokenizer()
    {
        auto& tokenizer = std::get<N>(Tokenizers);
        tokenizer.OnInitialize();
        auto& result = Status[N];
        result = tokenizer::TokenizerResult::Pending;

        if constexpr (N + 1 < TKCount)
            return InitTokenizer<N + 1>();
    }

    template<size_t N = 0>
    forceinline constexpr MatchResults InvokeTokenizer(const char32_t ch, const size_t idx, size_t pendings = 0, size_t waitlist = 0) noexcept
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
    inline constexpr ParserToken OutputToken(ParserContext& ctx, std::u32string_view tksv, const tokenizer::TokenizerResult target) noexcept
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
    forceinline constexpr static auto ToChecker(const std::basic_string_view<Char> str) noexcept
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
    template<typename... Args>
    constexpr ParserBase(Args&&... args) 
        : Status({ tokenizer::TokenizerResult::Pending })
    { 
        SetTokenizers(std::make_index_sequence<sizeof...(Args)>{}, std::forward<Args>(args)...);
    }

    template<typename Delim, typename Ignore>
    forceinline constexpr TokenResult GetDelimToken(ParserContext& context, Delim&& delim, Ignore&& ignore = std::string_view(" \t")) noexcept
    {
        return GetDelimTokenBy(context, ToChecker(delim), ToChecker(ignore));
    }

    template<typename Delim, typename Ignore>
    constexpr TokenResult GetDelimTokenBy(ParserContext& context, Delim&& isDelim, Ignore&& isIgnore) noexcept
    {
        static_assert(std::is_invocable_r_v<bool, Delim, char32_t>);
        static_assert(std::is_invocable_r_v<bool, Ignore, char32_t>);
        using tokenizer::TokenizerResult;
        constexpr char32_t EmptyChar = '\0';

        context.TryGetWhile(isIgnore);
        InitTokenizer();

        size_t count = 0;
        MatchResults mth = MatchResults::NotBegin;
        const auto idxBegin = context.Index;
        auto idxEnd = idxBegin;
        char32_t ch = '\0';

        while (true)
        {
            ch = context.GetNext();
            if (ch == ParserContext::CharEnd)
            {
                idxEnd = context.Index; break;
            }
            if (isDelim(ch))
            {
                idxEnd = context.Index - 1; break;
            }
            if (isIgnore(ch))
            {
                idxEnd = context.Index - 1;
                context.TryGetWhile(isIgnore);
                if (auto ch_ = context.TryGetNext(); !isDelim(ch_))
                    mth = MatchResults::Wrong;
                else
                    ch_.Accept();
                break;
            }
            
            mth = InvokeTokenizer(ch, count);
            
            if (mth == MatchResults::FullMatch)
            {
                const auto tokenTxt = context.Source.substr(idxBegin, context.Index - idxBegin);
                const auto token = OutputToken(context, tokenTxt, TokenizerResult::FullMatch);
                
                context.TryGetWhile(isIgnore);
                if (auto ch_ = context.TryGetNext(); !isDelim(ch_))
                    return { token, EmptyChar };
                else
                    return { token, ch_.Accept() };
            }
            count++;
        }
        
        const auto tokenTxt = context.Source.substr(idxBegin, idxEnd - idxBegin);

        switch (mth)
        {
        case MatchResults::Waitlist: // always be terminated by delim/end
            return { OutputToken(context, tokenTxt, TokenizerResult::Waitlist), ch };
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
    std::tuple<TKs...> Tokenizers;
    ResultArray Status;
};


}

