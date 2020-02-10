#pragma once

#include "../CommonRely.hpp"

namespace common::parser
{


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
                if (Char != CharEnd)
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
    static constexpr char32_t CharLF = '\n';
    static constexpr char32_t CharCR = '\r';
    static constexpr char32_t CharEnd = static_cast<char32_t>(-1);
    enum class CharType : uint8_t { End, NewLine, Digit, Blank, Special, Normal };
    static constexpr CharType ParseType(const char32_t ch) noexcept
    {
        switch (ch)
        {
        case CharEnd:
            return CharType::End;
        case CharLF:
        case CharCR:
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
            return CharEnd;
        const auto ch = Source[Index++];
        if (HandlePosition(ch))
            return CharLF;
        return ch;
    }

    forceinline constexpr char32_t PeekNext() const noexcept
    {
        if (Index >= Source.size())
            return CharEnd;
        const auto ch = Source[Index];
        return (ch == CharCR || ch == CharLF) ? CharLF : ch;
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
            if (ch == CharCR || ch == CharLF)
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
            if (ch == CharCR)
                ch = CharLF;
            if constexpr (!AllowNewLine)
            {
                if (ch == CharLF)
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
            if (ch == CharLF)
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
            if (ch == CharCR || ch == CharLF)
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
        if (ch == CharCR)
        {
            if (Index < Source.size() && Source[Index] == CharLF)
                Index++;
        }
        Row++; Col = 0;
    }
    forceinline constexpr bool HandlePosition(const char32_t ch) noexcept
    {
        switch (ch)
        {
        case CharCR:
        case CharLF:
            HandleNewLine(ch);
            return true;
        default:
            Col++;
            return false;
        }
    }
};


}

