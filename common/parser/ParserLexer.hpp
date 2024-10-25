#pragma once

#include "ParserContext.hpp"
#include "ParserTokenizer.hpp"



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


template<typename... Ts>
inline constexpr auto GenerateStateNeedness() noexcept
{
    std::array<bool, sizeof...(Ts)> ret = { false };
    size_t i = 0;
    (..., void(ret[i++] = !std::is_same_v<typename Ts::StateData, void>));
    return ret;
}
template<size_t N>
inline constexpr auto GenerateStateIndexes(const std::array<bool, N>& needness) noexcept
{
    std::array<uint32_t, N> dst = { 0 };
    uint32_t idx = 0;
    for (size_t i = 0; i < N; ++i)
    {
        dst[i] = idx;
        if (needness[i])
            idx++;
    }
    return dst;
}
template<size_t N>
inline constexpr size_t GenerateStateCount(const std::array<bool, N>& needness) noexcept
{
    size_t count = 0;
    for (size_t i = 0; i < N; ++i)
    {
        count += needness[i];
    }
    return count;
}

template<size_t TKCount, size_t StateCount>
struct TokenizerTemp
{
    std::array<uint32_t, StateCount> States;
    std::array<tokenizer::TokenizerResult, TKCount * 2> Results;
    constexpr TokenizerTemp() noexcept
    {
        for (size_t i = 0; i < StateCount; ++i)
            States[i] = 0;
        for (size_t i = 0; i < TKCount * 2; ++i)
            Results[i] = tokenizer::TokenizerResult::Pending;
    }
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
    template<size_t N>
    using TKType = std::tuple_element_t<N, std::tuple<TKs...>>;

    static constexpr auto StateNeedness = detail::GenerateStateNeedness<TKs...>();
    static constexpr auto StateIndexes  = detail::GenerateStateIndexes(StateNeedness);
    static constexpr auto StateCount    = detail::GenerateStateCount(StateNeedness);
    using TKTempData = detail::TokenizerTemp<TKCount, StateCount>;

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
        std::tuple<TKs...> tokenizers = {};
        (SetTokenizer(tokenizers, std::forward<Args>(args), Indexes), ...);
        return tokenizers;
    }

    template<size_t N>
    forceinline constexpr void InvokeTokenizer(TKTempData& temps, const char32_t ch, const size_t idx) const noexcept
    {
        using tokenizer::TokenizerResult;
        auto& result = temps.Results[N * 2 + (idx & 1)];
        const auto prev = temps.Results[N * 2 + ((idx & 1) ? 0 : 1)];
        if (prev == TokenizerResult::Pending || prev == TokenizerResult::Waitlist)
        {
            auto& tokenizer = std::get<N>(Tokenizers);
            if constexpr (!StateNeedness[N])
                result = tokenizer.OnChar(ch, idx);
            else
            {
                using StateData = typename TKType<N>::StateData;
                auto& state = temps.States[StateIndexes[N]];
                const auto ret = tokenizer.OnChar(static_cast<StateData>(state), ch, idx);
                result = ret.second;
                if (result != TokenizerResult::NotMatch && result != TokenizerResult::Wrong)
                    state = static_cast<uint32_t>(ret.first);
            }
        }
        else
            result = TokenizerResult::NotMatch;
    }
    template<size_t... I>
    [[nodiscard]] forceinline constexpr MatchResults InvokeTokenizers(TKTempData& temps, const char32_t ch, const size_t idx, std::index_sequence<I...>) const noexcept
    {
        using tokenizer::TokenizerResult;
        (..., InvokeTokenizer<I>(temps, ch, idx));
        uint32_t pendings = 0, waitlist = 0;
        for (size_t i = 0; i < TKCount; ++i)
        {
            const auto result = temps.Results[i * 2 + (idx & 1)];
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
        if (waitlist > 0) return MatchResults::Waitlist;
        if (pendings > 0) return MatchResults::Pending;
        return MatchResults::NoMatch;
    }

    template<size_t N = 0>
    [[nodiscard]] inline constexpr ParserToken OutputToken(const TKTempData& temps, const size_t offset, ContextReader& reader, std::u32string_view tksv, const tokenizer::TokenizerResult target) const noexcept
    {
        const auto result = temps.Results[N * 2 + offset];
        if (result == target)
        {
            auto& tokenizer = std::get<N>(Tokenizers);
            using StateData = typename TKType<N>::StateData;
            if constexpr (std::is_same_v<StateData, void>)
                return tokenizer.GetToken(reader, tksv);
            else
                return tokenizer.GetToken(static_cast<StateData>(temps.States[StateIndexes[N]]), reader, tksv);
        }
        if constexpr (N + 1 < TKCount)
            return OutputToken<N + 1>(temps, offset, reader, tksv, target);
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
    constexpr ParserLexerBase(Args&&... args) : Tokenizers(std::move(GenerateTokenizerList(std::forward<Args>(args)...)))
    { }

    template<typename Ignore>
    [[nodiscard]] forceinline constexpr DetailToken GetToken(ParserContext& context, Ignore&& ignore = std::string_view(" \t")) const noexcept
    {
        return GetTokenBy(context, ToChecker(ignore));
    }


    template<typename Ignore>
    [[nodiscard]] constexpr DetailToken GetTokenBy(ParserContext& context, Ignore&& isIgnore) const noexcept
    {
        static_assert(std::is_invocable_r_v<bool, Ignore, char32_t>);
        using tokenizer::TokenizerResult;

        ContextReader reader(context);
        reader.ReadWhile(isIgnore);

        const auto row = context.Row, col = context.Col;
        TKTempData data;
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
            mth = InvokeTokenizers(data, ch, count, Indexes);

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
            return { row, col, OutputToken(data, offset, reader, tokenTxt, TokenizerResult::FullMatch) };
        case MatchResults::Waitlist:
            return { row, col, OutputToken(data, offset, reader, tokenTxt, TokenizerResult::Waitlist) };
        default:
            if (count == 0 && ch == special::CharEnd)
                return { row, col, { BaseToken::End, ch } };
            else
                return { row, col, { BaseToken::Unknown, tokenTxt } };
        }
    }


private:
    std::tuple<TKs...> Tokenizers;
};


}

