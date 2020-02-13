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
private:
    class CharStub
    {
    private:
        ParserContext& Context;
        char32_t Char;
        bool HasAccept = false;
    public:
        forceinline constexpr CharStub(ParserContext& context, char32_t ch) :
            Context(context), Char(ch) { }
        forceinline constexpr char32_t Accept() noexcept
        {
            if (!HasAccept)
            {
                HasAccept = true;
                if (Char != special::CharEnd)
                {
                    Context.Index++;
                    Context.HandlePosition(Char);
                }
            }
            return Char;
        }
        forceinline constexpr operator char32_t() const noexcept
        {
            return Char;
        }
    };
public:
    enum class CharType : uint8_t { End, NewLine, Digit, Blank, Special, Normal };
    static constexpr CharType ParseType(const char32_t ch) noexcept
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

    forceinline constexpr char32_t GetNext() noexcept
    {
        if (Index >= Source.size())
            return special::CharEnd;
        const auto ch = Source[Index++];
        if (HandlePosition(ch))
            return special::CharLF;
        return ch;
    }

    forceinline constexpr char32_t PeekNext() const noexcept
    {
        if (Index >= Source.size())
            return special::CharEnd;
        const auto ch = Source[Index];
        return (ch == special::CharCR || ch == special::CharLF) ? special::CharLF : ch;
    }

    forceinline constexpr CharStub TryGetNext() noexcept
    {
        return { *this, PeekNext() };
    }

    forceinline constexpr std::u32string_view GetLine() noexcept
    {
        const auto start = Index;
        while (Index < Source.size())
        {
            const auto ch = Source[Index++];
            if (ch == special::CharCR || ch == special::CharLF)
            {
                const auto txt = Source.substr(start, Index - start - 1);
                HandleNewLine(ch);
                return txt;
            }
        }
        Col += Index - start;
        return Source.substr(start);
    }

    inline constexpr std::u32string_view TryGetUntil(const std::u32string_view target, const bool allowMultiLine = true)
    {
        if (target.size() == 0)
            return {};
        const auto pos = Source.find(target, Index);
        return GetUntil(target.size(), pos, allowMultiLine);
    }

    template<bool AllowNewLine = true, typename Pred>
    constexpr std::u32string_view TryGetWhile(Pred&& predictor)
    {
        const auto start = Index;
        auto lineIdx = Index;
        while (Index < Source.size())
        {
            auto ch = Source[Index];
            if (ch == special::CharCR)
                ch = special::CharLF;
            if constexpr (!AllowNewLine)
            {
                if (ch == special::CharLF)
                {
                    Index = start;
                    return {};
                }
            }
            bool shouldContinue = false;
            if constexpr (std::is_invocable_r_v<bool, Pred, const char32_t>)
                shouldContinue = predictor(ch);
            else if constexpr (std::is_invocable_r_v<bool, Pred, const char32_t, size_t>)
                shouldContinue = predictor(ch, Index);
            else
                static_assert(!common::AlwaysTrue<Pred>(), "predictor should accept a char32_t and return if should continue");
            if (!shouldContinue)
                break;
            // should continue
            Index++;
            if (ch == special::CharLF)
            {
                HandleNewLine(ch);
                lineIdx = Index;
            } 
        }
        Col += Index - lineIdx;
        return Source.substr(start, Index - start);
    }
private:
    constexpr std::u32string_view GetUntil(const size_t targetSize, const size_t pos, const bool allowMultiLine = true)
    {
        if (pos == std::u32string_view::npos)
            return {};
        const auto start = Index, end = pos + targetSize;
        auto lineIdx = Index;
        while (Index < end)
        {
            const auto ch = Source[Index++];
            if (ch == special::CharCR || ch == special::CharLF)
            {
                if (!allowMultiLine)
                {
                    Index = start;
                    return {};
                }
                HandleNewLine(ch);
                lineIdx = Index;
            }
        }
        Col += Index - lineIdx;
        return Source.substr(start, end - start);
    }
    forceinline constexpr void HandleNewLine(const char32_t ch) noexcept
    {
        if (ch == special::CharCR)
        {
            if (Index < Source.size() && Source[Index] == special::CharLF)
                Index++;
        }
        Row++; Col = 0;
    }
    forceinline constexpr bool HandlePosition(const char32_t ch) noexcept
    {
        switch (ch)
        {
        case special::CharCR:
        case special::CharLF:
            HandleNewLine(ch);
            return true;
        default:
            Col++;
            return false;
        }
    }
};


struct ContextReader
{
    ParserContext& Context;
    size_t Index;
    constexpr ContextReader(ParserContext& context) : Context(context), Index(context.Index) { }

    forceinline constexpr char32_t PeekNext() const noexcept
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
    forceinline constexpr std::u32string_view GetReadContent() const noexcept
    {
        return Context.Source.substr(Context.Index, Index - Context.Index);
    }
    forceinline constexpr void Commit() const noexcept
    {
        const auto limit = Context.Source.size();
        auto cur = Context.Index, lineIdx = cur;
        while (cur < Index)
        {
            const auto ch = Context.Source[cur++];
            switch (ch)
            {
            case special::CharCR:
                if (cur < limit && Context.Source[cur] == special::CharLF)
                    cur++;
            case special::CharLF:
                Context.Row++; Context.Col = 0;
                lineIdx = cur;
            default:
                break;
            }

        }
        Context.Col += cur - lineIdx;
        Context.Index = cur;
    }
};


}

