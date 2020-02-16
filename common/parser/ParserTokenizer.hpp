#pragma once

#include "ParserContext.hpp"
#include "../EnumEx.hpp"
#include "../CharConvs.hpp"


namespace common::parser
{


enum class BaseToken : uint16_t { End = 0, Error, Unknown, Delim, Comment, Raw, Bool, Uint, Int, FP, String, Custom = 128, __RangeMin = End, __RangeMax = Custom };

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

    constexpr bool                  GetBool()   const noexcept { /*Expects(Data2 == std::is_signed_v<T> ? 1 : 2);*/ return Data1 != 0; }
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
    template<typename T = BaseToken>
    constexpr T                     GetIDEnum() const noexcept 
    {
        if (ID <= common::enum_cast(T::__RangeMin)) return T::__RangeMin;
        if (ID >= common::enum_cast(T::__RangeMax)) return T::__RangeMax;
        return static_cast<T>(ID);
    }
};


#if COMMON_OS_WIN && _MSC_VER <= 1914
#   pragma message("ASCIIChecker may not be supported due to lack of constexpr support before VS 15.7")
#endif

template<bool Result = true>
struct ASCIIChecker
{
#if COMMON_OSBIT == 32
    using EleType = uint32_t;
#elif COMMON_OSBIT == 64
    using EleType = uint64_t;
#endif
    static constexpr uint8_t EleBits = sizeof(EleType) * 8;
    std::array<EleType, 128 / EleBits> LUT = { 0 };
    constexpr ASCIIChecker(const std::string_view str)
    {
        for (auto& ele : LUT)
            ele = Result ? std::numeric_limits<EleType>::min() : std::numeric_limits<EleType>::max();
        for (const auto ch : str)
        {
            if (ch >= 0 && ch < 128)
            {
                auto& ele = LUT[ch / EleBits];
                const auto obj = EleType(1) << (ch % EleBits);
                if constexpr (Result)
                    ele |= obj;
                else
                    ele &= (~obj);
            }
        }
    }

    constexpr bool operator()(const char32_t ch) const noexcept
    {
        if (ch >= 0 && ch < 128)
        {
            const auto ele = LUT[ch / EleBits];
            const auto ret = ele >> (ch % EleBits);
            return ret & 0x1 ? true : false;
        }
        else
            return !Result;
    }
};


namespace tokenizer
{


enum class TokenizerResult : uint8_t { NotMatch, Pending, Waitlist, Wrong, FullMatch, Preempt };

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

class DelimTokenizer : public TokenizerBase
{
private:
    ASCIIChecker<true> Checker;
public:
    constexpr DelimTokenizer(std::string_view delim = "(,)")
        : Checker(delim) { }
    forceinline constexpr void OnInitialize() noexcept
    { }
    constexpr TokenizerResult OnChar(const char32_t ch, const size_t idx) const noexcept
    {
        if (Checker(ch))
            return idx == 0 ? TokenizerResult::FullMatch : TokenizerResult::Preempt;
        else
            return TokenizerResult::Pending;
    }
    forceinline constexpr ParserToken GetToken(ContextReader&, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() == 1);
        return GenerateToken(BaseToken::Delim, txt[0]);
    }
};

