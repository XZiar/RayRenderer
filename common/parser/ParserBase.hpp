#pragma once

#include "ParserContext.hpp"
#include "ParserTokenizer.hpp"
#include "../CharConvs.hpp"


namespace common::parser
{


class ParserBase
{
protected:
    template<typename T, typename... Args>
    forceinline static constexpr ParserToken GenerateToken(T enumval, Args&&... args)
    {
        return ParserToken(common::enum_cast(enumval), std::forward<Args>(args)...);
    }
public:
    ParserBase(ParserContext& ctx) : Context(ctx) { }
public:
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
    ParserToken GetToken(const std::u32string_view delim) noexcept
    {
        enum class States { RAW, NUM0, INT, UINT, FP, HEX, BIN, STR, STREND, ERROR };
        auto state = States::RAW;
        size_t idxBegin = Context.Index, idxEnd = idxBegin;
        const auto data = Context.TryGetWhile<false>([&](const char32_t ch, const size_t idx)
            {
                if (state != States::STR && delim.find_first_of(ch) != std::u32string_view::npos)
                    return false;
                idxEnd = idx;
                switch (state)
                {
                case States::RAW:
                {
                    if (ch == '"')
                        state = States::STR;
                    else if (ch == '-')
                        state = States::INT;
                    else if (ch == '.')
                        state = States::FP;
                    else if (ch == '0')
                        state = States::NUM0;
                    else if (ch > '0' && ch <= '9')
                        state = States::UINT;
                } break;
                case States::STREND:
                    state = States::ERROR; break;
                case States::NUM0:
                {
                    if (ch == 'b' || ch == 'B')
                        state = States::BIN;
                    else if (ch == 'x' || ch == 'X' || ch == 'h' || ch == 'H')
                        state = States::HEX;
                    else if (ch >= '0' && ch <= '9')
                        state = States::UINT;
                    else if (ch == '.')
                        state = States::FP;
                    else
                        state = States::ERROR;
                } break;
                case States::FP:
                {
                    if (!(ch >= '0' && ch <= '9'))
                        state = States::ERROR;
                } break;
                case States::INT:
                case States::UINT:
                {
                    if (ch == '.')
                        state = States::FP;
                    else if (ch < '0' || ch > '9')
                        state = States::ERROR;
                } break;
                case States::BIN:
                {
                    if (!(ch == '0' || ch == '1'))
                        state = States::ERROR;
                } break;
                case States::HEX:
                {
                    if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')))
                        state = States::ERROR;
                } break;
                case States::STR:
                {
                    if (ch == '"')
                        state = States::STREND;
                } break;
                default:
                    break;
                }
                if (state == States::ERROR)
                    return false;
                return true;
            });

        constexpr auto ToU8Str = [](const std::u32string_view txt)
        {
            std::string str;
            str.resize(txt.size());
            for (size_t idx = 0; idx < txt.size(); ++idx)
                str[idx] = static_cast<char>(txt[idx]);
            return str;
        };

        switch (state)
        {
        case States::STR:
        case States::ERROR:
            return GenerateToken(BaseToken::Error, Context.Source.substr(idxBegin, idxEnd - idxBegin + 1));
        case States::RAW:
            return GenerateToken(BaseToken::Raw, data);
        case States::STREND:
            return GenerateToken(BaseToken::String, data.substr(1, data.size() - 2));
        case States::NUM0:
        {
            return GenerateToken(BaseToken::Uint, 0u);
        }
        case States::UINT:
        {
            uint64_t val = 0;
            const auto tmp = ToU8Str(data);
            StrToInt(tmp, val, 10);
            return GenerateToken(BaseToken::Uint, val);
        }
        case States::INT:
        {
            int64_t val = 0;
            const auto tmp = ToU8Str(data);
            StrToInt(tmp, val, 10);
            return GenerateToken(BaseToken::Int, val);
        }
        case States::FP:
        {
            double val = 0;
            const auto tmp = ToU8Str(data);
            StrToFP(tmp, val);
            return GenerateToken(BaseToken::FP, val);
        }
        default:
            break;
        }
        return common::enum_cast(BaseToken::End);
    }
private:
    ParserContext& Context;
};



template<typename... TKs>
class ParserBase2 : public tokenizer::TokenizerBase
{
private:
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
    bool InvokeTokenizer(const char32_t ch, const size_t idx, size_t& pendings, size_t& waitlist) noexcept
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
                return true;
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
            return false;
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
    ParserBase2(ParserContext& ctx) : Context(ctx), Status({ tokenizer::TokenizerResult::Pending })
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
    ParserToken GetToken(const std::u32string_view delim) noexcept
    {
        using tokenizer::TokenizerResult;
        IgnoreBlank();

        InitTokenizer();

        size_t pendings = 0, waitlist = 0, count = 0;
        bool hasFullMatch = false;
        const auto idxBegin = Context.Index;

        while (true)
        {
            const auto ch = Context.GetNext();
            if (ch == ParserContext::CharEnd || ch == ParserContext::CharLF)
                break;
            if (delim.find_first_of(ch) != std::u32string_view::npos)
                break;
            pendings = waitlist = 0;
            hasFullMatch = InvokeTokenizer(ch, count, pendings, waitlist);
            if (hasFullMatch || pendings + waitlist == 0)
                break;
            count++;
        }
        
        const auto tokenTxt = Context.Source.substr(idxBegin, Context.Index - idxBegin);
        if (hasFullMatch)
            return OutputToken(Context, tokenTxt, TokenizerResult::FullMatch);
        else if (waitlist > 0)
            return OutputToken(Context, tokenTxt, TokenizerResult::Waitlist);
        else
            return GenerateToken(BaseToken::Unknown, tokenTxt);
    }
private:
    ParserContext& Context;
    std::tuple<TKs...> Tokenizers;
    ResultArray Status;
};


}

