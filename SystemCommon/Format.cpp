#include "SystemCommonPch.h"
#include "Format.h"
#include "FormatExtra.h"
#include "StringFormat.h"
#include "StringConvert.h"
#include "StrEncodingBase.hpp"
#define HALF_ENABLE_F16C_INTRINSICS 0 // avoid platform compatibility
#include "3rdParty/half/half.hpp"
#include "3rdParty/fmt/src/format.cc"
#pragma message("Compiling SystemCommon with fmt[" STRINGIZE(FMT_VERSION) "]" )


#if COMMON_COMPILER_MSVC
#   define SYSCOMMONTPL SYSCOMMONAPI
#else
#   define SYSCOMMONTPL
#endif

#if defined(DEBUG) || defined(_DEBUG)
#   define ABORT_CHECK(...) Expects(__VA_ARGS__)
#else
#   define ABORT_CHECK(...)
#endif

namespace common::str
{
using namespace std::string_view_literals;

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4324)
#endif
template<typename Char>
struct alignas(4) OpaqueFormatSpecCh
{
    OpaqueFormatSpecBase Base;
    Char Fill[4];
    uint8_t Count;
};
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

static_assert(sizeof(fmt::basic_format_specs<    char>) == sizeof(OpaqueFormatSpecCh<    char>), "OpaqueFormatSpec size mismatch, incompatible");
static_assert(sizeof(fmt::basic_format_specs<char16_t>) == sizeof(OpaqueFormatSpecCh<char16_t>), "OpaqueFormatSpec size mismatch, incompatible");
static_assert(sizeof(fmt::basic_format_specs<char32_t>) == sizeof(OpaqueFormatSpecCh<char32_t>), "OpaqueFormatSpec size mismatch, incompatible");
static_assert(sizeof(fmt::basic_format_specs< wchar_t>) == sizeof(OpaqueFormatSpecCh< wchar_t>), "OpaqueFormatSpec size mismatch, incompatible");
static_assert(sizeof(fmt::basic_format_specs< char8_t>) == sizeof(OpaqueFormatSpecCh< char8_t>), "OpaqueFormatSpec size mismatch, incompatible");


ArgMismatchException::ExceptionInfo::ExceptionInfo(const std::u16string_view msg, Reasons reason,
    std::pair<uint8_t, uint8_t> indexArgCount, std::pair<uint8_t, uint8_t> namedArgCount) noexcept
    : TPInfo(TYPENAME, msg), Reason(reason), MismatchIndex(UINT8_MAX),
    IndexArgCount(indexArgCount), NamedArgCount(namedArgCount), ArgType{ ArgDispType::Any, ArgRealType::Error }
{ }
ArgMismatchException::ExceptionInfo::ExceptionInfo(const std::u16string_view msg,
    std::pair<uint8_t, uint8_t> indexArgCount, std::pair<uint8_t, uint8_t> namedArgCount,
    uint8_t idx, std::pair<ArgDispType, ArgRealType> types) noexcept
    : TPInfo(TYPENAME, msg), Reason(Reasons::IndexArgTypeMismatch), MismatchIndex(idx),
    IndexArgCount(indexArgCount), NamedArgCount(namedArgCount), ArgType(types)
{ }
ArgMismatchException::ExceptionInfo::ExceptionInfo(const std::u16string_view msg, 
    std::pair<uint8_t, uint8_t> indexArgCount, std::pair<uint8_t, uint8_t> namedArgCount,
    std::string&& name, std::pair<ArgDispType, ArgRealType> types) noexcept
    : TPInfo(TYPENAME, msg), Reason(Reasons::NamedArgTypeMismatch), MismatchIndex(UINT8_MAX),
    IndexArgCount(indexArgCount), NamedArgCount(namedArgCount), ArgType(types),
    MismatchName(std::move(name))
{ }
COMMON_EXCEPTION_IMPL(ArgMismatchException)


void ParseResult::CheckErrorRuntime(uint16_t errorPos, uint16_t opCount)
{
    std::u16string_view errMsg;
    if (errorPos != UINT16_MAX)
    {
        switch (static_cast<ErrorCode>(opCount))
        {
#define CHECK_ERROR_MSG(e, msg) case ErrorCode::e: errMsg = u ## msg; break;
            SCSTR_HANDLE_PARSE_ERROR(CHECK_ERROR_MSG)
#undef CHECK_ERROR_MSG
        default: errMsg = u"Unknown internal error"sv; break;
        }
        COMMON_THROW(BaseException, fmt::format(FMT_STRING(u"{} at [{}]"sv), errMsg, errorPos));
    }
}


DynamicTrimedResult::DynamicTrimedResult(const ParseResult& result, size_t strLength, size_t charSize) :
    StrSize(static_cast<uint32_t>((strLength + 1) * charSize)), OpCount(result.OpCount), NamedArgCount(result.NamedArgCount), IdxArgCount(result.IdxArgCount)
{
    static_assert(NASize == sizeof(uint32_t));
    static_assert(IASize == sizeof(uint8_t));
    Expects(OpCount > 0);
    Expects(StrSize > 0);
    ParseResult::CheckErrorRuntime(result.ErrorPos, result.OpCount);
    const auto naSize = NASize * NamedArgCount;
    const auto iaSize = IASize * IdxArgCount;
    const auto needLength = (naSize + StrSize + IdxArgCount + OpCount + sizeof(uint32_t) - 1) / sizeof(uint32_t);
    const auto space = FixedLenRefHolder<DynamicTrimedResult, uint32_t>::Allocate(needLength);
    Pointer = reinterpret_cast<uintptr_t>(space.data());
    if (NamedArgCount)
        memcpy_s(reinterpret_cast<void*>(Pointer), naSize, result.NamedTypes, naSize);
    if (IdxArgCount)
        memcpy_s(reinterpret_cast<void*>(Pointer + naSize + StrSize), iaSize, result.IndexTypes, iaSize);
    memcpy_s(reinterpret_cast<void*>(Pointer + naSize + StrSize + iaSize), OpCount, result.Opcodes, OpCount);
}


void ArgChecker::CheckDDBasic(const StrArgInfo& strInfo, const ArgInfo& argInfo)
{
    const auto strIndexArgCount = static_cast<uint8_t>(strInfo.IndexTypes.size());
    const auto strNamedArgCount = static_cast<uint8_t>(strInfo.NamedTypes.size());
    if (argInfo.IdxArgCount < strIndexArgCount)
    {
        COMMON_THROW(ArgMismatchException, fmt::format(u"No enough indexed arg, expects [{}], has [{}]"sv,
            strIndexArgCount, argInfo.IdxArgCount), ArgMismatchException::Reasons::TooLessIndexArg,
            std::pair{ argInfo.IdxArgCount, strIndexArgCount }, std::pair{ argInfo.NamedArgCount, strNamedArgCount });
    }
    if (argInfo.NamedArgCount < strNamedArgCount)
    {
        COMMON_THROW(ArgMismatchException, fmt::format(u"No enough named arg, expects [{}], has [{}]"sv,
            strNamedArgCount, argInfo.NamedArgCount), ArgMismatchException::Reasons::TooLessNamedArg,
            std::pair{ argInfo.IdxArgCount, strIndexArgCount }, std::pair{ argInfo.NamedArgCount, strNamedArgCount });
    }
    if (strIndexArgCount > 0)
    {
        const auto index = ArgChecker::GetIdxArgMismatch(strInfo.IndexTypes.data(), argInfo.IndexTypes, strIndexArgCount);
        if (index != ParseResult::IdxArgSlots)
            COMMON_THROW(ArgMismatchException, fmt::format(u"IndexArg[{}] type mismatch"sv, index),
                std::pair{ argInfo.IdxArgCount, strIndexArgCount }, std::pair{ argInfo.NamedArgCount, strNamedArgCount },
                static_cast<uint8_t>(index), std::pair{ strInfo.IndexTypes[index], argInfo.IndexTypes[index] & ArgRealType::BaseTypeMask });
    }
}

