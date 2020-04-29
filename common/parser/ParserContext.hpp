#pragma once

#include "../CommonRely.hpp"

namespace common::parser
{


namespace special
{
inline constexpr char32_t EmptyChar = '\0';
inline constexpr char32_t CharLF    = '\n';
inline constexpr char32_t CharCR    = '\r';
inline constexpr char32_t CharEnd   = static_cast<char32_t>(-1);
}


class ParserContext
{
public:
    enum class CharType : uint8_t { End, NewLine, Digit, Blank, Special, Normal };
    [[nodiscard]] static constexpr CharType ParseType(const char32_t ch) noexcept
    {
        switch (ch)
        {
        case special::CharEnd:
            return CharType::End;
        case special::CharLF:
        case special::CharCR:
            return CharType::NewLine;
        case '\t':
        case ' ':
            return CharType::Blank;
        default:
            if (ch >= '0' && ch <= '9')
                return CharType::Digit;
            return CharType::Normal;
        }
    }

    std::u16string SourceName;
    std::u32string_view Source;
    size_t Index = 0;
    size_t Row = 0, Col = 0;

    ParserContext(const std::u32string_view source, const std::u16string_view name = u"") noexcept :
        SourceName(std::u16string(name)), Source(source)
    { }
};


class ContextReader
{
private:
    ParserContext& Context;
    size_t Index;

    forceinline constexpr char32_t HandleNextChar(size_t& index) noexcept
    {
        const auto ch = Context.Source[index++];
        switch (ch)
        {
        case special::CharCR:
            if (index < Context.Source.size() && Context.Source[index] == special::CharLF)
                index++;
        case special::CharLF:
            Context.Row++; Context.Col = 0;
            return special::CharLF;
        default:
            return ch;
        }
    }
public:
    constexpr ContextReader(ParserContext& context) : Context(context), Index(context.Index) { }

    [[nodiscard]] forceinline constexpr char32_t PeekNext() const noexcept
    {
        if (Index >= Context.Source.size())
            return special::CharEnd;
        const auto ch = Context.Source[Index];
        return (ch == special::CharCR || ch == special::CharLF) ? special::CharLF : ch;
    }
    forceinline constexpr void MoveNext() noexcept
    {
        const auto limit = Context.Source.size();
        if (Index >= limit) return;
        const auto ch = Context.Source[Index++];
        if (Index < limit && ch == special::CharCR && Context.Source[Index] == special::CharLF)
            Index++;
    }

    forceinline constexpr char32_t ReadNext() noexcept
    {
        if (Index >= Context.Source.size())
            return special::CharEnd;

        const auto ch = HandleNextChar(Index);
        if (ch != special::CharLF)
            Context.Col++;
        Context.Index = Index;
        return ch;
    }

    inline constexpr std::u32string_view CommitRead() noexcept
    {
        const auto limit = Index, start = Context.Index;
        auto lineIdx = Index = start;
        while (Index < limit)
        {
            if (HandleNextChar(Index) == special::CharLF)
                lineIdx = Index;
        }
        Context.Col += Index - lineIdx;
        Context.Index = Index;
        return Context.Source.substr(start, Index - start);
    }

    inline constexpr std::u32string_view ReadLine() noexcept
    {
        const auto start = Index = Context.Index;
        while (Index < Context.Source.size())
        {
            const auto cur = Index;
            if (HandleNextChar(Index) == special::CharLF)
            {
                Context.Index = Index;
                return Context.Source.substr(start, cur - start);
            }
        }
        Context.Col += Index - start;
        Context.Index = Index;
        return Context.Source.substr(start);
    }

    inline constexpr std::u32string_view ReadUntil(const std::u32string_view target) noexcept
    {
        if (target.size() == 0)
            return {};
        const auto pos = Context.Source.find(target, Index);
        if (pos == std::u32string_view::npos)
            return {};
        Index = pos + target.size();
        return CommitRead();
    }

    template<typename Pred>
    inline constexpr std::u32string_view ReadWhile(Pred&& predictor) noexcept
    {
        const auto start = Index = Context.Index;
        auto lineIdx = start;
        for (size_t count = 0; Index < Context.Source.size(); count++)
        {
            auto ch = Context.Source[Index];
            if (ch == special::CharCR)
                ch = special::CharLF;
            bool shouldContinue = false;
            if constexpr (std::is_invocable_r_v<bool, Pred, const char32_t>)
                shouldContinue = predictor(ch);
            else if constexpr (std::is_invocable_r_v<bool, Pred, const char32_t, size_t>)
                shouldContinue = predictor(ch, count);
            else
                static_assert(!common::AlwaysTrue<Pred>, "predictor should accept a char32_t and return if should continue");
            if (!shouldContinue)
                break;
            // should continue
            if (HandleNextChar(Index) == special::CharLF)
                lineIdx = Index;
        }
        Context.Col += Index - lineIdx;
        Context.Index = Index;
        return Context.Source.substr(start, Index - start);
    }
};


}