class CommentTokenizer : public TokenizerBase
{
private:
    enum class States { Waiting, HasSlash, Singleline, Multiline };
    States State = States::Waiting;
public:
    forceinline constexpr void OnInitialize() noexcept
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
    constexpr ParserToken GetToken(ContextReader& reader, std::u32string_view) const noexcept
    {
        switch (State)
        {
        case States::Singleline:
            return GenerateToken(BaseToken::Comment, reader.ReadLine());
        case States::Multiline:
        {
            auto txt = reader.ReadUntil(U"*/"); txt.remove_suffix(2);
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
    forceinline constexpr void OnInitialize() noexcept
    { }
    constexpr TokenizerResult OnChar(const char32_t ch, const size_t) const noexcept
    {
        if (ch == '"')
            return TokenizerResult::FullMatch;
        else
            return TokenizerResult::NotMatch;
    }
    constexpr ParserToken GetToken(ContextReader& reader, std::u32string_view) const noexcept
    {
        bool successful = false;
        const auto content = reader.ReadWhile([&successful, inSlash = false](const char32_t ch) mutable
        {
            if (ch == special::CharEnd || ch == special::CharLF)
                return false;
            else if (!inSlash)
            {
                if (ch == '\\')
                    inSlash = true;
                else if (ch == '"')
                {
                    successful = true;
                    return false;
                }
            }
            else
            {
                inSlash = false;
            }
            return true;
        });
        if (successful)
        {
            reader.ReadNext(); // will be End/LF/"
            return GenerateToken(BaseToken::String, content);
        }
        else
            return GenerateToken(BaseToken::Error, content);
    }
};


class IntTokenizer : public TokenizerBase
{
private:
    enum class States { Init, Num0, Normal, Binary, Hex, NotMatch };
    States State = States::Init;
    bool IsUnsigned = true;
public:
    forceinline constexpr void OnInitialize() noexcept
    {
        State = States::Init;
        IsUnsigned = true;
    }
    constexpr TokenizerResult OnChar(const char32_t ch, const size_t) noexcept
    {
        constexpr auto NormalCheck = [](const char32_t ch_, States& state)
        {
            if (ch_ >= '0' && ch_ <= '9')
            {
                state = States::Normal;
                return TokenizerResult::Waitlist;
            }
            state = States::NotMatch;
            return TokenizerResult::NotMatch;
        };

#define RET(state, result) do { State = States::state; return TokenizerResult::result; } while(0)
        switch (State)
        {
        case States::Init:
            if (ch == '-')
            {
                IsUnsigned = false;
                RET(Normal, Pending);
            }
            else if (ch == '0')
                RET(Num0, Waitlist);
            else
                return NormalCheck(ch, State);
        case States::Num0:
            if (ch == 'x' || ch == 'X')
                RET(Hex, Pending);
            else if (ch == 'b')
                RET(Binary, Pending);
            else
                return NormalCheck(ch, State);
        case States::Hex:
            if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))
                RET(Hex, Waitlist);
            else
                RET(NotMatch, NotMatch);
        case States::Binary:
            if (ch == '0' || ch == '1')
                RET(Binary, Waitlist);
            else
                RET(NotMatch, NotMatch);
        case States::Normal:
            return NormalCheck(ch, State);
        default:
            RET(NotMatch, NotMatch);
        }
#undef RET
    }
    ParserToken GetToken(ContextReader&, std::u32string_view txt) const noexcept
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


class FPTokenizer : public TokenizerBase
{
private:
    enum class States { Init, Negative, FirstNum, Dot, Normal, Exp, Scientific, NotMatch };
    States State = States::Init;
public:
    forceinline constexpr void OnInitialize() noexcept
    {
        State = States::Init;
    }
    constexpr TokenizerResult OnChar(const char32_t ch, const size_t) noexcept
    {
#define RET(state, result) do { State = States::state; return TokenizerResult::result; } while(0)
        switch (State)
        {
        case States::Init:
            if (ch == '-')
                RET(Negative, Pending);
        case States::Negative:
            if (ch == '.')
                RET(Dot, Pending);
            if (ch >= '0' && ch <= '9')
                RET(FirstNum, Waitlist);
            RET(NotMatch, NotMatch);
        case States::FirstNum:
            if (ch == '.')
                RET(Dot, Pending);
            if (ch == 'e' || ch == 'E')
                RET(Exp, Pending);
            if (ch >= '0' && ch <= '9')
                RET(FirstNum, Waitlist);
            RET(NotMatch, NotMatch);
        case States::Dot:
            if (ch >= '0' && ch <= '9')
                RET(Normal, Waitlist);
            RET(NotMatch, NotMatch);
        case States::Normal:
            if (ch == 'e' || ch == 'E')
                RET(Exp, Pending);
            if (ch >= '0' && ch <= '9')
                RET(Normal, Waitlist);
            RET(NotMatch, NotMatch);
        case States::Exp:
            if (ch == '-')
                RET(Scientific, Pending);
        case States::Scientific:
            if (ch >= '0' && ch <= '9')
                RET(Scientific, Waitlist);
            RET(NotMatch, NotMatch);
        default:
            RET(NotMatch, NotMatch);
        }
#undef RET
    }
    ParserToken GetToken(ContextReader&, std::u32string_view txt) const noexcept
    {
        switch (State)
        {
        case States::FirstNum:
        case States::Normal:
        {
            const auto str = ToU8Str(txt);
            double val = 0;
            const bool valid = common::StrToFP(str, val, false).first;
            if (valid)
                return GenerateToken(BaseToken::FP, val);
        } break;
        case States::Scientific:
        {
            const auto str = ToU8Str(txt);
            double val = 0;
            const bool valid = common::StrToFP(str, val, true).first;
            if (valid)
                return GenerateToken(BaseToken::FP, val);
        } break;
        default:
            break;
        }
        return GenerateToken(BaseToken::Error, txt);
    }
};


class BoolTokenizer : public TokenizerBase
{
private:
    enum class States { Init, T_T, T_TR, T_TRU, F_F, F_FA, F_FAL, F_FALS, Match, NotMatch };
    States State = States::Init;
public:
    forceinline constexpr void OnInitialize() noexcept
    {
        State = States::Init;
    }
    constexpr TokenizerResult OnChar(const char32_t ch, const size_t) noexcept
    {
#define RET(state, result) do { State = States::state; return TokenizerResult::result; } while(0)
        switch (State)
        {
        case States::Init:
            if (ch == 'T' || ch == 't')
                RET(T_T, Pending);
            if (ch == 'F' || ch == 'f')
                RET(F_F, Pending);
            break;
        case States::T_T:
            if (ch == 'R' || ch == 'r')
                RET(T_TR, Pending);
            break;
        case States::T_TR:
            if (ch == 'U' || ch == 'u')
                RET(T_TRU, Pending);
            break;
        case States::F_F:
            if (ch == 'A' || ch == 'a')
                RET(F_FA, Pending);
            break;
        case States::F_FA:
            if (ch == 'L' || ch == 'l')
                RET(F_FAL, Pending);
            break;
        case States::F_FAL:
            if (ch == 'S' || ch == 's')
                RET(F_FALS, Pending);
            break;
        case States::T_TRU:
        case States::F_FALS:
            if (ch == 'E' || ch == 'e')
                RET(Match, Waitlist);
            break;
        default:
            break;
        }
        RET(NotMatch, NotMatch);
#undef RET
    }
    ParserToken GetToken(ContextReader&, std::u32string_view txt) const noexcept
    {
        if (State == States::Match)
            return GenerateToken(BaseToken::Bool, (txt[0] == 'T' || txt[0] == 't') ? 1 : 0);
        return GenerateToken(BaseToken::Error, txt);
    }
};


class ASCIIRawTokenizer : public TokenizerBase
{
private:
    ASCIIChecker<true> Checker;
public:
    constexpr ASCIIRawTokenizer(std::string_view str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") 
        : Checker(str) { }
    forceinline constexpr void OnInitialize() noexcept
    { }
    constexpr TokenizerResult OnChar(const char32_t ch, const size_t) const noexcept
    {
        if (Checker(ch))
            return TokenizerResult::Waitlist;
        else
            return TokenizerResult::NotMatch;
    }
    forceinline constexpr ParserToken GetToken(ContextReader&, std::u32string_view txt) const noexcept
    {
        return GenerateToken(BaseToken::Raw, txt);
    }
};


class ASCII2PartTokenizer : public TokenizerBase
{
private:
    ASCIIChecker<true> FirstChecker;
    ASCIIChecker<true> SecondChecker;
    uint16_t FirstPartLen;
    uint16_t TokenID;
public:
    template<typename T = BaseToken>
    constexpr ASCII2PartTokenizer(
        const T tokenId = BaseToken::Raw, const uint16_t firstPartLen = 1,
        std::string_view first  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
        std::string_view second = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_")
        : FirstChecker(first), SecondChecker(second), FirstPartLen(firstPartLen), TokenID(enum_cast(tokenId)) 
    { }
    forceinline constexpr void OnInitialize() noexcept
    { }
    constexpr TokenizerResult OnChar(const char32_t ch, const size_t idx) const noexcept
    {
        const auto& checker = idx < FirstPartLen ? FirstChecker : SecondChecker;
        if (checker(ch))
            return TokenizerResult::Waitlist;
        else
            return TokenizerResult::NotMatch;
    }
    forceinline ParserToken GetToken(ContextReader&, std::u32string_view txt) const noexcept
    {
        return ParserToken(TokenID, txt);
    }
};


}


}