template<typename Char>
NamedMapper ArgChecker::CheckDDNamedArg(const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo)
{
    NamedMapper mapper = { 0 };
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
            std::u16string errMsg = u"NamedArg [";
            errMsg.append(name.begin(), name.end());
            if (namedRet.GiveIndex) // type mismatch
            {
                errMsg.append(u"] type mismatch"sv);
                COMMON_THROW(ArgMismatchException, errMsg,
                    std::pair{ argInfo.IdxArgCount, static_cast<uint8_t>(strInfo.IndexTypes.size()) }, std::pair{ argInfo.NamedArgCount, strNamedArgCount },
                    std::move(name), std::pair{namedArg.Type, argInfo.NamedTypes[*namedRet.GiveIndex] & ArgRealType::BaseTypeMask});
            }
            else // arg not found
            {
                errMsg.append(u"] not found in args"sv);
                COMMON_THROW(ArgMismatchException, errMsg,
                    std::pair{ argInfo.IdxArgCount, static_cast<uint8_t>(strInfo.IndexTypes.size()) }, std::pair{ argInfo.NamedArgCount, strNamedArgCount },
                    std::move(name), std::pair{ namedArg.Type, ArgRealType::Error });
            }
        }
        Ensures(!namedRet.GiveIndex);
        mapper = namedRet.Mapper;
    }
    return mapper;
}
template SYSCOMMONTPL NamedMapper ArgChecker::CheckDDNamedArg(const StrArgInfoCh<char>&     strInfo, const ArgInfo& argInfo);
template SYSCOMMONTPL NamedMapper ArgChecker::CheckDDNamedArg(const StrArgInfoCh<wchar_t>&  strInfo, const ArgInfo& argInfo);
template SYSCOMMONTPL NamedMapper ArgChecker::CheckDDNamedArg(const StrArgInfoCh<char16_t>& strInfo, const ArgInfo& argInfo);
template SYSCOMMONTPL NamedMapper ArgChecker::CheckDDNamedArg(const StrArgInfoCh<char32_t>& strInfo, const ArgInfo& argInfo);
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template SYSCOMMONTPL NamedMapper ArgChecker::CheckDDNamedArg(const StrArgInfoCh<char8_t> & strInfo, const ArgInfo& argInfo);
#endif


void FormatterExecutor::OnBrace(Context& context, bool isLeft)
{
    PutString(context, isLeft ? "{"sv : "}"sv, nullptr);
}
void FormatterExecutor::OnColor(Context&, ScreenColor)
{
}


struct FormatterHelper : public FormatterBase
{
    template<typename Char, typename T>
    static forceinline void ConvertSpecNoFill(fmt::basic_format_specs<Char>& spec, const T& in) noexcept
    {
        spec.width = in.Width;
        spec.precision = static_cast<int32_t>(in.Precision);
        spec.alt = in.AlterForm;
        spec.localized = false;
        //spec.sign = fmt::sign::none;
        if (in.ZeroPad)
            spec.align = fmt::align::numeric;
        else
        {
            switch (in.Alignment)
            {
            default:
            case FormatSpec::Align::None:   spec.align = fmt::align::none;   break;
            case FormatSpec::Align::Left:   spec.align = fmt::align::left;   break;
            case FormatSpec::Align::Middle: spec.align = fmt::align::center; break;
            case FormatSpec::Align::Right:  spec.align = fmt::align::right;  break;
            }
        }
    }

    template<typename T>
    static forceinline constexpr auto ConvertFill(char32_t ch) noexcept
    {
        std::array<typename T::ElementType, T::MaxOutputUnit> tmp;
        const auto count = T::To(ch, T::MaxOutputUnit, tmp.data());
        return std::pair{ tmp, count };
    }
    template<typename Char>
    static forceinline void ConvertSpecFill(fmt::basic_format_specs<Char>& spec, const FormatSpec& in) noexcept
    {
        if (in.ZeroPad)
        {
            spec.fill[0] = static_cast<Char>('0');
        }
        else if (in.Fill != ' ') // ignore space
        {
            if constexpr (sizeof(Char) == 4)
            {
                spec.fill[0] = static_cast<Char>(in.Fill);
            }
            else if constexpr (sizeof(Char) == 2)
            {
                const auto [tmp, count] = ConvertFill<charset::detail::UTF16>(static_cast<char32_t>(in.Fill));
                spec.fill = std::basic_string_view<Char>(reinterpret_cast<const Char*>(tmp.data()), count);
            }
            else if constexpr (sizeof(Char) == 1)
            {
                const auto [tmp, count] = ConvertFill<charset::detail::UTF8>(static_cast<char32_t>(in.Fill));
                spec.fill = std::basic_string_view<Char>(reinterpret_cast<const Char*>(tmp.data()), count);
            }
            else
                static_assert(!AlwaysTrue<Char>);
        }
    }
    static forceinline void ConvertSpecFill(OpaqueFormatSpec& spec, bool zeroPad, char32_t fill) noexcept
    {
        if (zeroPad)
        {
            spec.Fill32[0] = U'0';
            spec.Fill16[0] = u'0';
            spec.Fill8 [0] =  '0';
            spec.Count32 = spec.Count16 = spec.Count8;
        }
        else if (fill == ' ') // fast pass
        {
            spec.Fill32[0] = U' ';
            spec.Fill16[0] = u' ';
            spec.Fill8 [0] =  ' ';
            spec.Count32 = spec.Count16 = spec.Count8;
        }
        else
        {
            spec.Fill32[0] = fill;
            spec.Count32 = 1;
            {
                const auto [tmp, count] = FormatterHelper::ConvertFill<charset::detail::UTF16>(fill);
                for (size_t i = 0; i < tmp.size(); ++i)
                    spec.Fill16[i] = tmp[i];
                spec.Count16 = count;
            }
            {
                const auto [tmp, count] = FormatterHelper::ConvertFill<charset::detail::UTF8>(fill);
                for (size_t i = 0; i < tmp.size(); ++i)
                    spec.Fill8[i] = tmp[i];
                spec.Count8 = count;
            }
        }
    }

    template<typename Char>
    static forceinline fmt::basic_format_specs<Char> ConvertSpecBasic(const FormatSpec& in) noexcept
    {
        fmt::basic_format_specs<Char> spec;
        ConvertSpecNoFill(spec, in);
        ConvertSpecFill(spec, in);
        return spec;
    }
    static forceinline constexpr fmt::sign_t ConvertSpecSign(const FormatSpec::Sign sign) noexcept
    {
        switch (sign)
        {
        default:
        case FormatSpec::Sign::None:  return fmt::sign::none;
        case FormatSpec::Sign::Neg:   return fmt::sign::minus;
        case FormatSpec::Sign::Pos:   return fmt::sign::plus;
        case FormatSpec::Sign::Space: return fmt::sign::space;
        }
    }
    static constexpr uint32_t IntPrefixes[4] = { 0, 0, 0x1000000u | '+', 0x1000000u | ' ' };
    static forceinline constexpr uint32_t ConvertSpecIntSign(FormatSpec::Sign sign) noexcept
    {
        switch (sign)
        {
        default:
        case FormatSpec::Sign::None:  return IntPrefixes[fmt::sign::none];
        case FormatSpec::Sign::Neg:   return IntPrefixes[fmt::sign::minus];
        case FormatSpec::Sign::Pos:   return IntPrefixes[fmt::sign::plus];
        case FormatSpec::Sign::Space: return IntPrefixes[fmt::sign::space];
        }
    }
    static forceinline constexpr uint32_t ConvertSpecIntSign(fmt::sign_t sign) noexcept
    {
        return IntPrefixes[sign];
    }
    
