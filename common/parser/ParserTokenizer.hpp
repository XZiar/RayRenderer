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
    struct EmptyTag
    {
        static constexpr uint8_t ID = 0;
    };
    struct UINTTag
    {
        static constexpr uint8_t ID = 1;
    };
    struct INTTag
    {
        static constexpr uint8_t ID = 2;
    };
    struct FPTag
    {
        static constexpr uint8_t ID = 3;
    };
    struct CharTag
    {
        static constexpr uint8_t ID = 4;
    };
    struct StrTag
    {
        static constexpr uint8_t ID = 5;
    };
    struct ErrorTag {};
    union DataUnion
    {
        uint8_t Dummy;
        uint64_t Uint;
        int64_t Int;
        double FP;
        char32_t Char;
        std::u32string_view Str;
        constexpr DataUnion()                     noexcept : Dummy(0) { }
        template<typename T>
        constexpr DataUnion(UINTTag, const T val) noexcept : Uint(static_cast<std::make_unsigned_t<T>>(val)) { }
        template<typename T>
        constexpr DataUnion(INTTag, const T val) noexcept : Int(static_cast<std::make_signed_t<T>>  (val)) { }
        template<typename T>
        constexpr DataUnion(FPTag, const T val) noexcept : FP(static_cast<double>                 (val)) { }
        template<typename T>
        constexpr DataUnion(CharTag, const T val) noexcept : Char(static_cast<char32_t>               (val)) { }
        template<typename T>
        constexpr DataUnion(StrTag, const T val) noexcept : Str(static_cast<std::u32string_view>    (val)) { }
    } Data;
    uint16_t ID;
    uint8_t ValType;
public:
    constexpr ParserToken(uint16_t id) noexcept :
        Data(), ID(id), ValType(EmptyTag::ID)
    { }
    constexpr ParserToken(uint16_t id, bool val) noexcept :
        Data(UINTTag{}, val ? 1 : 0), ID(id), ValType(UINTTag::ID)
    { }
    constexpr ParserToken(uint16_t id, char32_t val) noexcept :
        Data(CharTag{}, val), ID(id), ValType(CharTag::ID)
    { }
    ParserToken(uint16_t id, const std::u32string_view val) noexcept :
        Data(StrTag{}, val), ID(id), ValType(StrTag::ID)
    { }

    template<typename T, typename Tag = std::conditional_t<
        !std::is_floating_point_v<T>,
        std::conditional_t<
        std::is_integral_v<T>,
        std::conditional_t<std::is_unsigned_v<T>, UINTTag, INTTag>,
        ErrorTag>,
        FPTag>>
        ParserToken(uint16_t id, const T val) :
        Data(Tag{}, val), ID(id), ValType(Tag::ID) { }

    constexpr bool                  GetBool()   const noexcept { /*Expects(Data2 == std::is_signed_v<T> ? 1 : 2);*/ return Data.Uint != 0; }
    constexpr int64_t               GetInt()    const noexcept { /*Expects(Data2 == 1);*/ return Data.Int; }
    constexpr uint64_t              GetUInt()   const noexcept { /*Expects(Data2 == 2);*/ return Data.Uint; }
    constexpr char32_t              GetChar()   const noexcept { /*Expects(Data2 == 3);*/ return Data.Char; }
    double                GetDouble() const noexcept { /*Expects(Data2 == 4);*/ return Data.FP; }
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    constexpr T                     GetIntNum() const noexcept { /*Expects(Data2 == std::is_signed_v<T> ? 1 : 2);*/ return static_cast<T>(Data.Uint); }
    template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    constexpr T                     GetFPNum()  const noexcept { /*Expects(Data2 == 4);*/ return static_cast<T>(Data.FP); }
    constexpr std::u32string_view   GetString() const noexcept { return Data.Str; }

    constexpr uint16_t              GetID()     const noexcept { return ID; }
    template<typename T = BaseToken>
    constexpr T                     GetIDEnum() const noexcept
    {
        if (ID <= common::enum_cast(T::__RangeMin)) return T::__RangeMin;
        if (ID >= common::enum_cast(T::__RangeMax)) return T::__RangeMax;
        return static_cast<T>(ID);
    }

    constexpr bool operator==(const ParserToken& token) const noexcept
    {
        if (this->ID == token.ID && this->ValType == token.ValType)
        {
            switch (this->ValType)
            {
            case UINTTag::ID: return this->Data.Uint == token.Data.Uint;
            case  INTTag::ID: return this->Data.Int == token.Data.Int;
            case   FPTag::ID: return this->Data.FP == token.Data.FP;
            case CharTag::ID: return this->Data.Char == token.Data.Char;
            case  StrTag::ID: return this->Data.Str == token.Data.Str;
            }
        }
        return false;
    }
};


#if COMPILER_MSVC && _MSC_VER <= 1914
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
                    hex |= static_cast<uint64_t>(ch) - '0';
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
        std::string_view first = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
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

