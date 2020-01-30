#pragma once

#include "ParserContext.hpp"
#include "../EnumEx.hpp"
#include <charconv>


namespace common::parser
{


class ParserToken
{
private:
    uint64_t Data1;
    uintptr_t Data2;
    uint16_t ID;
    union IntWrapper
    {
        double FPVal;
        uint64_t UINTVal;
        IntWrapper(const uint64_t val) : UINTVal(val) {}
        IntWrapper(const double val) : FPVal(val) {}
    };
    struct UINTTag {};
    struct INTTag {};
    struct FPTag {};
    struct ErrorTag {};

    template<typename T>
    constexpr ParserToken(INTTag, uint16_t id, const T val) noexcept :
        Data1(static_cast<std::make_unsigned_t<T>>(val)), Data2(1), ID(id)
    { }
    template<typename T>
    constexpr ParserToken(UINTTag, uint16_t id, const T val) noexcept :
        Data1(val), Data2(2), ID(id)
    { }
    template<typename T>
    ParserToken(FPTag, uint16_t id, const T val) noexcept : 
        Data1(IntWrapper(val).UINTVal), Data2(4), ID(id)
    { }
public:
    constexpr ParserToken(uint16_t id) noexcept : 
        Data1(0), Data2(0), ID(id)
    { }
    constexpr ParserToken(uint16_t id, char32_t val) noexcept : 
        Data1(val), Data2(3), ID(id)
    { }
    ParserToken(uint16_t id, const std::u32string_view val) noexcept : 
        Data1(val.size()), Data2(reinterpret_cast<uintptr_t>(val.data())), ID(id)
    { }

    template<typename T, typename Tag = std::conditional_t<
        !std::is_floating_point_v<T>,
        std::conditional_t<
            std::is_integral_v<T>,
            std::conditional_t<std::is_unsigned_v<T>, UINTTag, INTTag>,
            ErrorTag>,
        FPTag>>
    ParserToken(uint16_t id, const T val) :
        ParserToken(Tag{}, id, val) { }

    constexpr int64_t               GetInt()    const noexcept { /*Expects(Data2 == 1);*/ return static_cast<int64_t>(Data1); }
    constexpr uint64_t              GetUInt()   const noexcept { /*Expects(Data2 == 2);*/ return Data1; }
    constexpr char32_t              GetChar()   const noexcept { /*Expects(Data2 == 3);*/ return static_cast<char32_t>(Data1); }
              double                GetDouble() const noexcept { /*Expects(Data2 == 4);*/ return IntWrapper(Data1).FPVal;  }
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    constexpr T                     GetIntNum() const noexcept { /*Expects(Data2 == std::is_signed_v<T> ? 1 : 2);*/ return static_cast<T>(Data1); }
    template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    constexpr T                     GetFPNum()  const noexcept { /*Expects(Data2 == 4);*/ return static_cast<T>(Data1); }
              std::u32string_view   GetString() const noexcept { return std::u32string_view(reinterpret_cast<const char32_t*>(Data2), Data1); }
              
    constexpr uint16_t              GetID()     const noexcept { return ID; }
    template<typename T>
    constexpr T                     GetIDEnum() const noexcept 
    {
        if (ID <= common::enum_cast(T::__RangeMin)) return T::__RangeMin;
        if (ID >= common::enum_cast(T::__RangeMax)) return T::__RangeMax;
        return static_cast<T>(ID);
    }
};
constexpr auto kk = sizeof(ParserToken);


class ParserBase
{
protected:
    template<typename T, typename... Args>
    forceinline static constexpr ParserToken GenerateToken(T enumval, Args&&... args)
    {
        return ParserToken(common::enum_cast(enumval), std::forward<Args>(args)...);
    }
public:
    enum class BaseToken : uint16_t { End = 0, Error, Raw, Uint, Int, FP, String, Custom = 128, __RangeMin = End, __RangeMax = Custom };
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
            auto ptr = tmp.data() + tmp.size();
#if COMPILER_MSVC
            std::from_chars(tmp.data(), tmp.data() + tmp.size(), val);
#else
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


}

