#pragma once

#include "ParserContext.hpp"
#include "ParserTokenizer.hpp"
#include <charconv>


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
            std::string tmp(data.cbegin(), data.cend());
            std::from_chars(tmp.data(), tmp.data() + tmp.size(), val, 10);
            return GenerateToken(BaseToken::Uint, val);
        }
        case States::INT:
        {
            int64_t val = 0;
            std::string tmp(data.cbegin(), data.cend());
            std::from_chars(tmp.data(), tmp.data() + tmp.size(), val, 10);
            return GenerateToken(BaseToken::Int, val);
        }
        case States::FP:
        {
            double val = 0;
            std::string tmp(data.cbegin(), data.cend());
#if COMPILER_MSVC
            std::from_chars(tmp.data(), tmp.data() + tmp.size(), val);
#else
            auto ptr = tmp.data() + tmp.size();
            val = std::strtod(tmp.data(), &ptr);
#endif
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

    std::tuple<TKs...> Tokenizers;

    template<size_t N>
    bool InvokeTokenizer(ResultArray& results, const char32_t ch, const size_t idx, size_t& pendings) noexcept
    {
        auto& result = result[N];
        if (result == tokenizer::TokenizerResult::Pending)
        {
            auto& tokenizer = std::get<N>(Tokenizers);
            result = tokenizer.OnChar(ch, idx);
            if (result == tokenizer::TokenizerResult::Match)
                return true;
            else if (result == tokenizer::TokenizerResult::Pending)
                pendings++;
        }
        return false;
    }
    template<size_t... I>
    std::pair<size_t, bool> InvokeTokenizers(ResultArray& results, const char32_t ch, const size_t idx, std::index_sequence<I...>) noexcept
    {
        size_t pendings = 0;
        const auto anymatch = (InvokeTokenizer<I>(results, ch, idx, pendings) || ...);
        return { pendings, anymatch };
    }

    template<size_t N>
    void ProcessTokenizer(ResultArray& results, ParserContext& ctx, std::u32string_view tksv, ParserToken& token) noexcept
    {
        auto& result = result[N];
        if (result == tokenizer::TokenizerResult::Pending || result == tokenizer::TokenizerResult::Match)
        {
            auto& tokenizer = std::get<N>(Tokenizers);
            token = tokenizer.GetToken(ctx, tksv);
        }
    }
    template<size_t... I>
    ParserToken ProcessTokenizers(ResultArray& results, ParserContext& ctx, std::u32string_view tksv, std::index_sequence<I...>) noexcept
    {
        ParserToken token;
        (InvokeTokenizer<I>(results, ctx, tksv, token), ...);
        return token;
    }
public:
    ParserBase2(ParserContext& ctx) : Context(ctx) { }
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
        IgnoreBlank();

        ResultArray status;
        for (auto& res : status) 
            res = tokenizer::TokenizerResult::Pending;

        size_t idxBegin = Context.Index, idxEnd = idxBegin;
        bool successful = false;
        const auto tksv = Context.TryGetWhile<false>([&, count = 0](const char32_t ch, const size_t idx) mutable
            {
                if (delim.find_first_of(ch) != std::u32string_view::npos)
                    return false;
                const auto [pendings, anymatch] = InvokeTokenizers(status, ch, count, Indexes);
                if (anymatch || pendings == 1)
                {
                    successful = true; return false;
                }
                if (pendings == 0)
                    return false;
                return true;
            });
        if (successful)
        {
            return ProcessTokenizers(status, Context, tksv, Indexes);
        }
        else
        {
            return GenerateToken(BaseToken::Error, tksv);
        }
    }
private:
    ParserContext& Context;
};


}

