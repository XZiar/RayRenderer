#pragma once

#include "ParserContext.hpp"
#include "../EnumEx.hpp"
#include "../CharConvs.hpp"


namespace common::parser
{

namespace detail
{
struct TokenizerHelper
{
    static forceinline std::string ToU8Str(const std::u32string_view txt) noexcept
    {
        std::string str;
        str.resize(txt.size());
        for (size_t idx = 0; idx < txt.size(); ++idx)
            str[idx] = static_cast<char>(txt[idx]);
        return str;
    };
};
}

enum class BaseToken : uint16_t { End = 0, Error, Unknown, Delim, Comment, Raw, Bool, Uint, Int, FP, String, Custom = 128, __RangeMin = End, __RangeMax = Custom };

class ParserToken
{
private:
    enum class Type : uint16_t { Empty, Bool, Char, Str, FP, Uint, Int };
    union DataUnion
    {
        uint8_t Dummy;
        uint64_t Uint;
        int64_t Int;
        double FP;
        char32_t Char;
        bool Bool;
        const char32_t* Ptr;
        constexpr DataUnion()                    noexcept : Dummy(0)  { }
        constexpr DataUnion(const uint64_t val)  noexcept : Uint(val) { }
        constexpr DataUnion(const int64_t val)   noexcept : Int (val) { }
        constexpr DataUnion(const double val)    noexcept : FP  (val) { }
        constexpr DataUnion(const char32_t val)  noexcept : Char(val) { }
        constexpr DataUnion(const bool val)      noexcept : Bool(val) { }
        constexpr DataUnion(const char32_t* val) noexcept : Ptr (val) { }
    } Data;
    uint32_t Data2;
    uint16_t ID;
    Type ValType;
    template<Type TE, typename TD>
    struct DummyTag {};
    template<typename TV>
    static constexpr auto CastType() noexcept
    {
        using T = std::decay_t<TV>;
        if constexpr (std::is_floating_point_v<T>)
            return DummyTag<Type::FP, double>{};
        else if constexpr (std::is_same_v<T, char32_t>)
            return DummyTag<Type::Char, char32_t>{};
        else if constexpr (std::is_unsigned_v<T>)
            return DummyTag<Type::Uint, uint64_t>{};
        else if constexpr (std::is_signed_v<T>)
            return DummyTag<Type::Int, int64_t>{};
        else
            return DummyTag<Type::Empty, void>{};
    }
    template<Type TE, typename TD, typename T>
    constexpr ParserToken(DummyTag<TE, TD>, const uint16_t id, const T val) noexcept :
        Data(static_cast<TD>(val)), Data2(0), ID(id), ValType(TE) 
    { }
public:
    template<typename E>
    constexpr ParserToken(E id) noexcept :
        Data(), Data2(0), ID(static_cast<uint16_t>(id)), ValType(Type::Empty)
    { }
    template<typename E>
    constexpr ParserToken(E id, const std::u32string_view val) noexcept :
        Data(val.data()), Data2(gsl::narrow_cast<uint32_t>(val.size())), ID(static_cast<uint16_t>(id)), ValType(Type::Str)
    { }
    template<typename E>
    constexpr ParserToken(E id, const bool val) noexcept :
        Data(val), Data2(0), ID(static_cast<uint16_t>(id)), ValType(Type::Bool)
    { }
    template<typename E, typename T>
    constexpr ParserToken(E id, const T val) noexcept : 
        ParserToken(CastType<T>(), static_cast<uint16_t>(id), val)
    { }

    [[nodiscard]] constexpr bool                  GetBool()   const noexcept { return Data.Bool; }
    [[nodiscard]] constexpr int64_t               GetInt()    const noexcept { return Data.Int; }
    [[nodiscard]] constexpr uint64_t              GetUInt()   const noexcept { return Data.Uint; }
    [[nodiscard]] constexpr char32_t              GetChar()   const noexcept { return Data.Char; }
    [[nodiscard]] constexpr double                GetDouble() const noexcept { return Data.FP; }
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    [[nodiscard]] constexpr T                     GetIntNum() const noexcept { return static_cast<T>(Data.Uint); }
    template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    [[nodiscard]] constexpr T                     GetFPNum()  const noexcept { return static_cast<T>(Data.FP); }
    [[nodiscard]] constexpr std::u32string_view   GetString() const noexcept { return { Data.Ptr, Data2 }; }