    static constexpr fmt::presentation_type IntTypes[] =
    { // dbBoxX
        fmt::presentation_type::dec,
        fmt::presentation_type::bin_lower,
        fmt::presentation_type::bin_upper,
        fmt::presentation_type::oct,
        fmt::presentation_type::hex_lower,
        fmt::presentation_type::hex_upper,
    };
    static forceinline constexpr fmt::presentation_type ConvertSpecIntPresent(const FormatSpec& spec) noexcept
    {
        return IntTypes[spec.TypeExtra];
    }
    static forceinline constexpr fmt::presentation_type ConvertSpecIntPresent(const ParseResult::FormatSpec& spec) noexcept
    {
        return IntTypes[spec.Type.Extra];
    }
    
    static constexpr fmt::presentation_type FloatTypes[] =
    { // gGaAeEfF
        fmt::presentation_type::general_lower,
        fmt::presentation_type::general_upper,
        fmt::presentation_type::hexfloat_lower,
        fmt::presentation_type::hexfloat_upper,
        fmt::presentation_type::exp_lower,
        fmt::presentation_type::exp_upper,
        fmt::presentation_type::fixed_lower,
        fmt::presentation_type::fixed_upper,
    };
    static forceinline constexpr fmt::presentation_type ConvertSpecFloatPresent(const FormatSpec& spec) noexcept
    {
        return FloatTypes[spec.TypeExtra];
    }
    static forceinline constexpr fmt::presentation_type ConvertSpecFloatPresent(const ParseResult::FormatSpec& spec) noexcept
    {
        return FloatTypes[spec.Type.Extra];
    }

