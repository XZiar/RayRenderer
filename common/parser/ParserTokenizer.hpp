#pragma once

#include "ParserContext.hpp"
#include "../EnumEx.hpp"

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

enum class BaseToken : uint16_t { End = 0, Error, Comment, Raw, Uint, Int, FP, String, Custom = 128, __RangeMin = End, __RangeMax = Custom };



namespace tokenizer
{


enum class TokenizerResult : uint8_t { NotMatch, Pending, Waitlist, Wrong, FullMatch };

class TokenizerBase
{
protected:
    template<typename T, typename... Args>
    forceinline static constexpr ParserToken GenerateToken(T enumval, Args&&... args)
    {
        return ParserToken(common::enum_cast(enumval), std::forward<Args>(args)...);
    }
};

class CommentTokenizer : public TokenizerBase
{
private:
    enum class States { Waiting, HasSlash, Singleline, Multiline };
    States State = States::Waiting;
public:
    constexpr void OnInitialize() noexcept
    {
        State = States::Waiting;
    }
    constexpr TokenizerResult OnChar(const char32_t ch, const size_t) noexcept
    {
        switch (State)
        {
        case States::Waiting:
            if (ch == '/')
            {
                State = States::HasSlash;
                return TokenizerResult::Pending;
            }
            else
            {
                return TokenizerResult::NotMatch;
            }
        case States::HasSlash:
            if (ch == '/')
            {
                State = States::Singleline;
                return TokenizerResult::FullMatch;
            }
            else if (ch == '*')
            {
                State = States::Multiline;
                return TokenizerResult::FullMatch;
            }
            else
            {
                State = States::Waiting;
                return TokenizerResult::NotMatch;
            }
        default:
            return TokenizerResult::Wrong;
        }
    }
    constexpr ParserToken GetToken(ParserContext& ctx, std::u32string_view) const noexcept
    {
        switch (State)
        {
        case States::Singleline:
            return GenerateToken(BaseToken::Comment, ctx.GetLine());
        case States::Multiline:
            return GenerateToken(BaseToken::Comment, ctx.TryGetUntil(U"*/"));
        default:
            return GenerateToken(BaseToken::Error);
        }
    }
};


class StringTokenizer : public TokenizerBase
{
public:
    constexpr void OnInitialize() noexcept
    { }
    constexpr TokenizerResult OnChar(const char32_t ch, const size_t) noexcept
    {
        if (ch == '"')
            return TokenizerResult::FullMatch;
        else
            return TokenizerResult::NotMatch;
    }
    constexpr ParserToken GetToken(ParserContext& ctx, std::u32string_view) const noexcept
    {
        bool successful = false;
        const auto content = ctx.TryGetWhile<false>([&, inSlash = false](const char32_t ch) mutable
        {
            if (successful == true)
                return false;
            if (ch == ParserContext::CharEnd)
                return false;
            if (!inSlash)
            {
                if (ch == '\\')
                    inSlash = true;
                else if (ch == '"')
                    successful = true;
            }
            else
            {
                inSlash = false;
            }
            return true;
        });

        if (successful)
            return GenerateToken(BaseToken::String, content);
        else
            return GenerateToken(BaseToken::Error, content);
    }
};



}


}