    [[nodiscard]] constexpr uint16_t              GetID()     const noexcept { return ID; }
    template<typename T = BaseToken>
    [[nodiscard]] constexpr T                     GetIDEnum() const noexcept
    {
        if (ID <= common::enum_cast(T::__RangeMin)) return T::__RangeMin;
        if (ID >= common::enum_cast(T::__RangeMax)) return T::__RangeMax;
        return static_cast<T>(ID);
    }

    [[nodiscard]] constexpr bool operator==(const ParserToken& token) const noexcept
    {
        if (this->ID == token.ID && this->ValType == token.ValType)
        {
            switch (this->ValType)
            {
            case Type::Uint:  return this->Data.Uint == token.Data.Uint;
            case Type::Int:   return this->Data.Int  == token.Data.Int;
            case Type::FP:    return this->Data.FP   == token.Data.FP;
            case Type::Char:  return this->Data.Char == token.Data.Char;
            case Type::Bool:  return this->Data.Bool == token.Data.Bool;
            case Type::Str:   return GetString()     == token.GetString();
            case Type::Empty: return true;
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
    constexpr ASCIIChecker(const std::string_view str) noexcept
    {
        for (auto& ele : LUT)
            ele = Result ? std::numeric_limits<EleType>::min() : std::numeric_limits<EleType>::max();
        for (const auto ch : str)
        {
            if (ch >= 0) // char is signed
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

    [[nodiscard]] constexpr bool operator()(const char32_t ch) const noexcept
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

enum class TokenizerResult : uint8_t { Pending, Waitlist, NotMatch, Wrong, FullMatch };


class TokenizerBase {};


class DelimTokenizer : public TokenizerBase
{
private:
    ASCIIChecker<true> Checker;
public:
    using StateData = void;
    constexpr DelimTokenizer(std::string_view delim = "(,)") noexcept
        : Checker(delim) { }
    [[nodiscard]] forceinline constexpr TokenizerResult OnChar(const char32_t ch, const size_t idx) const noexcept
    {
        Expects(idx == 0);
        if (Checker(ch))
            return TokenizerResult::FullMatch;
        else
            return TokenizerResult::NotMatch;
    }
    [[nodiscard]] forceinline constexpr ParserToken GetToken(ContextReader&, std::u32string_view txt) const noexcept
    {
        Expects(txt.size() == 1);
        return ParserToken(BaseToken::Delim, txt[0]);
    }
};

class CommentTokenizer : public TokenizerBase
{
public:
    enum class States : uint32_t { Waiting, HasSlash, Singleline, Multiline };
    using StateData = States;
    [[nodiscard]] constexpr std::pair<States, TokenizerResult> OnChar(const States state, const char32_t ch, const size_t) const noexcept
    {
        switch (state)
        {
        case States::Waiting:
            if (ch == '/')
                return { States::HasSlash, TokenizerResult::Pending };
            else
                return { state, TokenizerResult::NotMatch };
        case States::HasSlash:
            if (ch == '/')
                return { States::Singleline, TokenizerResult::FullMatch };
            else if (ch == '*')
                return { States::Multiline, TokenizerResult::FullMatch };
            else
                return { States::Waiting, TokenizerResult::NotMatch };
        default:
            return { state, TokenizerResult::Wrong };
        }
    }
    [[nodiscard]] forceinline constexpr ParserToken GetToken(const States state, ContextReader& reader, std::u32string_view) const noexcept
    {
        switch (state)
        {
        case States::Singleline:
            return ParserToken(BaseToken::Comment, reader.ReadLine());
        case States::Multiline:
        {
            auto txt = reader.ReadUntil(U"*/"); txt.remove_suffix(2);
            return ParserToken(BaseToken::Comment, txt);
        }
        default:
            return ParserToken(BaseToken::Error);
        }
    }
};


class StringTokenizer : public TokenizerBase
{
public:
    using StateData = void;
    [[nodiscard]] forceinline constexpr TokenizerResult OnChar(const char32_t ch, const size_t idx) const noexcept
    {
        Expects(idx == 0);
        if (ch == '"')
            return TokenizerResult::FullMatch;
        else
            return TokenizerResult::NotMatch;
    }
    [[nodiscard]] constexpr ParserToken GetToken(ContextReader& reader, std::u32string_view) const noexcept
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
            return ParserToken(BaseToken::String, content);
        }
        else
            return ParserToken(BaseToken::Error, content);
    }
};


class IntTokenizer : public TokenizerBase
{
private:
    enum class States : uint16_t { Init, Num0, Normal, Binary, Hex, NotMatch, UnsEnd };
    static constexpr uint32_t SignedFlag   = 0x80000000u;
public:
    using StateData = uint32_t;
    [[nodiscard]] constexpr std::pair<uint32_t, TokenizerResult> OnChar(const uint32_t state, const char32_t ch, const size_t) const noexcept
    {
        bool forceSigned   = state & SignedFlag;
#define RET(state, result) return { (forceSigned ? SignedFlag : 0u) | common::enum_cast(States::state), TokenizerResult::result }
        switch (static_cast<States>(state))
        {
        case States::Init:
            if (ch == '-')
            {
                forceSigned = true;
                RET(Normal, Pending);
            }
            else if (ch == '0')
                RET(Num0, Waitlist);
            else
                break; // normal check
        case States::Num0:
            if (ch == 'x' || ch == 'X')
                RET(Hex, Pending);
            else if (ch == 'b')
                RET(Binary, Pending);
            else if (ch == 'u')
                RET(UnsEnd, Waitlist);
            else
                break; // normal check
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
            break; // normal check
        default:
            RET(NotMatch, NotMatch);
        }

        if (ch >= '0' && ch <= '9')
            RET(Normal, Waitlist);
        else if (ch == 'u')
            RET(UnsEnd, Waitlist);
        else
            RET(NotMatch, NotMatch);
#undef RET
    }
    [[nodiscard]] ParserToken GetToken(const uint32_t state, ContextReader&, std::u32string_view txt) const noexcept
    {
        const auto realState = static_cast<States>(state);
        switch (realState)
        {
        case States::Num0:
            return ParserToken(BaseToken::Int, 0);
        case States::Binary:
        {
            Expects(txt.size() > 2);
            if (txt.size() > 66)
                return ParserToken(BaseToken::Error, txt);
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
            return ParserToken(BaseToken::Uint, bin);
        }
        case States::Hex:
        {
            Expects(txt.size() > 2);
            if (txt.size() > 18)
                return ParserToken(BaseToken::Error, txt);
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
            return ParserToken(BaseToken::Uint, hex);
        }
        case States::UnsEnd:
        case States::Normal:
        {
            const bool endWithU = realState == States::UnsEnd;
            auto str = detail::TokenizerHelper::ToU8Str(txt);
            if (endWithU) str.pop_back(); // remove 'u'
            uint64_t val_ = 0;
            bool valid = false;
            if (state & SignedFlag)
            {
                int64_t val = 0;
                valid = common::StrToInt(str, val, 10).first;
                val_ = static_cast<uint64_t>(val);
            }
            else
            {
                uint64_t val = 0;
                valid = common::StrToInt(str, val, 10).first;
                val_ = val;
            }
            if (valid)
            {
                if (endWithU)
                    return ParserToken(BaseToken::Uint, val_);
                else
                    return ParserToken(BaseToken::Int, static_cast<int64_t>(val_));
            }
        } break;
        default:
            break;
        }
        return ParserToken(BaseToken::Error, txt);
    }
};


class FPTokenizer : public TokenizerBase
{
public:
    enum class States : uint32_t { Init, Negative, FirstNum, Dot, Normal, Exp, Scientific, NotMatch };
    using StateData = States;
    [[nodiscard]] constexpr std::pair<States, TokenizerResult> OnChar(const States state, const char32_t ch, const size_t) const noexcept
    {
#define RET(state, result) return { States::state, TokenizerResult::result }
        switch (state)
        {
        case States::Init:
            if (ch == '-')
                RET(Negative, Pending);
            [[fallthrough]];
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
            [[fallthrough]];
        case States::Scientific:
            if (ch >= '0' && ch <= '9')
                RET(Scientific, Waitlist);
            RET(NotMatch, NotMatch);
        default:
            RET(NotMatch, NotMatch);
        }
#undef RET
    }
    [[nodiscard]] ParserToken GetToken(const States state, ContextReader&, std::u32string_view txt) const noexcept
    {
        switch (state)
        {
        case States::FirstNum:
        case States::Normal:
        {
            const auto str = detail::TokenizerHelper::ToU8Str(txt);
            double val = 0;
            const bool valid = common::StrToFP(str, val, false).first;
            if (valid)
                return ParserToken(BaseToken::FP, val);
        } break;
        case States::Scientific:
        {
            const auto str = detail::TokenizerHelper::ToU8Str(txt);
            double val = 0;
            const bool valid = common::StrToFP(str, val, true).first;
            if (valid)
                return ParserToken(BaseToken::FP, val);
        } break;
        default:
            break;
        }
        return ParserToken(BaseToken::Error, txt);
    }
};


class BoolTokenizer : public TokenizerBase
{
public:
    enum class States : uint32_t { Init, T_T, T_TR, T_TRU, F_F, F_FA, F_FAL, F_FALS, Match, NotMatch };
    using StateData = States;
    [[nodiscard]] constexpr std::pair<States, TokenizerResult> OnChar(const States state, const char32_t ch, const size_t) const noexcept
    {
#define RET(state, result) return { States::state, TokenizerResult::result }
        switch (state)
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
    [[nodiscard]] ParserToken GetToken(const States state, ContextReader&, std::u32string_view txt) const noexcept
    {
        if (state == States::Match)
            return ParserToken(BaseToken::Bool, (txt[0] == 'T' || txt[0] == 't') ? 1 : 0);
        return ParserToken(BaseToken::Error, txt);
    }
};


class ASCIIRawTokenizer : public TokenizerBase
{
private:
    ASCIIChecker<true> Checker;
public:
    using StateData = void;
    constexpr ASCIIRawTokenizer(std::string_view str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") noexcept
        : Checker(str) { }
    [[nodiscard]] forceinline constexpr TokenizerResult OnChar(const char32_t ch, const size_t) const noexcept
    {
        if (Checker(ch))
            return TokenizerResult::Waitlist;
        else
            return TokenizerResult::NotMatch;
    }
    [[nodiscard]] forceinline constexpr ParserToken GetToken(ContextReader&, std::u32string_view txt) const noexcept
    {
        return ParserToken(BaseToken::Raw, txt);
    }
};


class ASCII2PartTokenizer : public TokenizerBase
{
protected:
    ASCIIChecker<true> FirstChecker;
    ASCIIChecker<true> SecondChecker;
    uint16_t FirstPartLen;
    uint16_t TokenID;
public:
    using StateData = void;
    template<typename T = BaseToken>
    constexpr ASCII2PartTokenizer(
        const T tokenId = BaseToken::Raw, const uint16_t firstPartLen = 1,
        std::string_view first = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
        std::string_view second = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") noexcept
        : FirstChecker(first), SecondChecker(second), FirstPartLen(firstPartLen), TokenID(enum_cast(tokenId))
    { }
    [[nodiscard]] forceinline constexpr TokenizerResult OnChar(const char32_t ch, const size_t idx) const noexcept
    {
        const auto& checker = idx < FirstPartLen ? FirstChecker : SecondChecker;
        if (checker(ch))
            return TokenizerResult::Waitlist;
        else
            return TokenizerResult::NotMatch;
    }
    [[nodiscard]] forceinline ParserToken GetToken(ContextReader&, std::u32string_view txt) const noexcept
    {
        return ParserToken(TokenID, txt);
    }
};


}


}