    template<typename T, typename U>
    static forceinline uint32_t ProcessIntSign(T& val, bool isSigned, const U sign) noexcept
    {
        using S = std::conditional_t<std::is_same_v<T, uint32_t>, int32_t, int64_t>;
        if (const auto signedVal = static_cast<S>(val); isSigned && signedVal < 0)
        {
            val = static_cast<T>(S(0) - signedVal);
            return 0x01000000 | '-';
        }
        else
        {
            return ConvertSpecIntSign(sign);
        }
    }
    template<typename Dst, typename Src>
    static void PutString(std::basic_string<Dst>& ret, std::basic_string_view<Src> str, const fmt::basic_format_specs<Dst>* __restrict spec)
    {
        if constexpr (std::is_same_v<Dst, Src>)
        {
            if (spec)
            {
                /*auto fmtSpec = ConvertSpecBasic<Dst>(*spec);
                fmtSpec.type = fmt::presentation_type::string;*/
                fmt::detail::write<Dst>(std::back_inserter(ret), str, *spec);
            }
            else
                ret.append(str);
        }
        else if constexpr (sizeof(Dst) == sizeof(Src))
        {
            PutString<Dst, Dst>(ret, std::basic_string_view<Dst>{ reinterpret_cast<const Dst*>(str.data()), str.size() }, spec);
        }
        else if constexpr (std::is_same_v<Dst, char>)
        {
            PutString<char, char>(ret, str::to_string(str, Encoding::UTF8), spec);
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
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
        else if constexpr (std::is_same_v<Dst, char8_t>)
        {
            PutString<char8_t, char8_t>(ret, str::to_u8string(str), spec);
        }
#endif
        else
            static_assert(!common::AlwaysTrue<Src>, "not supported");
    }
};


bool FormatterExecutor::ConvertSpec(OpaqueFormatSpec& dst, const FormatSpec* src, ArgRealType real, ArgDispType disp) noexcept
{
    if (!ArgChecker::CheckCompatible(disp, real))
        return false;
    if (src)
        FormatterHelper::ConvertSpecFill(dst, src->ZeroPad, static_cast<char32_t>(src->Fill));
    else
        FormatterHelper::ConvertSpecFill(dst, false, U' ');

    fmt::basic_format_specs<char> spec{};
    if (src)
        FormatterHelper::ConvertSpecNoFill(spec, *src);
    switch (const auto realType = real & ArgRealType::BaseTypeMask; realType)
    {
    case ArgRealType::String:
        if (disp == ArgDispType::Pointer)
            spec.type = fmt::presentation_type::pointer;
        else
            spec.type = fmt::presentation_type::string;
        break;
    case ArgRealType::Bool:
        if (disp == ArgDispType::Char)
            spec.type = fmt::presentation_type::string;
        else if (disp == ArgDispType::Char)
        {
            if (src)
                spec.type = FormatterHelper::ConvertSpecIntPresent(*src);
        }
        else
            spec.type = fmt::presentation_type::string;
        break;
    case ArgRealType::Ptr:
        spec.type = fmt::presentation_type::pointer;
        break;
    case ArgRealType::Float:
        if (src)
        {
            spec.type = FormatterHelper::ConvertSpecFloatPresent(*src);
            spec.sign = FormatterHelper::ConvertSpecSign(src->SignFlag);
        }
        break;
    case ArgRealType::Char:
        if (disp == ArgDispType::Char || disp == ArgDispType::Any)
        {
            spec.type = fmt::presentation_type::string;
            break;
        }
        [[fallthrough]];
    case ArgRealType::SInt:
    case ArgRealType::UInt:
        ABORT_CHECK(disp == ArgDispType::Integer || disp == ArgDispType::Any);
        if (src)
            spec.type = FormatterHelper::ConvertSpecIntPresent(*src);
        break;
    default:
        return false;
    }
    memcpy_s(&dst.Base, sizeof(OpaqueFormatSpecBase), &spec, sizeof(OpaqueFormatSpecBase));
    return true;
}

bool FormatterExecutor::ConvertSpec(OpaqueFormatSpec& dst, std::u32string_view spectxt, ArgRealType real) noexcept
{
    ParseResult res;
    ParseResult::FormatSpec spec_;
    ParseResultCh<char32_t>::ParseFormatSpec(res, spec_, spectxt.data(), spectxt);
    if (res.ErrorPos != UINT16_MAX)
        return false;
    const auto disp = spec_.Type.Type;
    if (!ArgChecker::CheckCompatible(disp, real))
        return false;

    FormatterHelper::ConvertSpecFill(dst, spec_.ZeroPad, static_cast<char32_t>(spec_.Fill));

    fmt::basic_format_specs<char> spec{};
    FormatterHelper::ConvertSpecNoFill(spec, spec_);
    switch (const auto realType = real & ArgRealType::BaseTypeMask; realType)
    {
    case ArgRealType::String:
        if (disp == ArgDispType::Pointer)
            spec.type = fmt::presentation_type::pointer;
        else
            spec.type = fmt::presentation_type::string;
        break;
    case ArgRealType::Bool:
        if (disp == ArgDispType::Char)
            spec.type = fmt::presentation_type::string;
        else if (disp == ArgDispType::Char)
            spec.type = FormatterHelper::ConvertSpecIntPresent(spec_);
        else
            spec.type = fmt::presentation_type::string;
        break;
    case ArgRealType::Ptr:
        spec.type = fmt::presentation_type::pointer;
        break;
    case ArgRealType::Float:
        spec.type = FormatterHelper::ConvertSpecFloatPresent(spec_);
        spec.sign = FormatterHelper::ConvertSpecSign(spec_.SignFlag);
        break;
    case ArgRealType::Char:
        if (disp == ArgDispType::Char || disp == ArgDispType::Any)
        {
            spec.type = fmt::presentation_type::string;
            break;
        }
        [[fallthrough]];
    case ArgRealType::SInt:
    case ArgRealType::UInt:
        ABORT_CHECK(disp == ArgDispType::Integer || disp == ArgDispType::Any);
        spec.type = FormatterHelper::ConvertSpecIntPresent(spec_);
        break;
    default:
        return false;
    }
    memcpy_s(&dst.Base, sizeof(OpaqueFormatSpecBase), &spec, sizeof(OpaqueFormatSpecBase));
    return true;
}


template<typename Char>
struct alignas(4) WrapSpec
{
    using T = fmt::basic_format_specs<Char>;
    uint8_t Data[sizeof(T)] = { 0 };
    T& Get() noexcept
    {
        return *reinterpret_cast<T*>(Data);
    }
    WrapSpec(const OpaqueFormatSpec& spec) noexcept
    {
        memcpy_s(&Data, sizeof(OpaqueFormatSpecBase), &spec.Base, sizeof(OpaqueFormatSpecBase));
        auto& dst = Get();
        if constexpr (sizeof(Char) == 4)
        {
            dst.fill = std::basic_string_view<Char>{ reinterpret_cast<const Char*>(&spec.Fill32), spec.Count32 };
        }
        else if constexpr (sizeof(Char) == 2)
        {
            dst.fill = std::basic_string_view<Char>{ reinterpret_cast<const Char*>(&spec.Fill16), spec.Count16 };
        }
        else if constexpr (sizeof(Char) == 1)
        {
            dst.fill = std::basic_string_view<Char>{ reinterpret_cast<const Char*>(&spec.Fill8), spec.Count8 };
        }
        else
            static_assert(!AlwaysTrue<Char>);
    }
};


template<typename Char>
void CommonFormatter<Char>::PutString(StrType& ret, std::   string_view str, const OpaqueFormatSpec* spec)
{
    if (spec)
    {
        WrapSpec<Char> fmtSpec(*spec);
        FormatterHelper::PutString(ret, str, &fmtSpec.Get());
    }
    else
        FormatterHelper::PutString<Char>(ret, str, nullptr);
}
template<typename Char>
void CommonFormatter<Char>::PutString(StrType& ret, std::  wstring_view str, const OpaqueFormatSpec* spec)
{
    if (spec)
    {
        WrapSpec<Char> fmtSpec(*spec);
        FormatterHelper::PutString(ret, str, &fmtSpec.Get());
    }
    else
        FormatterHelper::PutString<Char>(ret, str, nullptr);
}
template<typename Char>
void CommonFormatter<Char>::PutString(StrType& ret, std::u16string_view str, const OpaqueFormatSpec* spec)
{
    if (spec)
    {
        WrapSpec<Char> fmtSpec(*spec);
        FormatterHelper::PutString(ret, str, &fmtSpec.Get());
    }
    else
        FormatterHelper::PutString<Char>(ret, str, nullptr);
}
template<typename Char>
void CommonFormatter<Char>::PutString(StrType& ret, std::u32string_view str, const OpaqueFormatSpec* spec)
{
    if (spec)
    {
        WrapSpec<Char> fmtSpec(*spec);
        FormatterHelper::PutString(ret, str, &fmtSpec.Get());
    }
    else
        FormatterHelper::PutString<Char>(ret, str, nullptr);
}

template<typename Char>
void CommonFormatter<Char>::PutInteger(StrType& ret, uint32_t val, bool isSigned, const OpaqueFormatSpec& spec)
{
    WrapSpec<Char> spec_(spec);
    const auto& fmtSpec = spec_.Get();
    const auto prefix = FormatterHelper::ProcessIntSign(val, isSigned, fmtSpec.sign);
    const fmt::detail::write_int_arg<uint32_t> arg{ val, prefix };
    fmt::detail::write_int(std::back_inserter(ret), arg, fmtSpec, {});
}
template<typename Char>
void CommonFormatter<Char>::PutInteger(StrType& ret, uint64_t val, bool isSigned, const OpaqueFormatSpec& spec)
{
    WrapSpec<Char> spec_(spec);
    const auto& fmtSpec = spec_.Get();
    const auto prefix = FormatterHelper::ProcessIntSign(val, isSigned, fmtSpec.sign);
    const fmt::detail::write_int_arg<uint64_t> arg{ val, prefix };
    fmt::detail::write_int(std::back_inserter(ret), arg, fmtSpec, {});
}

template<typename Char>
void CommonFormatter<Char>::PutFloat(StrType& ret, float  val, const OpaqueFormatSpec& spec)
{
    WrapSpec<Char> spec_(spec);
    const auto& fmtSpec = spec_.Get();
    fmt::detail::write(std::back_inserter(ret), val, fmtSpec, {});
}
template<typename Char>
void CommonFormatter<Char>::PutFloat(StrType& ret, double val, const OpaqueFormatSpec& spec)
{
    WrapSpec<Char> spec_(spec);
    const auto& fmtSpec = spec_.Get();
    fmt::detail::write(std::back_inserter(ret), val, fmtSpec, {});
}

template<typename Char>
void CommonFormatter<Char>::PutPointer(StrType& ret, uintptr_t val, const OpaqueFormatSpec& spec)
{
    WrapSpec<Char> spec_(spec);
    const auto& fmtSpec = spec_.Get();
    fmt::detail::write_ptr<Char>(std::back_inserter(ret), val, &fmtSpec);
}

template struct CommonFormatter<char>;
template struct CommonFormatter<wchar_t>;
template struct CommonFormatter<char16_t>;
template struct CommonFormatter<char32_t>;
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template struct CommonFormatter<char8_t>;
#endif


template<typename Char>
void Formatter<Char>::PutColor(StrType&, ScreenColor) { }


template<typename Char>
inline void Formatter<Char>::PutColorArg(StrType& ret, ScreenColor color, const FormatSpec* spec)
{
    const bool shouldColor = spec && (spec->Alignment == FormatSpec::Align::Middle || spec->Alignment == FormatSpec::Align::Right);
    if (shouldColor)
        PutColor(ret, color);
    const bool shouldPrint = !spec || (spec->Alignment != FormatSpec::Align::Right);
    if (shouldPrint)
    {
        switch (color.Type)
        {
        case ScreenColor::ColorType::Common:
            PutString(ret, GetColorName(static_cast<CommonColor>(color.Value[0])), nullptr);
            break;
        case ScreenColor::ColorType::Bit8:
            color = Expend256ColorToRGB(color.Value[0]);
            // it's now 24bit
            [[fallthrough]];
        case ScreenColor::ColorType::Bit24:
            if (spec && spec->AlterForm)
            {
                ret.push_back('#');
                fmt::basic_format_specs<Char> fmtSpec = {};
                fmtSpec.type = fmt::presentation_type::hex_lower;
                fmtSpec.fill[0] = static_cast<Char>('0');
                fmtSpec.width = 6;
                const uint32_t color24 = (color.Value[0] << 0) | (color.Value[1] << 8) | (color.Value[2] << 16);
                fmt::detail::write_int_arg<uint32_t> arg{ color24, 0 };
                fmt::detail::write_int(std::back_inserter(ret), arg, fmtSpec, {});
            }
            else
            {
                char head[] = "rgb(";
                ret.append(&head[0], &head[4]);
                fmt::basic_format_specs<Char> fmtSpec = {};
                fmtSpec.type = fmt::presentation_type::dec;
                fmt::detail::write_int_arg<uint32_t> arg{ 0, 0 };

                arg.abs_value = color.Value[0];
                fmt::detail::write_int(std::back_inserter(ret), arg, fmtSpec, {});
                ret.push_back(',');

                arg.abs_value = color.Value[1];
                fmt::detail::write_int(std::back_inserter(ret), arg, fmtSpec, {});
                ret.push_back(',');

                arg.abs_value = color.Value[2];
                fmt::detail::write_int(std::back_inserter(ret), arg, fmtSpec, {});
                ret.push_back(')');
            }
            break;
        case ScreenColor::ColorType::Default:
        default:
            break;
        }
    }
}

template<typename Char>
void Formatter<Char>::PutString(StrType& ret, std::   string_view str, const FormatSpec* spec)
{
    if (spec)
    {
        auto fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
        fmtSpec.type = fmt::presentation_type::string;
        FormatterHelper::PutString(ret, str, &fmtSpec);
    }
    else
        FormatterHelper::PutString<Char>(ret, str, nullptr);
}
template<typename Char>
void Formatter<Char>::PutString(StrType& ret, std::  wstring_view str, const FormatSpec* spec)
{
    if (spec)
    {
        auto fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
        fmtSpec.type = fmt::presentation_type::string;
        FormatterHelper::PutString(ret, str, &fmtSpec);
    }
    else
        FormatterHelper::PutString<Char>(ret, str, nullptr);
}
template<typename Char>
void Formatter<Char>::PutString(StrType& ret, std::u16string_view str, const FormatSpec* spec)
{
    if (spec)
    {
        auto fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
        fmtSpec.type = fmt::presentation_type::string;
        FormatterHelper::PutString(ret, str, &fmtSpec);
    }
    else
        FormatterHelper::PutString<Char>(ret, str, nullptr);
}
template<typename Char>
void Formatter<Char>::PutString(StrType& ret, std::u32string_view str, const FormatSpec* spec)
{
    if (spec)
    {
        auto fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
        fmtSpec.type = fmt::presentation_type::string;
        FormatterHelper::PutString(ret, str, &fmtSpec);
    }
    else
        FormatterHelper::PutString<Char>(ret, str, nullptr);
}

template<typename Char>
void Formatter<Char>::PutInteger(StrType& ret, uint32_t val, bool isSigned, const FormatSpec* spec)
{
    fmt::basic_format_specs<Char> fmtSpec = {};
    if (spec)
    {
        fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
        fmtSpec.type = FormatterHelper::ConvertSpecIntPresent(*spec);
    }
    const auto prefix = FormatterHelper::ProcessIntSign(val, isSigned, spec ? spec->SignFlag : FormatSpec::Sign::None);
    const fmt::detail::write_int_arg<uint32_t> arg{ val, prefix };
    fmt::detail::write_int(std::back_inserter(ret), arg, fmtSpec, {});
}
template<typename Char>
void Formatter<Char>::PutInteger(StrType& ret, uint64_t val, bool isSigned, const FormatSpec* spec)
{
    fmt::basic_format_specs<Char> fmtSpec = {};
    if (spec)
    {
        fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
        fmtSpec.type = FormatterHelper::ConvertSpecIntPresent(*spec);
    }
    const auto prefix = FormatterHelper::ProcessIntSign(val, isSigned, spec ? spec->SignFlag : FormatSpec::Sign::None);
    const fmt::detail::write_int_arg<uint64_t> arg{ val, prefix };
    fmt::detail::write_int(std::back_inserter(ret), arg, fmtSpec, {});
}

template<typename Char>
void Formatter<Char>::PutFloat(StrType& ret, float val, const FormatSpec* spec)
{
    fmt::basic_format_specs<Char> fmtSpec = {};
    if (spec)
    {
        fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
        fmtSpec.type = FormatterHelper::ConvertSpecFloatPresent(*spec);
        fmtSpec.sign = FormatterHelper::ConvertSpecSign(spec->SignFlag);
    }
    fmt::detail::write(std::back_inserter(ret), val, fmtSpec, {});
}
template<typename Char>
void Formatter<Char>::PutFloat(StrType& ret, double val, const FormatSpec* spec)
{
    fmt::basic_format_specs<Char> fmtSpec = {};
    if (spec)
    {
        fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
        fmtSpec.type = FormatterHelper::ConvertSpecFloatPresent(*spec);
        fmtSpec.sign = FormatterHelper::ConvertSpecSign(spec->SignFlag);
    }
    fmt::detail::write(std::back_inserter(ret), val, fmtSpec, {});
}

template<typename Char>
void Formatter<Char>::PutPointer(StrType& ret, uintptr_t val, const FormatSpec* spec)
{
    fmt::basic_format_specs<Char> fmtSpec = {};
    if (spec)
        fmtSpec = FormatterHelper::ConvertSpecBasic<Char>(*spec);
    fmtSpec.type = fmt::presentation_type::pointer;
    fmt::detail::write_ptr<Char>(std::back_inserter(ret), val, &fmtSpec);
}

template<typename Char>
constexpr auto GetDateStr() noexcept
{
    if constexpr (std::is_same_v<Char, char>)
        return "%Y-%m-%dT%H:%M:%S"sv;
    else if constexpr (std::is_same_v<Char, char16_t>)
        return u"%Y-%m-%dT%H:%M:%S"sv;
    else if constexpr (std::is_same_v<Char, char32_t>)
        return U"%Y-%m-%dT%H:%M:%S"sv;
    else if constexpr (std::is_same_v<Char, wchar_t>)
        return L"%Y-%m-%dT%H:%M:%S"sv;
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    else if constexpr (std::is_same_v<Char, char8_t>)
        return u8"%Y-%m-%dT%H:%M:%S"sv;
#endif
    else
        return ""sv;
}

template<typename Char>
inline void PutDate_(std::basic_string<Char>& ret, std::basic_string_view<Char> fmtStr, const std::tm& date)
{
    auto ins = std::back_inserter(ret);
    if (fmtStr.empty())
        fmtStr = GetDateStr<Char>();
    fmt::detail::tm_writer<decltype(ins), Char> writer({}, ins, date);
    fmt::detail::parse_chrono_format(fmtStr.data(), fmtStr.data() + fmtStr.size(), writer);
}

template<typename Char>
void Formatter<Char>::PutDate(StrType& ret, std::basic_string_view<Char> fmtStr, const std::tm& date)
{
#if COMMON_OS_ANDROID || COMMON_OS_IOS // android's std::put_time is limited to char/wchar_t
    if constexpr (std::is_same_v<Char, char> || std::is_same_v<Char, wchar_t>)
    {
        PutDate_(ret, fmtStr, date);
    }
    else if constexpr (sizeof(Char) == sizeof(wchar_t))
    {
        PutDate_(*reinterpret_cast<std::wstring*>(&ret), *reinterpret_cast<std::wstring_view*>(&fmtStr), date);
    }
    else if constexpr (sizeof(Char) == sizeof(char))
    {
        PutDate_(*reinterpret_cast<std::string*>(&ret), *reinterpret_cast<std::string_view*>(&fmtStr), date);
    }
    else
    {
        const auto fmtStr_ = to_string(fmtStr, Encoding::UTF8);
        std::string tmp;
        PutDate_<char>(tmp, fmtStr_, date);
        if constexpr (std::is_same_v<Char, char16_t>)
            ret.append(to_u16string(tmp, Encoding::UTF8));
        else
            ret.append(to_u32string(tmp, Encoding::UTF8));
    }
#else
    PutDate_(ret, fmtStr, date);
#endif
}
template<typename Char>
void Formatter<Char>::PutDateBase(StrType& ret, std::string_view fmtStr, const std::tm& date)
{
    if constexpr (sizeof(Char) == sizeof(char))
    {
        PutDate_(*reinterpret_cast<std::string*>(&ret), fmtStr, date);
    }
    else
    {
        std::string tmp;
        PutDate_<char>(tmp, fmtStr, date);
        if constexpr (sizeof(Char) == sizeof(char16_t))
        {
            const auto tmp2 = to_u16string(tmp, Encoding::UTF8);
            ret.append(reinterpret_cast<const Char*>(tmp2.data()), tmp2.size());
        }
        else
        {
            const auto tmp2 = to_u32string(tmp, Encoding::UTF8);
            ret.append(reinterpret_cast<const Char*>(tmp2.data()), tmp2.size());
        }
    }
}

template<typename Char>
void Formatter<Char>::FormatToDynamic_(std::basic_string<Char>& dst, std::basic_string_view<Char> format, const ArgInfo& argInfo, span<const uint16_t> argStore)
{
    const auto result = ParseResult::ParseString<Char>(format);
    ParseResult::CheckErrorRuntime(result.ErrorPos, result.OpCount);
    const auto res = result.ToInfo(format);
    const auto mapping = ArgChecker::CheckDD(res, argInfo);
    FormatterBase::FormatTo<Char>(*this, dst, res, argInfo, argStore, mapping);
}


template struct Formatter<char>;
template struct Formatter<wchar_t>;
template struct Formatter<char16_t>;
template struct Formatter<char32_t>;
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template struct Formatter<char8_t>;
#endif


#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template<typename Char>
using SimChar = std::conditional_t<std::is_same_v<Char, char8_t>, char, Char>;
#else
template<typename Char>
using SimChar = Char;
#endif


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


constexpr auto ShortLenMap = []() 
{
    std::array<uint8_t, 64> ret = { 0 };
    for (uint32_t fill = 0; fill <= 3; fill++)
    {
        for (uint32_t prec = 0; prec <= 3; prec++)
        {
            for (uint32_t width = 0; width <= 3; width++)
            {
                ret[width + (prec << 2) + (fill << 4)] = static_cast<uint8_t>(
                    (width == 3 ? 4 : width) + (prec == 3 ? 4 : prec) + (fill == 3 ? 4 : fill));
            }
        }
    }
    return ret;
}();
[[nodiscard]] forceinline constexpr uint32_t SpecReader::EnsureSize() noexcept
{
    if (Ptr && !SpecSize)
    {
        const auto val0 = Ptr[0];
        const auto val1 = Ptr[1];
        SpecSize = 2;
        if (val1 & 0b11111100)
        {
            SpecSize += ShortLenMap[val1 >> 2];
        }
        const bool hasExtraFmt = val0 & 0b10000;
        if (hasExtraFmt)
        {
            SpecSize += val0 & 0x80 ? 2 : 1;
            SpecSize += val0 & 0x40 ? 2 : 1;
        }
    }
    return SpecSize;
}
struct SpecReader::Checker
{
#if defined(DEBUG) || defined(_DEBUG)
    static constexpr uint32_t SpecReaderCheckResult = []() -> uint32_t
    {
        uint8_t Dummy[32] = { 0 };
        SpecReader reader;
        for (uint16_t i = 0; i < 0xff; i += 0b10000)
        {
            for (uint16_t j = 0; j < 0xff; j += 0b100)
            {
                Dummy[0] = static_cast<uint8_t>(i);
                Dummy[1] = static_cast<uint8_t>(j);
                reader.Reset(Dummy);
                const auto quickSize = reader.EnsureSize();
                reader.Reset(Dummy);
                [[maybe_unused]] const auto dummy = reader.ReadSpec();
                const auto readSize = reader.EnsureSize();
                if (quickSize != readSize)
                    return i * 256 + j;
            }
        }
        return UINT32_MAX;
    }();
    static_assert(SpecReaderCheckResult == UINT32_MAX);
#endif
};


template<typename T>
forceinline uint32_t FormatterBase::Execute(span<const uint8_t>& opcodes, T& executor, typename T::Context& context, uint32_t instCount)
{
    uint32_t icnt = 0; 
    uint32_t opOffset = 0;
    SpecReader reader; // outside of loop for reuse
    for (; icnt < instCount && opOffset < opcodes.size(); icnt++)
    {
        const auto op = opcodes[opOffset++];
        const auto opid = op & ParseResult::OpIdMask;
        const auto opfield = op & ParseResult::OpFieldMask;
        const auto opdata = op & ParseResult::OpDataMask;
        switch (opid)
        {
        case ParseResult::BuiltinOp::Op:
        {
            switch (opfield)
            {
            case ParseResult::BuiltinOp::FieldFmtStr:
            {
                uint32_t offset = opcodes[opOffset++];
                if (opdata & ParseResult::BuiltinOp::DataOffset16)
                    offset = (offset << 8) + opcodes[opOffset++];
                uint32_t length = opcodes[opOffset++];
                if (opdata & ParseResult::BuiltinOp::DataLength16)
                    length = (length << 8) + opcodes[opOffset++];
                executor.OnFmtStr(context, offset, length);
            } break;
            case ParseResult::BuiltinOp::FieldBrace:
            {
                executor.OnBrace(context, opdata == 0);
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
                    executor.OnColor(context, { isBG });
                    break;
                case ParseResult::ColorOp::DataBit8:
                    executor.OnColor(context, { isBG, opcodes[opOffset++] });
                    break;
                case ParseResult::ColorOp::DataBit24:
                    executor.OnColor(context, { isBG, *reinterpret_cast<const std::array<uint8_t, 3>*>(&opcodes[opOffset]) });
                    opOffset += 3;
                    break;
                default:
                    break;
                }
            }
            else
            {
                executor.OnColor(context, { isBG, static_cast<CommonColor>(opdata) });
            }
        } break;
        case ParseResult::ArgOp::Op:
        {
            const auto argIdx = opcodes[opOffset++];
            const bool hasSpec = opfield & ParseResult::ArgOp::FieldHasSpec;
            reader.Reset(hasSpec ? opcodes.data() + opOffset : nullptr);
            const bool isNamed = opfield & ParseResult::ArgOp::FieldNamed;
            executor.OnArg(context, argIdx, isNamed, reader);
            if (hasSpec)
                opOffset += reader.EnsureSize();
        } break;
        default:
            break;
        }
    }
    opcodes = opcodes.subspan(opOffset);
    return icnt;
}
template SYSCOMMONTPL uint32_t FormatterBase::Execute(span<const uint8_t>& opcodes, FormatterExecutor& executor, FormatterExecutor::Context& context, uint32_t instCount);


bool FormatSpecCacher::Cache(const StrArgInfo& strInfo, const ArgInfo& argInfo, const NamedMapper& mapper) noexcept
{
    struct RecordExecutor final : public FormatterExecutor, private FormatterBase
    {
        FormatSpecCacher& Cache;
        const StrArgInfo& StrInfo;
        const ArgInfo& RealArgInfo;
        const NamedMapper& Mapper;

        void PutString(Context&, ::std::   string_view, const OpaqueFormatSpec&) final {}
        void PutString(Context&, ::std::u16string_view, const OpaqueFormatSpec&) final {}
        void PutString(Context&, ::std::u32string_view, const OpaqueFormatSpec&) final {}
        void PutInteger(Context&, uint32_t, bool, const OpaqueFormatSpec&) final {}
        void PutInteger(Context&, uint64_t, bool, const OpaqueFormatSpec&) final {}
        void PutFloat  (Context&, float , const OpaqueFormatSpec&) final {}
        void PutFloat  (Context&, double, const OpaqueFormatSpec&) final {}
        void PutPointer(Context&, uintptr_t, const OpaqueFormatSpec&) final {}

        void PutString(Context&, ::std::   string_view, const FormatSpec*) final {}
        void PutString(Context&, ::std::u16string_view, const FormatSpec*) final {}
        void PutString(Context&, ::std::u32string_view, const FormatSpec*) final {}
        void PutInteger(Context&, uint32_t, bool, const FormatSpec*) final {}
        void PutInteger(Context&, uint64_t, bool, const FormatSpec*) final {}
        void PutFloat  (Context&, float, const FormatSpec*) final {}
        void PutFloat  (Context&, double, const FormatSpec*) final {}
        void PutPointer(Context&, uintptr_t, const FormatSpec*) final {}
        void PutDate   (Context&, ::std::string_view, const ::std::tm&) final {}

        void OnFmtStr(Context&, uint32_t, uint32_t) final {}
        void OnArg(Context&, uint8_t argIdx, bool isNamed, SpecReader& reader) final
        {
            const auto dispType = isNamed ? StrInfo.NamedTypes[argIdx].Type : StrInfo.IndexTypes[argIdx];
            const auto realType = isNamed ? RealArgInfo.NamedTypes[Mapper[argIdx]] : RealArgInfo.IndexTypes[argIdx];
            const auto bitIndex = (isNamed ? StrInfo.NamedTypes.size() : 0) + argIdx;
            auto& target = isNamed ? Cache.NamedSpec : Cache.IndexSpec;
            target.resize(std::max<size_t>(target.size(), argIdx + 1));
            Cache.CachedBitMap.Set(bitIndex, ConvertSpec(target[argIdx], reader.ReadSpec(), realType, dispType));
        }

        constexpr RecordExecutor(FormatSpecCacher& cache, const StrArgInfo& strInfo, const ArgInfo& argInfo, const NamedMapper& mapper) noexcept :
            Cache(cache), StrInfo(strInfo), RealArgInfo(argInfo), Mapper(mapper) {}
        using FormatterBase::Execute;
    };
    IndexSpec.clear();
    IndexSpec.reserve(strInfo.IndexTypes.size());
    NamedSpec.clear();
    NamedSpec.reserve(strInfo.NamedTypes.size());
    CachedBitMap = SmallBitset(strInfo.IndexTypes.size() + strInfo.NamedTypes.size(), false);
    RecordExecutor executor{ *this, strInfo, argInfo, mapper };
    RecordExecutor::Context context;
    auto opcodes = strInfo.Opcodes;
    RecordExecutor::Execute(opcodes, executor, context);
    return true;
}


template<typename Char>
struct WrapExecutor final : public FormatterExecutor
{
public:
    using CFmter = CommonFormatter<Char>;
    using CTX = FormatterExecutor::Context;
    struct Context : public CTX
    {
        std::basic_string<Char>& Dst;
        constexpr Context(std::basic_string<Char>& dst) noexcept : Dst(dst) { }
    };
    Formatter<Char>& Fmter;
    constexpr WrapExecutor(Formatter<Char>& fmter) noexcept : Fmter(fmter) {}

