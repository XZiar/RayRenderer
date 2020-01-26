#pragma once

#include "../CommonRely.hpp"

namespace common::parser
{


class ParserContext
{
private:
public:
    struct SingleChar
    {
        friend class ParserContext;
        static constexpr char32_t LF = '\n';
        static constexpr char32_t CR = '\r';
        static constexpr char32_t End = static_cast<char32_t>(-1);
        enum class CharType : uint8_t { End, NewLine, Digit, WhiteSpace, Special, Normal };
        static constexpr CharType ParseType(const char32_t ch) noexcept
        {
            switch (ch)
            {
            case LF:
            case CR:
                return CharType::NewLine;
            case '\t':
            case ' ':
                return CharType::WhiteSpace;
            default:
                if (ch >= '0' && ch <= '9')
                    return CharType::Digit;
                return CharType::Normal;
            }
        }
        char32_t Val;
        CharType Type;
        constexpr SingleChar(const char32_t ch) noexcept : Val(ch), Type(ParseType(ch)) { }
        forceinline static constexpr SingleChar EndChar() noexcept { return { End, CharType::End }; }
        forceinline static constexpr SingleChar NewLineChar() noexcept { return { LF, CharType::NewLine }; }
    private:
        constexpr SingleChar(const char32_t ch, const CharType type) noexcept : Val(ch), Type(type) { }
    };

    std::u16string SourceName;
    std::u32string_view Source;
    size_t Index = 0;
    size_t Row = 0, Col = 0;

    ParserContext(const std::u32string_view source, const std::u16string_view name = u"") noexcept :
        SourceName(std::u16string(name)), Source(source)
    { }

    constexpr SingleChar GetNext() noexcept
    {
        if (Index >= Source.size())
            return SingleChar::EndChar();
        const auto ch = Source[Index++];
        if (HandlePosition(ch))
            return SingleChar::NewLineChar();
        return ch;
    }

    constexpr std::u32string_view GetLine() noexcept
    {
        const auto start = Index;
        while (Index < Source.size())
        {
            const auto ch = Source[Index++];
            if (ch == SingleChar::CR || ch == SingleChar::LF)
            {
                const auto txt = Source.substr(start, Index - start - 1);
                HandleNewLine(ch);
                return txt;
            }
        }
        Col += Index - start;
        return Source.substr(start);
    }

    constexpr std::u32string_view TryGetUntil(const std::u32string_view target, const bool allowMultiLine = true)
    {
        if (target.size() == 0)
            return {};
        const auto pos = Source.find(target, Index);
        return GetUntil(target.size(), pos, allowMultiLine);
    }

    constexpr std::u32string_view TryGetUntilAny(const std::u32string_view target, const bool allowMultiLine = true)
    {
        if (target.size() == 0)
            return {};
        const auto pos = Source.find_first_of(target, Index);
        return GetUntil(target.size(), pos, allowMultiLine);
    }

    constexpr std::u32string_view TryGetUntilNot(const std::u32string_view target, const bool allowMultiLine = true)
    {
        if (target.size() == 0)
            return {};
        const auto pos = Source.find_first_not_of(target, Index);
        return GetUntil(target.size(), pos, allowMultiLine);
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
            if (ch == SingleChar::CR || ch == SingleChar::LF)
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
        if (ch == SingleChar::CR)
        {
            if (Index < Source.size() && Source[Index] == SingleChar::LF)
                Index++;
        }
        Row++; Col = 0;
    }
    constexpr bool HandlePosition(const char32_t ch) noexcept
    {
        switch (ch)
        {
        case SingleChar::CR:
        case SingleChar::LF:
            HandleNewLine(ch);
            return true;
        default:
            Col++;
            return false;
        }
    }
};


}

