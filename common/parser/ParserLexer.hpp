#pragma once

#include "ParserContext.hpp"
#include "ParserTokenizer.hpp"
#include "../CharConvs.hpp"



namespace common::parser
{

namespace detail
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


struct TokenizerData
{
    uint32_t State;
    std::array<tokenizer::TokenizerResult, 2> Result = { tokenizer::TokenizerResult::Pending, tokenizer::TokenizerResult::Pending };
    std::array<uint8_t, 2> Dummy = { 0, 0 };
};

}


template<typename... TKs>
class ParserLexerBase
{
    static_assert(detail::CheckRepeatTypes<TKs...>(), "tokenizer types should be unique");
private:
    enum class MatchResults { FullMatch, Waitlist, Pending, NoMatch };

    static constexpr auto TKCount = sizeof...(TKs);
    static constexpr auto Indexes = std::make_index_sequence<sizeof...(TKs)>{};
    using ResultArray = std::array<detail::TokenizerData, TKCount>;
    template<size_t N>
    using TKType = std::tuple_element_t<N, std::tuple<TKs...>>;

    template<typename TDst, typename TVal>
    forceinline static constexpr void SetTokenizer_([[maybe_unused]] TDst& dst, [[maybe_unused]] TVal&& val) noexcept
    {
        if constexpr (std::is_same_v<TDst, std::decay_t<TVal>>)
            dst = std::forward<TVal>(val);
    }
    template<typename T, size_t... I>
    forceinline static constexpr void SetTokenizer(std::tuple<TKs...>& tokenizers, T&& val, std::index_sequence<I...>) noexcept
    {
        (SetTokenizer_(std::get<I>(tokenizers), std::forward<T>(val)), ...);
    }
    template<typename... Args>
    [[nodiscard]] forceinline static constexpr std::tuple<TKs...> GenerateTokenizerList(Args&&... args) noexcept
    {
        std::tuple<TKs...> tokenizers;
        (SetTokenizer(tokenizers, std::forward<Args>(args), Indexes), ...);
        return tokenizers;
    }

    template<size_t N = 0>
    [[nodiscard]] forceinline constexpr MatchResults InvokeTokenizer(ResultArray& status, const char32_t ch, const size_t idx, size_t pendings = 0, size_t waitlist = 0) const noexcept
    {
        using tokenizer::TokenizerResult;
        auto& result = status[N].Result[idx & 1];
        const auto prev = status[N].Result[(idx & 1) ? 0 : 1];
        if (prev == TokenizerResult::Pending || prev == TokenizerResult::Waitlist)
        {
            auto& tokenizer = std::get<N>(Tokenizers);
            using StateData = typename TKType<N>::StateData;
            auto& state = status[N].State;
            const auto oldState = state;
            if constexpr (std::is_same_v<StateData, void>)
                result = tokenizer.OnChar(ch, idx);
            else
            {
                const auto ret = tokenizer.OnChar(static_cast<StateData>(status[N].State), ch, idx);
                state = static_cast<uint32_t>(ret.first), result = ret.second;
            }
            switch (result)
            {
            case TokenizerResult::FullMatch:
                return MatchResults::FullMatch;
            case TokenizerResult::Waitlist:
                waitlist++; break;
            case TokenizerResult::Pending:
                pendings++; break;
            default:
                state = oldState;
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
    [[nodiscard]] inline constexpr ParserToken OutputToken(const ResultArray& status, const size_t offset, ContextReader& reader, std::u32string_view tksv, const tokenizer::TokenizerResult target) const noexcept
    {
        const auto result = status[N].Result[offset];
        if (result == target)
        {
            auto& tokenizer = std::get<N>(Tokenizers);
            using StateData = typename TKType<N>::StateData;
            if constexpr (std::is_same_v<StateData, void>)
                return tokenizer.GetToken(reader, tksv);
            else
                return tokenizer.GetToken(static_cast<StateData>(status[N].State), reader, tksv);
        }
        if constexpr (N + 1 < TKCount)
            return OutputToken<N + 1>(status, offset, reader, tksv, target);
        else
            return ParserToken(BaseToken::Error);
    }

protected:
    template<typename Char>
    [[nodiscard]] forceinline constexpr static auto ToChecker(const std::basic_string_view<Char> str) noexcept
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
    constexpr ParserLexerBase(Args&&... args) : Tokenizers(GenerateTokenizerList(std::forward<Args>(args)...))
    { }

    template<typename Ignore>
    [[nodiscard]] forceinline constexpr ParserToken GetToken(ParserContext& context, Ignore&& ignore = std::string_view(" \t")) const noexcept
    {
        return GetTokenBy(context, ToChecker(ignore));
    }


    template<typename Ignore>
    [[nodiscard]] constexpr ParserToken GetTokenBy(ParserContext& context, Ignore&& isIgnore) const noexcept
    {
        static_assert(std::is_invocable_r_v<bool, Ignore, char32_t>);
        using tokenizer::TokenizerResult;

        ContextReader reader(context);
        reader.ReadWhile(isIgnore);

        ResultArray status = { {} };
        status.fill({});
        size_t count = 0;
        MatchResults prev = MatchResults::NoMatch, mth = prev;
        char32_t ch = '\0';

        while (true)
        {
            ch = reader.PeekNext();
            if (count == 0)
                Expects(!isIgnore(ch));
            if (ch == special::CharEnd || isIgnore(ch))// force stop checking
                break;

            prev = mth;
            mth = InvokeTokenizer(status, ch, count);

            if (prev != MatchResults::NoMatch && mth == MatchResults::NoMatch) // max matched
                break;

            reader.MoveNext();
            count++;

            if (mth == MatchResults::FullMatch || mth == MatchResults::NoMatch)
                break;
        }

        const auto tokenTxt = reader.CommitRead();

        const size_t offset = (count & 1) ? 0 : 1;

        switch (mth == MatchResults::NoMatch ? prev : mth)
        {
        case MatchResults::FullMatch:
            return OutputToken(status, offset, reader, tokenTxt, TokenizerResult::FullMatch);
        case MatchResults::Waitlist:
            return OutputToken(status, offset, reader, tokenTxt, TokenizerResult::Waitlist);
        default:
            if (count == 0 && ch == special::CharEnd)
                return ParserToken(BaseToken::End);
            else
                return ParserToken(BaseToken::Unknown, tokenTxt);
        }
    }


private:
    std::tuple<TKs...> Tokenizers;
};


}