    void OnBrace(CTX& ctx, bool isLeft) final
    {
        auto& context = static_cast<Context&>(ctx);
        context.Dst.push_back(ParseLiterals<Char>::BracePair[isLeft ? 0 : 1]);
    }
    void OnColor(CTX& ctx, ScreenColor color) final
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter.PutColor(context.Dst, color);
    }

    void PutString(CTX& ctx, ::std::   string_view str, const OpaqueFormatSpec& spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutString(context.Dst, str, &spec);
    }
    void PutString(CTX& ctx, ::std::u16string_view str, const OpaqueFormatSpec& spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutString(context.Dst, str, &spec);
    }
    void PutString(CTX& ctx, ::std::u32string_view str, const OpaqueFormatSpec& spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutString(context.Dst, str, &spec);
    }
    void PutInteger(CTX& ctx, uint32_t val, bool isSigned, const OpaqueFormatSpec& spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutInteger(context.Dst, val, isSigned, spec);
    }
    void PutInteger(CTX& ctx, uint64_t val, bool isSigned, const OpaqueFormatSpec& spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutInteger(context.Dst, val, isSigned, spec);
    }
    void PutFloat  (CTX& ctx, float  val, const OpaqueFormatSpec& spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutFloat(context.Dst, val, spec);
    }
    void PutFloat  (CTX& ctx, double val, const OpaqueFormatSpec& spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutFloat(context.Dst, val, spec);
    }
    void PutPointer(CTX& ctx, uintptr_t val, const OpaqueFormatSpec& spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutPointer(context.Dst, val, spec);
    }
    
