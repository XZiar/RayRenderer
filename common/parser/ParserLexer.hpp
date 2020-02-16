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
class ParserLexerBase : public tokenizer::TokenizerBase
{
    static_assert(CheckRepeatTypes<TKs...>(), "tokenizer types should be unique");
private:
    enum class MatchResults { NotBegin, Preempt, FullMatch, Waitlist, Pending, NoMatch, Wrong };

    static constexpr auto TKCount = sizeof...(TKs);
    static constexpr auto Indexes = std::make_index_sequence<sizeof...(TKs)>{};
    using ResultArray = std::array<std::array<tokenizer::TokenizerResult, 2>, TKCount>;
    template<size_t N>
    using TKType = std::tuple_element_t<N, std::tuple<TKs...>>;

    template<size_t N, typename T>
    forceinline constexpr void SetTokenizer_([[maybe_unused]] T&& val) noexcept
    {
        if constexpr (std::is_same_v<TKType<N>, std::decay_t<T>>)
            std::get<N>(Tokenizers) = val;
    }
    template<typename T, size_t... I>
    forceinline constexpr void SetTokenizer(T&& val, std::index_sequence<I...>) noexcept
    {
        (SetTokenizer_<I>(std::forward<T>(val)), ...);
    }

    template<size_t N = 0>
    forceinline constexpr void InitTokenizer() noexcept
    {
        auto& tokenizer = std::get<N>(Tokenizers);
        tokenizer.OnInitialize();

        if constexpr (N + 1 < TKCount)
            return InitTokenizer<N + 1>();
    }

    template<size_t N = 0>
    forceinline constexpr MatchResults InvokeTokenizer(ResultArray &status, const char32_t ch, const size_t idx, size_t pendings = 0, size_t waitlist = 0) noexcept
    {
        using tokenizer::TokenizerResult;
        auto& result = status[N][idx & 1];
        if (const auto prev = status[N][(idx & 1) ? 0 : 1]; prev == TokenizerResult::Pending || prev == TokenizerResult::Waitlist)
        {
            auto& tokenizer = std::get<N>(Tokenizers);
            result = tokenizer.OnChar(ch, idx);
            switch (result)
            {
            case TokenizerResult::FullMatch:
                return MatchResults::FullMatch;
            case TokenizerResult::Preempt:
                return MatchResults::Preempt;
            case TokenizerResult::Waitlist:
                waitlist++; break;
            case TokenizerResult::Pending:
                pendings++; break;
            default:
                break;
            }
        }
        else
            result = TokenizerResult::NotMatch;
        if constexpr (N + 1 < TKCount)
            return InvokeTokenizer<N + 1>(status, ch, idx, pendings, waitlist);
        else
        {
            if (waitlist > 0) return MatchResults::Waitlist;
            if (pendings > 0) return MatchResults::Pending;
            return MatchResults::NoMatch;
        }
    }

    template<size_t N = 0>
    inline constexpr ParserToken OutputToken(const ResultArray& status, const size_t offset, ContextReader& reader, std::u32string_view tksv, const tokenizer::TokenizerResult target) noexcept
    {
        const auto result = status[N][offset];
        if (result == target)
        {
            auto& tokenizer = std::get<N>(Tokenizers);
            return tokenizer.GetToken(reader, tksv);
        }
        if constexpr (N + 1 < TKCount)
            return OutputToken<N + 1>(status, offset, reader, tksv, target);
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
    constexpr ParserLexerBase(Args&&... args)
    { 
        (SetTokenizer(std::forward<Args>(args), Indexes), ...);
    }

    template<typename Ignore>
    forceinline constexpr ParserToken GetToken(ParserContext& context, Ignore&& ignore = std::string_view(" \t")) noexcept
    {
        return GetTokenBy(context, ToChecker(ignore));
    }


    template<typename Ignore>
    constexpr ParserToken GetTokenBy(ParserContext& context, Ignore&& isIgnore) noexcept
    {
        static_assert(std::is_invocable_r_v<bool, Ignore, char32_t>);
        using tokenizer::TokenizerResult;

        ContextReader reader(context);
        reader.ReadWhile(isIgnore);

        ResultArray status = { {} };
        status.fill({ {TokenizerResult::Pending, TokenizerResult::Pending} });
        size_t count = 0;
        MatchResults mth = MatchResults::NotBegin;
        char32_t ch = '\0';

        InitTokenizer();

        while (true)
        {
            ch = reader.PeekNext();
            if (ch == special::CharEnd || isIgnore(ch)) // force stop checking
                break;

            mth = InvokeTokenizer(status, ch, count);

            if (mth == MatchResults::Preempt)
                break;

            reader.MoveNext();
            count++;

            if (mth == MatchResults::FullMatch)
                break;
        }

        const auto tokenTxt = reader.CommitRead();

        const size_t offset = (count & 1) ? 0 : 1;
        switch (mth)
        {
        case MatchResults::FullMatch:
            return OutputToken(status, offset, reader, tokenTxt, TokenizerResult::FullMatch);
        case MatchResults::Waitlist:
        case MatchResults::Preempt:
            return OutputToken(status, offset, reader, tokenTxt, TokenizerResult::Waitlist);
        case MatchResults::NotBegin:
            return GenerateToken(BaseToken::End);
        case MatchResults::Wrong:
            return GenerateToken(BaseToken::Error, tokenTxt);
        default:
            return GenerateToken(BaseToken::Unknown, tokenTxt);
        }
    }

    
private:
    std::tuple<TKs...> Tokenizers;
};


}

