#include "SystemCommonPch.h"
#include "Format.h"
#include "Exceptions.h"
#include "StringFormat.h"
#include "StringConvert.h"
#define HALF_ENABLE_F16C_INTRINSICS 0 // avoid platform compatibility
#include "3rdParty/half/half.hpp"


#if COMMON_COMPILER_MSVC
#   define SYSCOMMONTPL SYSCOMMONAPI
#else
#   define SYSCOMMONTPL
#endif


namespace common::str::exp
{
using namespace std::string_view_literals;


void ArgChecker::CheckDDBasic(const StrArgInfo& strInfo, const ArgInfo& argInfo)
{
    const auto strIndexArgCount = static_cast<uint8_t>(strInfo.IndexTypes.size());
    const auto strNamedArgCount = static_cast<uint8_t>(strInfo.NamedTypes.size());
    if (argInfo.IdxArgCount < strIndexArgCount)
    {
        COMMON_THROW(BaseException, FMTSTR(u"No enough indexed arg, expects [{}], has [{}]"sv, 
            strIndexArgCount, argInfo.IdxArgCount));
    }
    if (argInfo.NamedArgCount < strNamedArgCount)
    {
        COMMON_THROW(BaseException, FMTSTR(u"No enough named arg, expects [{}], has [{}]"sv,
            strNamedArgCount, argInfo.NamedArgCount));
    }
    if (strIndexArgCount > 0)
    {
        const auto index = ArgChecker::GetIdxArgMismatch(strInfo.IndexTypes.data(), argInfo.IndexTypes, strIndexArgCount);
        if (index != ParseResult::IdxArgSlots)
            COMMON_THROW(BaseException, FMTSTR(u"IndexArg[{}] type mismatch"sv, index)).Attach("arg", index)
                .Attach("argType", std::pair{ strInfo.IndexTypes[index], ArgInfo::CleanRealType(argInfo.IndexTypes[index]) });
    }
}

template<typename Char>
ArgPack::NamedMapper ArgChecker::CheckDDNamedArg(const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo)
{
    ArgPack::NamedMapper mapper = { 0 };
    const auto strNamedArgCount = static_cast<uint8_t>(strInfo.NamedTypes.size());
    if (strNamedArgCount > 0)
    {
        const auto namedRet = ArgChecker::GetNamedArgMismatch(
            strInfo.NamedTypes.data(), strInfo.FormatString, strNamedArgCount,
            argInfo.NamedTypes, argInfo.Names, argInfo.NamedArgCount);
        if (namedRet.AskIndex)
        {
            const auto& namedArg = strInfo.NamedTypes[*namedRet.AskIndex];
            const auto name_ = strInfo.FormatString.substr(namedArg.Offset, namedArg.Length);
            std::string name;
            if constexpr (std::is_same_v<Char, char>)
                name.assign(name_);
            else
            {
                name.reserve(name_.size());
                for (const auto ch : name_)
                    name.push_back(static_cast<char>(ch));
            }
            if (namedRet.GiveIndex) // type mismatch
                COMMON_THROW(BaseException, FMTSTR(u"NamedArg [{}] type mismatch"sv, name_)).Attach("arg", name)
                .Attach("argType", std::pair{ namedArg.Type, ArgInfo::CleanRealType(argInfo.NamedTypes[*namedRet.GiveIndex]) });
            else // arg not found
                COMMON_THROW(BaseException, FMTSTR(u"NamedArg [{}] not found in args"sv, name_)).Attach("arg", name);
        }
        Ensures(!namedRet.GiveIndex);
        mapper = namedRet.Mapper;
    }
    return mapper;
}
template SYSCOMMONTPL ArgPack::NamedMapper ArgChecker::CheckDDNamedArg(const StrArgInfoCh<char>&     strInfo, const ArgInfo& argInfo);
template SYSCOMMONTPL ArgPack::NamedMapper ArgChecker::CheckDDNamedArg(const StrArgInfoCh<wchar_t>&  strInfo, const ArgInfo& argInfo);
template SYSCOMMONTPL ArgPack::NamedMapper ArgChecker::CheckDDNamedArg(const StrArgInfoCh<char16_t>& strInfo, const ArgInfo& argInfo);
template SYSCOMMONTPL ArgPack::NamedMapper ArgChecker::CheckDDNamedArg(const StrArgInfoCh<char32_t>& strInfo, const ArgInfo& argInfo);


struct FormatterHelper : public FormatterBase
{
    template<typename Char>
    static forceinline constexpr fmt::basic_format_specs<Char> ConvertSpecBasic(const FormatSpec& in) noexcept
    {
        fmt::basic_format_specs<Char> spec;
        spec.width = in.Width;
        spec.precision = static_cast<int32_t>(in.Precision);
        spec.alt = in.AlterForm;
        spec.localized = false;
        if (in.ZeroPad)
        {
            spec.fill[0] = static_cast<Char>('0');
            spec.align = fmt::align::numeric;
        }
        else
        {
            spec.fill[0] = static_cast<Char>(in.Fill);
            switch (in.Alignment)
            {
            default: 
            case FormatSpec::Align::None:   spec.align = fmt::align::none;   break;
            case FormatSpec::Align::Left:   spec.align = fmt::align::left;   break;
            case FormatSpec::Align::Middle: spec.align = fmt::align::center; break;
            case FormatSpec::Align::Right:  spec.align = fmt::align::right;  break;
            }
        }
        spec.sign = fmt::sign::none;
        return spec;
    }
    static forceinline constexpr fmt::sign_t ConvertSpecSign(const FormatSpec* spec) noexcept
    {
        if (!spec) return fmt::sign::none;
        switch (spec->SignFlag)
        {
        default:
        case FormatSpec::Sign::None:  return fmt::sign::none;
        case FormatSpec::Sign::Neg:   return fmt::sign::minus;
        case FormatSpec::Sign::Pos:   return fmt::sign::plus;
        case FormatSpec::Sign::Space: return fmt::sign::space;
        }
    }
    static forceinline constexpr uint32_t ConvertSpecIntSign(const FormatSpec* spec) noexcept
    {
        if (!spec) return 0;
        constexpr const uint32_t Prefixes[4] = { 0, 0, 0x1000000u | '+', 0x1000000u | ' ' };
        switch (spec->SignFlag)
        {
        default:
        case FormatSpec::Sign::None:  return Prefixes[fmt::sign::none];
        case FormatSpec::Sign::Neg:   return Prefixes[fmt::sign::minus];
        case FormatSpec::Sign::Pos:   return Prefixes[fmt::sign::plus];
        case FormatSpec::Sign::Space: return Prefixes[fmt::sign::space];
        }
    }
    static forceinline constexpr fmt::presentation_type ConvertSpecIntPresent(const FormatSpec* spec) noexcept
    {
        if (!spec) return fmt::presentation_type::none;
        // dbBoxX
        constexpr fmt::presentation_type Types[] = 
        {
            fmt::presentation_type::dec,
            fmt::presentation_type::bin_lower,
            fmt::presentation_type::bin_upper,
            fmt::presentation_type::oct,
            fmt::presentation_type::hex_lower,
            fmt::presentation_type::hex_upper,
        };
        return Types[spec->TypeExtra];
    }
    static forceinline constexpr fmt::presentation_type ConvertSpecFloatPresent(const FormatSpec* spec) noexcept
    {
        if (!spec) return fmt::presentation_type::none;
        // gGaAeEfF
        constexpr fmt::presentation_type Types[] =
        {
            fmt::presentation_type::general_lower,
            fmt::presentation_type::general_upper,
            fmt::presentation_type::hexfloat_lower,
            fmt::presentation_type::hexfloat_upper,
            fmt::presentation_type::exp_lower,
            fmt::presentation_type::exp_upper,
            fmt::presentation_type::fixed_lower,
            fmt::presentation_type::fixed_upper,
        };
        return Types[spec->TypeExtra];
    }
    template<typename T>
    static forceinline uint32_t ProcessIntSign(T& val, bool isSigned, const FormatSpec* spec) noexcept
    {
        using S = std::conditional_t<std::is_same_v<T, uint32_t>, int32_t, int64_t>;
        if (const auto signedVal = static_cast<S>(val); isSigned && signedVal < 0)
        {
            val = static_cast<T>(S(0) - signedVal);
            return 0x01000000 | '-';
        }
        else
        {
            return ConvertSpecIntSign(spec);
        }
    }
    template<typename Dst, typename Src>
    static void PutString(std::basic_string<Dst>& ret, std::basic_string_view<Src> str, const FormatSpec* spec)
    {
        if constexpr (std::is_same_v<Dst, Src>)
        {
            if (spec)
            {
                auto fmtSpec = ConvertSpecBasic<Dst>(*spec);
                fmtSpec.type = fmt::presentation_type::string;
                fmt::detail::write<Dst>(std::back_inserter(ret), str, fmtSpec);
                //fmt::format_to(std::back_inserter(ret), "");
            }
            else
                ret.append(str);
        }
        else if constexpr (std::is_same_v<Dst, char>)
        {
            PutString<char, char>(ret, str::to_string(str), spec);
        }
        else if constexpr (std::is_same_v<Dst, char16_t>)
        {
            PutString<char16_t, char16_t>(ret, str::to_u16string(str), spec);
        }
        else if constexpr (std::is_same_v<Dst, char32_t>)
        {
            PutString<char32_t, char32_t>(ret, str::to_u32string(str), spec);
        }
        else if constexpr (std::is_same_v<Dst, wchar_t>)
        {
            if constexpr (sizeof(wchar_t) == sizeof(char16_t))
            {
                const auto newStr = str::to_u16string(str);
                PutString<wchar_t, wchar_t>(ret, std::wstring_view{ reinterpret_cast<const wchar_t*>(newStr.data()), newStr.size() }, spec);
            }
            else if constexpr (sizeof(wchar_t) == sizeof(char32_t))
            {
                const auto newStr = str::to_u32string(str);
                PutString<wchar_t, wchar_t>(ret, std::wstring_view{ reinterpret_cast<const wchar_t*>(newStr.data()), newStr.size() }, spec);
            }
            else
                static_assert(!common::AlwaysTrue<Src>, "not supported");
        }
        else
            static_assert(!common::AlwaysTrue<Src>, "not supported");
    }
};


template<typename Char>
void Formatter<Char>::PutColor(StrType&, bool, Color) const { }

template<typename Char>
void Formatter<Char>::PutString(StrType& ret, std::   string_view str, const FormatSpec* spec) const
{
    FormatterHelper::PutString(ret, str, spec);
}
template<typename Char>
void Formatter<Char>::PutString(StrType& ret, std::  wstring_view str, const FormatSpec* spec) const
{
    FormatterHelper::PutString(ret, str, spec);
}
template<typename Char>
void Formatter<Char>::PutString(StrType& ret, std::u16string_view str, const FormatSpec* spec) const
{
    FormatterHelper::PutString(ret, str, spec);
}
template<typename Char>
void Formatter<Char>::PutString(StrType& ret, std::u32string_view str, const FormatSpec* spec) const
{
    FormatterHelper::PutString(ret, str, spec);
}

template<typename T>
forceinline std::basic_string_view<T> BuildStr(uintptr_t ptr, size_t len) noexcept
{
    const auto arg = reinterpret_cast<const T*>(ptr);
    if (len == SIZE_MAX)
        len = std::char_traits<T>::length(arg);
    return { arg, len };
}

template<typename Char>
void Formatter<Char>::PutInteger(StrType& ret, uint32_t val, bool isSigned, const FormatSpec* spec) const
{
    fmt::basic_format_specs<Char> fmtSpec = {};
    if (spec)
        fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
    fmtSpec.type = FormatterHelper::ConvertSpecIntPresent(spec);
    const auto prefix = FormatterHelper::ProcessIntSign(val, isSigned, spec);
    fmt::detail::write_int_arg<uint32_t> arg { val, prefix };
    fmt::detail::write_int(std::back_inserter(ret), arg, fmtSpec, {});
}
template<typename Char>
void Formatter<Char>::PutInteger(StrType& ret, uint64_t val, bool isSigned, const FormatSpec* spec) const
{
    fmt::basic_format_specs<Char> fmtSpec = {};
    if (spec)
        fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
    fmtSpec.type = FormatterHelper::ConvertSpecIntPresent(spec);
    const auto prefix = FormatterHelper::ProcessIntSign(val, isSigned, spec);
    fmt::detail::write_int_arg<uint64_t> arg{ val, prefix };
    fmt::detail::write_int(std::back_inserter(ret), arg, fmtSpec, {});
}

template<typename Char>
void Formatter<Char>::PutFloat(StrType& ret, float val, const FormatSpec* spec) const
{
    fmt::basic_format_specs<Char> fmtSpec = {};
    if (spec)
        fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
    fmtSpec.type = FormatterHelper::ConvertSpecFloatPresent(spec);
    fmtSpec.sign = FormatterHelper::ConvertSpecSign(spec);
    fmt::detail::write(std::back_inserter(ret), val, fmtSpec, {});
}
template<typename Char>
void Formatter<Char>::PutFloat(StrType& ret, double val, const FormatSpec* spec) const
{
    fmt::basic_format_specs<Char> fmtSpec = {};
    if (spec)
        fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
    fmtSpec.type = FormatterHelper::ConvertSpecFloatPresent(spec);
    fmtSpec.sign = FormatterHelper::ConvertSpecSign(spec);
    fmt::detail::write(std::back_inserter(ret), val, fmtSpec, {});
}

template<typename Char>
void Formatter<Char>::PutPointer(StrType& ret, uintptr_t val, const FormatSpec* spec) const
{
    fmt::basic_format_specs<Char> fmtSpec = {};
    if (spec)
        fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
    fmtSpec.type = fmt::presentation_type::pointer;
    fmt::detail::write_ptr<Char>(std::back_inserter(ret), val, &fmtSpec);
}

template struct Formatter<char>;
template struct Formatter<wchar_t>;
template struct Formatter<char16_t>;
template struct Formatter<char32_t>;


template<typename Char>
static constexpr std::basic_string_view<Char> GetStrTrueFalse(bool val) noexcept
{
    if constexpr (std::is_same_v<Char, char>)
        return val ? "true"sv : "false"sv;
    else if constexpr (std::is_same_v<Char, wchar_t>)
        return val ? L"true"sv : L"false"sv;
    else if constexpr (std::is_same_v<Char, char16_t>)
        return val ? u"true"sv : u"false"sv;
    else if constexpr (std::is_same_v<Char, char32_t>)
        return val ? U"true"sv : U"false"sv;
    else
        static_assert(!AlwaysTrue<Char>);
}

template<typename Char>
static constexpr std::basic_string_view<Char> GetStrBrace(bool isLeft) noexcept
{
    if constexpr (std::is_same_v<Char, char>)
        return isLeft ?  "{"sv :  "}"sv;
    else if constexpr (std::is_same_v<Char, wchar_t>)
        return isLeft ? L"{"sv : L"}"sv;
    else if constexpr (std::is_same_v<Char, char16_t>)
        return isLeft ? u"{"sv : u"}"sv;
    else if constexpr (std::is_same_v<Char, char32_t>)
        return isLeft ? U"{"sv : U"}"sv;
    else
        static_assert(!AlwaysTrue<Char>);
}

template<typename Char>
void FormatterBase::FormatTo(const Formatter<Char>& formatter, std::basic_string<Char>& ret, const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo, const ArgPack& argPack)
{
    const uint8_t* opPtr = strInfo.Opcodes.data();
    const uint8_t* const opEnd = opPtr + strInfo.Opcodes.size();
    while (opPtr < opEnd)
    {
        const auto op       = *opPtr++;
        const auto opid     = op & ParseResult::OpIdMask;
        const auto opfield  = op & ParseResult::OpFieldMask;
        const auto opdata   = op & ParseResult::OpDataMask;
        switch (opid)
        {
        case ParseResult::BuiltinOp::Op:
        {
            switch (opfield)
            {
            case ParseResult::BuiltinOp::FieldFmtStr:
            {
                uint32_t offset = *opPtr++;
                if (opdata & ParseResult::BuiltinOp::DataOffset16)
                    offset = (offset << 8) + *opPtr++;
                uint32_t length = *opPtr++;
                if (opdata & ParseResult::BuiltinOp::DataLength16)
                    length = (length << 8) + *opPtr++;
                const auto str = strInfo.FormatString.substr(offset, length);
                formatter.PutString(ret, str, nullptr);
            } break;
            case ParseResult::BuiltinOp::FieldBrace:
            {
                switch (opdata)
                {
                case 0:  formatter.PutString(ret, GetStrBrace<Char>(true),  nullptr); break;
                case 1:  formatter.PutString(ret, GetStrBrace<Char>(false), nullptr); break;
                default: break;
                }
            } break;
            default:
                break;
            }
        } break;
        case ParseResult::ColorOp::Op:
        {
            const bool isBG = opfield & ParseResult::ColorOp::FieldBackground;
            if (opfield & ParseResult::ColorOp::FieldSpecial)
            {
                switch (opdata)
                {
                case ParseResult::ColorOp::DataDefault: 
                    formatter.PutColor(ret, isBG, {});
                    break;
                case ParseResult::ColorOp::DataBit8: 
                    formatter.PutColor(ret, isBG, *opPtr++);
                    break;
                case ParseResult::ColorOp::DataBit24:
                    formatter.PutColor(ret, isBG, *reinterpret_cast<const std::array<uint8_t, 3>*>(opPtr));
                    opPtr += 3;
                    break;
                default:
                    break;
                }
            }
            else
            {
                formatter.PutColor(ret, isBG, static_cast<common::CommonColor>(opdata));
            }
        } break;
        case ParseResult::ArgOp::Op:
        {
            const auto argIdx = *opPtr++;
            [[maybe_unused]] ArgDispType fmtType = ArgDispType::Any;
            uint32_t argSlot = UINT32_MAX;
            ArgRealType argType = ArgRealType::Error;
            if (opfield & ParseResult::ArgOp::FieldNamed)
            {
                fmtType = strInfo.NamedTypes[argIdx].Type;
                const auto mapIdx = argPack.Mapper[argIdx];
                argType = argInfo.NamedTypes[mapIdx];
                argSlot = mapIdx + argInfo.IdxArgCount;
            }
            else
            {
                fmtType = strInfo.IndexTypes[argIdx];
                argType = argInfo.IndexTypes[argIdx];
                argSlot = argIdx;
            }
            const auto realType = ArgInfo::CleanRealType(argType);
            //const auto realExtra = enum_cast(argType & ArgRealType::SizeMask8);
            const auto argPtr = argPack.Args.data() + argPack.Args[argSlot];
            std::optional<FormatSpec> spec;
            if (opfield & ParseResult::ArgOp::FieldHasSpec)
            {
                spec.emplace();
                // enum class Align : uint8_t { None, Left, Right, Middle };
                // enum class Sign  : uint8_t { None, Pos, Neg, Space };
                // uint32_t Fill       = ' ';
                // uint32_t Precision  = 0;
                // uint16_t Width      = 0;
                // Align Alignment     : 4;
                // Sign SignFlag       : 4;
                // bool AlterForm      : 4;
                // bool ZeroPad        : 4;
                const auto val0 = *opPtr++;
                const auto val1 = *opPtr++;
                spec->TypeExtra = static_cast<uint8_t>(val0 >> 4);
                spec->Alignment = static_cast<FormatSpec::Align>((val0 & 0b1100) >> 2);
                spec->SignFlag  = static_cast<FormatSpec::Sign> ((val0 & 0b0011) >> 0);
                const auto fillSize = (val1 >> 6) & 0b11u, precSize = (val1 >> 4) & 0b11u, widthSize = (val1 >> 2) & 0b11u;
                spec->AlterForm = val1 & 0b10;
                spec->ZeroPad   = val1 & 0b01;
                constexpr auto ValLenReader = [](const uint8_t* opPtr, uint32_t len, uint32_t defVal) -> uint32_t 
                {
                    switch (len)
                    {
                    case 0: return defVal;
                    case 1: return *opPtr;
                    case 2: return opPtr[0] | (opPtr[1] << 8);
                    case 3: return opPtr[0] | (opPtr[1] << 8) | (opPtr[1] << 16) | (opPtr[1] << 24);
                    default: return UINT32_MAX;
                    }
                };
                spec->Fill      = ValLenReader(opPtr, fillSize, ' ');
                opPtr += fillSize;
                spec->Precision = ValLenReader(opPtr, precSize, UINT32_MAX);
                opPtr += precSize;
                spec->Width     = static_cast<uint16_t>(ValLenReader(opPtr, widthSize, 0));
                opPtr += widthSize;
            }
            const auto specPtr = spec ? &*spec : nullptr;
            switch (realType)
            {
            case ArgRealType::String:
            {
                uintptr_t ptr = 0; 
                size_t len = SIZE_MAX;
                if (enum_cast(argType & ArgRealType::StrPtrBit))
                    ptr = *reinterpret_cast<const uintptr_t*>(argPtr);
                else
                    std::tie(ptr, len) = *reinterpret_cast<const std::pair<uintptr_t, size_t>*>(argPtr);
                switch (enum_cast(argType & ArgRealType::SizeMask4))
                {
                case 0x0:
                    formatter.PutString(ret, BuildStr<char    >(ptr, len), specPtr);
                    break;
                case 0x1:
                    formatter.PutString(ret, BuildStr<char16_t>(ptr, len), specPtr);
                    break;
                case 0x2:
                    formatter.PutString(ret, BuildStr<char32_t>(ptr, len), specPtr);
                    break;
                default:
                    break;
                }
            } break;
            case ArgRealType::Char:
            {
                switch (enum_cast(argType & ArgRealType::SizeMask4))
                {
                case 0x0:
                {
                    const auto ch = *reinterpret_cast<const char    *>(argPtr);
                    formatter.PutString(ret, std::   string_view{ &ch, 1 }, specPtr);
                } break;
                case 0x1:
                {
                    const auto ch = *reinterpret_cast<const char16_t*>(argPtr);
                    formatter.PutString(ret, std::u16string_view{ &ch, 1 }, specPtr);
                } break;
                case 0x2:
                {
                    const auto ch = *reinterpret_cast<const char32_t*>(argPtr);
                    formatter.PutString(ret, std::u32string_view{ &ch, 1 }, specPtr);
                } break;
                default:
                    break;
                }
            } break;
            case ArgRealType::SInt:
            case ArgRealType::UInt:
            {
                const auto isSigned = realType == ArgRealType::SInt;
                const auto size = enum_cast(argType & ArgRealType::SizeMask8);
                if (size <= 2) // 32bit
                {
                    uint32_t val = 0;
                    switch (size)
                    {
                    case 0x0: val = *reinterpret_cast<const uint8_t *>(argPtr); break;
                    case 0x1: val = *reinterpret_cast<const uint16_t*>(argPtr); break;
                    case 0x2: val = *reinterpret_cast<const uint32_t*>(argPtr); break;
                    default: break;
                    }
                    formatter.PutInteger(ret, val, isSigned, specPtr);
                }
                else
                {
                    const auto val = *reinterpret_cast<const uint64_t*>(argPtr);
                    formatter.PutInteger(ret, val, isSigned, specPtr);
                }
            } break;
            case ArgRealType::Float:
            {
                const auto size = enum_cast(argType & ArgRealType::SizeMask4);
                if (size <= 2) // 32bit
                {
                    const float val = size == 2 ? *reinterpret_cast<const float*>(argPtr) :
                        *reinterpret_cast<const half_float::half*>(argPtr);
                    formatter.PutFloat(ret, val, specPtr);
                }
                else
                {
                    const auto val = *reinterpret_cast<const double*>(argPtr);
                    formatter.PutFloat(ret, val, specPtr);
                }
            } break;
            case ArgRealType::Bool:
            {
                const bool val = *reinterpret_cast<const uint8_t*>(argPtr);
                formatter.PutString(ret, GetStrTrueFalse<Char>(val), specPtr);
            } break;
            case ArgRealType::Ptr:
            case ArgRealType::PtrVoid:
            {
                const auto val = *reinterpret_cast<const uintptr_t*>(argPtr);
                formatter.PutPointer(ret, val, specPtr);
            } break;
            default: break;
            }
        } break;
        default:
            break;
        }
    }
}
template SYSCOMMONTPL void FormatterBase::FormatTo(const Formatter<char>    & formatter, std::basic_string<char>    & ret, const StrArgInfoCh<char>    & strInfo, const ArgInfo& argInfo, const ArgPack& argPack);
template SYSCOMMONTPL void FormatterBase::FormatTo(const Formatter<wchar_t> & formatter, std::basic_string<wchar_t> & ret, const StrArgInfoCh<wchar_t> & strInfo, const ArgInfo& argInfo, const ArgPack& argPack);
template SYSCOMMONTPL void FormatterBase::FormatTo(const Formatter<char16_t>& formatter, std::basic_string<char16_t>& ret, const StrArgInfoCh<char16_t>& strInfo, const ArgInfo& argInfo, const ArgPack& argPack);
template SYSCOMMONTPL void FormatterBase::FormatTo(const Formatter<char32_t>& formatter, std::basic_string<char32_t>& ret, const StrArgInfoCh<char32_t>& strInfo, const ArgInfo& argInfo, const ArgPack& argPack);



}