    void PutString(CTX& ctx, std::string_view str, const FormatSpec* spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter.PutString(context.Dst, str, spec);
    }
    void PutString(CTX& ctx, std::u16string_view str, const FormatSpec* spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter.PutString(context.Dst, str, spec);
    }
    void PutString(CTX& ctx, std::u32string_view str, const FormatSpec* spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter.PutString(context.Dst, str, spec);
    }
    void PutInteger(CTX& ctx, uint32_t val, bool isSigned, const FormatSpec* spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter.PutInteger(context.Dst, val, isSigned, spec);
    }
    void PutInteger(CTX& ctx, uint64_t val, bool isSigned, const FormatSpec* spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter.PutInteger(context.Dst, val, isSigned, spec);
    }
    void PutFloat(CTX& ctx, float  val, const FormatSpec* spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter.PutFloat(context.Dst, val, spec);
    }
    void PutFloat(CTX& ctx, double val, const FormatSpec* spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter.PutFloat(context.Dst, val, spec);
    }
    void PutPointer(CTX& ctx, uintptr_t val, const FormatSpec* spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter.PutPointer(context.Dst, val, spec);
    }
    void PutDate(CTX& ctx, std::string_view fmtStr, const std::tm& date) final
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter.PutDateBase(context.Dst, fmtStr, date);
    }
