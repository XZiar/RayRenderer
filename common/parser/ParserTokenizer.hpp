#pragma once

#include "ParserContext.hpp"
#include "../EnumEx.hpp"
#include "../CharConvs.hpp"


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

enum class BaseToken : uint16_t { End = 0, Error, Unknown, Comment, Raw, Uint, Int, FP, String, Custom = 128, __RangeMin = End, __RangeMax = Custom };



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
    static std::string ToU8Str(const std::u32string_view txt)
    {
        std::string str;
        str.resize(txt.size());
        for (size_t idx = 0; idx < txt.size(); ++idx)
            str[idx] = static_cast<char>(txt[idx]);
        return str;
    };
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
        {
            auto txt = ctx.TryGetUntil(U"*/"); txt.remove_suffix(2);
            return GenerateToken(BaseToken::Comment, txt);
        }
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
        const auto idxBegin = ctx.Index;
        bool successful = false, inSlash = false;
        while (true)
        {
            const auto ch = ctx.GetNext();
            if (ch == ParserContext::CharEnd || ch == ParserContext::CharLF)
                break;
            if (!inSlash)
            {
                if (ch == '\\')
                    inSlash = true;
                else if (ch == '"')
                {
                    successful = true;
                    break;
                }
            }
            else
            {
                inSlash = false;
            }
        }

        auto content = ctx.Source.substr(idxBegin, ctx.Index - idxBegin);
        if (successful)
        {
            content.remove_suffix(1);
            return GenerateToken(BaseToken::String, content);
        }
        else
            return GenerateToken(BaseToken::Error, content);
    }
};


class IntTokenizer : public TokenizerBase
{
private:
    enum class States { Waiting, Num0, Normal, Binary, Hex, NotMatch };
    States State = States::Waiting;
    bool IsUnsigned = true;
public:
    constexpr void OnInitialize() noexcept
    {
        State = States::Waiting;
        IsUnsigned = true;
    }
    constexpr TokenizerResult OnChar(const char32_t ch, const size_t) noexcept
    {
        bool commonPath = true;
        switch (State)
        {
        case States::Waiting:
            if (ch == '-')
            {
                State = States::Normal;
                IsUnsigned = false;
                return TokenizerResult::Pending;
            }
            if (ch == '0')
            {
                State = States::Num0;
                return TokenizerResult::Waitlist;
            }
            break; // Normal
        case States::Num0:
            if (ch == 'x' || ch == 'X')
            {
                State = States::Hex;
                return TokenizerResult::Pending;
            }
            if (ch == 'b')
            {
                State = States::Binary;
                return TokenizerResult::Pending;
            }
            break; // Normal
        case States::Hex:
            if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))
            {
                State = States::Hex;
                return TokenizerResult::Waitlist;
            }
            else
            {
                commonPath = false;
                break; // not match
            }
        case States::Binary:
            if (ch == '0' || ch == '1')
            {
                State = States::Binary;
                return TokenizerResult::Waitlist;
            }
            else
            {
                commonPath = false;
                break; // not match
            }
        case States::Normal:
            break; // Normal
        default:
            commonPath = false;
            break; // not match
        }
        if (commonPath && ch >= '0' && ch <= '9')
        {
            State = States::Normal;
            return TokenizerResult::Waitlist;
        }
        State = States::NotMatch;
        return TokenizerResult::NotMatch;
    }
    ParserToken GetToken(ParserContext&, std::u32string_view txt) const noexcept
    {
        switch (State)
        {
        case States::Num0:
            return GenerateToken(BaseToken::Uint, 0);
        case States::Binary:
        {
            Expects(txt.size() > 2);
            if (txt.size() > 66)
                return GenerateToken(BaseToken::Error, txt);
            txt.remove_prefix(2);
            uint64_t bin = 0;
            while (true)
            {
                bin |= (txt.front() == '1' ? 0x1 : 0x0);
                txt.remove_prefix(1);
                if (txt.size() > 0)
                {
                    bin <<= 1;
                    continue;
                }
                else
                    break;
            }
            return GenerateToken(BaseToken::Uint, bin);
        }
        case States::Hex:
        {
            Expects(txt.size() > 2);
            if (txt.size() > 18)
                return GenerateToken(BaseToken::Error, txt);
            txt.remove_prefix(2);
            uint64_t hex = 0;
            while (true)
            {
                const auto ch = txt.front();
                if (ch >= 'a')
                    hex |= (static_cast<uint64_t>(ch) - 'a' + 10);
                else if (ch >= 'A')
                    hex |= (static_cast<uint64_t>(ch) - 'A' + 10);
                else
                    hex |=  static_cast<uint64_t>(ch) - '0';
                txt.remove_prefix(1);
                if (txt.size() > 0)
                {
                    hex <<= 4;
                    continue;
                }
                else
                    break;
            }
            return GenerateToken(BaseToken::Uint, hex);
        }
        case States::Normal:
        {
            const auto str = ToU8Str(txt);
            if (IsUnsigned)
            {
                uint64_t val = 0;
                const bool valid = common::StrToInt(str, val, 10).first;
                if (valid)
                    return GenerateToken(BaseToken::Uint, val);
            }
            else
            {
                int64_t val = 0;
                const bool valid = common::StrToInt(str, val, 10).first;
                if (valid)
                    return GenerateToken(BaseToken::Int, val);
            }
        } break;
        default:
            break;
        }
        return GenerateToken(BaseToken::Error, txt);
    }
};



}


}