private:
    void OnFmtStr(CTX&, uint32_t, uint32_t) final
    {
        Expects(false);
    }
    void OnArg(CTX&, uint8_t, bool, SpecReader&) final
    {
        Expects(false);
    }
};

enum class RealSizeInfo : uint32_t { Byte1 = 0, Byte2 = 1, Byte4 = 2, Byte8 = 3 };
template<typename T>
constexpr RealSizeInfo GetSizeInfo() noexcept
{
    if constexpr (sizeof(T) == 1)
        return RealSizeInfo::Byte1;
    else if constexpr (sizeof(T) == 2)
        return RealSizeInfo::Byte2;
    else if constexpr (sizeof(T) == 4)
        return RealSizeInfo::Byte4;
    else if constexpr (sizeof(T) == 8)
        return RealSizeInfo::Byte8;
    else
        static_assert(!AlwaysTrue<T>, "unsupported type size");
}

template<typename Char>
forceinline void FormatterBase::StaticExecutor<Char>::OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader)
{
    ArgDispType fmtType = ArgDispType::Any;
    const uint16_t* argPtr = nullptr;
    ArgRealType argType = ArgRealType::Error;
    if (isNamed)
    {
        fmtType = context.StrInfo.NamedTypes[argIdx].Type;
        const auto mapIdx = context.Mapping[argIdx];
        argType = context.TheArgInfo.NamedTypes[mapIdx];
        const auto argSlot = mapIdx + context.TheArgInfo.IdxArgCount;
        argPtr = context.ArgStore.data() + context.ArgStore[argSlot];
    }
    else
    {
        fmtType = context.StrInfo.IndexTypes[argIdx];
        argType = context.TheArgInfo.IndexTypes[argIdx];
        const auto argSlot = argIdx;
        argPtr = context.ArgStore.data() + context.ArgStore[argSlot];
    }
    const auto realType = argType & ArgRealType::BaseTypeMask;
    const auto intSize = static_cast<RealSizeInfo>(enum_cast(argType & ArgRealType::TypeSizeMask) >> 4);
    const auto spec = reader.ReadSpec();
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
        if (fmtType == ArgDispType::Pointer)
        {
            Fmter.PutPointer(context.Dst, ptr, spec);
        }
        else
        {
            ABORT_CHECK(fmtType == ArgDispType::String || fmtType == ArgDispType::Any);
            switch (static_cast<RealSizeInfo>(enum_cast(intSize) & 0x3)) // remove StrPtrBit
            {
            case RealSizeInfo::Byte1: Fmter.PutString(context.Dst, BuildStr<char    >(ptr, len), spec); break;
            case RealSizeInfo::Byte2: Fmter.PutString(context.Dst, BuildStr<char16_t>(ptr, len), spec); break;
            case RealSizeInfo::Byte4: Fmter.PutString(context.Dst, BuildStr<char32_t>(ptr, len), spec); break;
            default: break;
            }
        }
    } return;
    case ArgRealType::Bool:
    {
        const auto val = *reinterpret_cast<const uint16_t*>(argPtr);
        if (fmtType == ArgDispType::Char)
        {
            if constexpr (sizeof(Char) == 1)
                Fmter.PutString(context.Dst, val ?  "Y"sv : "N"sv, spec);
            else if constexpr (sizeof(Char) == 2)
                Fmter.PutString(context.Dst, val ? u"Y"sv : u"N"sv, spec);
            else if constexpr (sizeof(Char) == 4)
                Fmter.PutString(context.Dst, val ? U"Y"sv : U"N"sv, spec);
            else
                static_assert(!AlwaysTrue<Char>, "unsupported char size");
        }
        else if (fmtType == ArgDispType::Integer)
        {
            Fmter.PutInteger(context.Dst, val ? 1u : 0u, false, spec);
        }
        else
        {
            ABORT_CHECK(fmtType == ArgDispType::String || fmtType == ArgDispType::Any);
            Fmter.PutString(context.Dst, GetStrTrueFalse<SimChar<Char>>(val), nullptr);
        }
    } return;
    case ArgRealType::Ptr:
    {
        ABORT_CHECK(fmtType == ArgDispType::Pointer || fmtType == ArgDispType::Any);
        const auto val = *reinterpret_cast<const uintptr_t*>(argPtr);
        Fmter.PutPointer(context.Dst, val, spec);
    } return;
    case ArgRealType::Date:
    {
        ABORT_CHECK(fmtType == ArgDispType::Date || fmtType == ArgDispType::Any);
        std::basic_string_view<Char> fmtStr{};
        if (spec && spec->FmtLen)
            fmtStr = context.StrInfo.FormatString.substr(spec->FmtOffset, spec->FmtLen);
        std::tm date{};
        if (HAS_FIELD(argType, ArgRealType::DateDeltaBit))
        {
            const auto delta = *reinterpret_cast<const int64_t*>(argPtr);
            const std::chrono::duration<int64_t, std::milli> d{ delta };
            const std::chrono::time_point<std::chrono::system_clock> tp{ d };
            const auto time = std::chrono::system_clock::to_time_t(tp);
            date = fmt::localtime(time);
        }
        else
            date = *reinterpret_cast<const CompactDate*>(argPtr);
        Fmter.PutDate(context.Dst, fmtStr, date);
    } return;
    case ArgRealType::Color:
    {
        ABORT_CHECK(fmtType == ArgDispType::Color || fmtType == ArgDispType::Any);
        switch (intSize)
        {
        case RealSizeInfo::Byte1: Fmter.PutColorArg(context.Dst, { false, *reinterpret_cast<const CommonColor*>(argPtr) }, spec); break;
        case RealSizeInfo::Byte4: Fmter.PutColorArg(context.Dst,          *reinterpret_cast<const ScreenColor*>(argPtr)  , spec); break;
        default: break;
        }
    } return;
    case ArgRealType::Custom:
    {
        const auto& pair = *reinterpret_cast<const detail::FmtWithPair*>(argPtr);
        WrapExecutor<Char> executor{ Fmter };
        typename WrapExecutor<Char>::Context context2{ context.Dst };
        pair.Executor(pair.Data, executor, context2, spec);
    } return;
    case ArgRealType::Float:
    {
        switch (intSize)
        {
        case RealSizeInfo::Byte2: Fmter.PutFloat(context.Dst, static_cast<float>(*reinterpret_cast<const half_float::half*>(argPtr)), spec); break;
        case RealSizeInfo::Byte4: Fmter.PutFloat(context.Dst,                    *reinterpret_cast<const float           *>(argPtr),  spec); break;
        case RealSizeInfo::Byte8: Fmter.PutFloat(context.Dst,                    *reinterpret_cast<const double          *>(argPtr),  spec); break;
        default: break;
        }
    } return;
    // Below are integer
    case ArgRealType::Char:
    {
        if (fmtType == ArgDispType::Char || fmtType == ArgDispType::Any)
        {
            switch (intSize)
            {
            case RealSizeInfo::Byte1: Fmter.PutString(context.Dst, std::   string_view{ reinterpret_cast<const char    *>(argPtr), 1 }, spec); break;
            case RealSizeInfo::Byte2: Fmter.PutString(context.Dst, std::u16string_view{ reinterpret_cast<const char16_t*>(argPtr), 1 }, spec); break;
            case RealSizeInfo::Byte4: Fmter.PutString(context.Dst, std::u32string_view{ reinterpret_cast<const char32_t*>(argPtr), 1 }, spec); break;
            default: break;
            }
            return;
        }
    }
    [[fallthrough]];
    case ArgRealType::SInt:
    case ArgRealType::UInt:
    {
        ABORT_CHECK(fmtType == ArgDispType::Integer || fmtType == ArgDispType::Any);
        const auto isSigned = realType == ArgRealType::SInt;
        switch (intSize)
        {
        case RealSizeInfo::Byte1: Fmter.PutInteger(context.Dst, static_cast<uint32_t>(*reinterpret_cast<const uint8_t *>(argPtr)), isSigned, spec); break;
        case RealSizeInfo::Byte2: Fmter.PutInteger(context.Dst, static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(argPtr)), isSigned, spec); break;
        case RealSizeInfo::Byte4: Fmter.PutInteger(context.Dst, static_cast<uint32_t>(*reinterpret_cast<const uint32_t*>(argPtr)), isSigned, spec); break;
        case RealSizeInfo::Byte8: Fmter.PutInteger(context.Dst, static_cast<uint64_t>(*reinterpret_cast<const uint64_t*>(argPtr)), isSigned, spec); break;
        default: break;
        }
    } return;
    default:
        return;
    }
}
template SYSCOMMONTPL void FormatterBase::StaticExecutor<char>    ::OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader);
template SYSCOMMONTPL void FormatterBase::StaticExecutor<wchar_t> ::OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader);
template SYSCOMMONTPL void FormatterBase::StaticExecutor<char16_t>::OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader);
template SYSCOMMONTPL void FormatterBase::StaticExecutor<char32_t>::OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader);
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template SYSCOMMONTPL void FormatterBase::StaticExecutor<char8_t> ::OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader);
#endif


template<typename Char>
void FormatterBase::FormatTo(Formatter<Char>& formatter, std::basic_string<Char>& ret, const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo, span<const uint16_t> argStore, const NamedMapper& mapping)
{
    typename StaticExecutor<Char>::Context context{ ret, strInfo, argInfo, argStore, mapping };
    StaticExecutor<Char> executor{ formatter };
    auto opcodes = strInfo.Opcodes;
    Execute(opcodes, executor, context);
}
template SYSCOMMONTPL void FormatterBase::FormatTo(Formatter<char>    & formatter, std::basic_string<char>    & ret, const StrArgInfoCh<char>    & strInfo, const ArgInfo& argInfo, span<const uint16_t> argStore, const NamedMapper& mapping);
template SYSCOMMONTPL void FormatterBase::FormatTo(Formatter<wchar_t> & formatter, std::basic_string<wchar_t> & ret, const StrArgInfoCh<wchar_t> & strInfo, const ArgInfo& argInfo, span<const uint16_t> argStore, const NamedMapper& mapping);
template SYSCOMMONTPL void FormatterBase::FormatTo(Formatter<char16_t>& formatter, std::basic_string<char16_t>& ret, const StrArgInfoCh<char16_t>& strInfo, const ArgInfo& argInfo, span<const uint16_t> argStore, const NamedMapper& mapping);
template SYSCOMMONTPL void FormatterBase::FormatTo(Formatter<char32_t>& formatter, std::basic_string<char32_t>& ret, const StrArgInfoCh<char32_t>& strInfo, const ArgInfo& argInfo, span<const uint16_t> argStore, const NamedMapper& mapping);
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template SYSCOMMONTPL void FormatterBase::FormatTo(Formatter<char8_t> & formatter, std::basic_string<char8_t> & ret, const StrArgInfoCh<char8_t> & strInfo, const ArgInfo& argInfo, span<const uint16_t> argStore, const NamedMapper& mapping);
#endif


}

