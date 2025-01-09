#include "SystemCommonPch.h"
#include "common/simd/SIMD128.hpp"
#include "Format.h"
#include "FormatExtra.h"
#include "StringFormat.h"
#include "StringConvert.h"
#include "StrEncodingBase.hpp"
#include "StackTrace.h"
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

#if COMMON_COMPILER_GCC && COMMON_GCC_VER < 100000
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wattributes"
#endif
namespace common::str
{
using namespace std::string_view_literals;

static_assert(sizeof(fmt::format_specs) == sizeof(OpaqueFormatSpecReal), "OpaqueFormatSpec size mismatch, incompatible");


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

ArgFormatException::ExceptionInfo::ExceptionInfo(const std::u16string_view msg, int16_t index,
    ArgDispType dispType, ArgRealType realType) noexcept
    : TPInfo(TYPENAME, msg), ArgIndex(index), ArgType{ dispType, realType }
{ }
COMMON_EXCEPTION_IMPL(ArgFormatException)


void ParseResultBase::CheckErrorRuntime(uint16_t errorPos, uint16_t errorNum)
{
    std::u16string_view errMsg;
    if (errorPos != UINT16_MAX)
    {
        switch (static_cast<ErrorCode>(errorNum))
        {
#define CHECK_ERROR_MSG(e, msg) case ErrorCode::e: errMsg = u ## msg; break;
            SCSTR_HANDLE_PARSE_ERROR(CHECK_ERROR_MSG)
#undef CHECK_ERROR_MSG
        default: errMsg = u"Unknown internal error"sv; break;
        }
        COMMON_THROW(BaseException, fmt::format(u"{} at [{}]"sv, errMsg, errorPos));
    }
}


DynamicTrimedResult::DynamicTrimedResult(const ParseResultCommon& result, span<const uint8_t> opcodes, size_t strLength, size_t charSize) :
    StrSize(static_cast<uint32_t>((strLength + 1) * charSize)), OpCount(gsl::narrow_cast<uint16_t>(opcodes.size())), NamedArgCount(result.NamedArgCount), IdxArgCount(result.IdxArgCount)
{
    static_assert(NASize == sizeof(uint32_t));
    static_assert(IASize == sizeof(uint8_t));
    Expects(OpCount > 0);
    Expects(StrSize > 0);
    ParseResultBase::CheckErrorRuntime(result.ErrorPos, result.ErrorNum);
    const auto naSize = NASize * NamedArgCount;
    const auto iaSize = IASize * IdxArgCount;
    const auto needLength = (naSize + StrSize + IdxArgCount + OpCount + sizeof(uint32_t) - 1) / sizeof(uint32_t);
    const auto space = FixedLenRefHolder<DynamicTrimedResult, uint32_t>::Allocate(needLength);
    Pointer = reinterpret_cast<uintptr_t>(space.data());
    if (NamedArgCount)
        memcpy_s(reinterpret_cast<void*>(Pointer), naSize, result.NamedTypes, naSize);
    if (IdxArgCount)
        memcpy_s(reinterpret_cast<void*>(Pointer + naSize + StrSize), iaSize, result.IndexTypes, iaSize);
    memcpy_s(reinterpret_cast<void*>(Pointer + naSize + StrSize + iaSize), OpCount, opcodes.data(), OpCount);
}


void ArgChecker::CheckDDBasic(const StrArgInfo& strInfo, const ArgInfo& argInfo)
{
    const auto strIndexArgCount = static_cast<uint8_t>(strInfo.IndexTypes.size());
    const auto strNamedArgCount = static_cast<uint8_t>(strInfo.NamedTypes.size());
    IF_UNLIKELY(argInfo.IdxArgCount < strIndexArgCount)
    {
        COMMON_THROW(ArgMismatchException, fmt::format(u"No enough indexed arg, expects [{}], has [{}]"sv,
            strIndexArgCount, argInfo.IdxArgCount), ArgMismatchException::Reasons::TooLessIndexArg,
            std::pair{ argInfo.IdxArgCount, strIndexArgCount }, std::pair{ argInfo.NamedArgCount, strNamedArgCount });
    }
    IF_UNLIKELY(argInfo.NamedArgCount < strNamedArgCount)
    {
        COMMON_THROW(ArgMismatchException, fmt::format(u"No enough named arg, expects [{}], has [{}]"sv,
            strNamedArgCount, argInfo.NamedArgCount), ArgMismatchException::Reasons::TooLessNamedArg,
            std::pair{ argInfo.IdxArgCount, strIndexArgCount }, std::pair{ argInfo.NamedArgCount, strNamedArgCount });
    }
    if (strIndexArgCount > 0)
    {
        const auto index = ArgChecker::GetIdxArgMismatch(strInfo.IndexTypes.data(), argInfo.IndexTypes, strIndexArgCount);
        IF_UNLIKELY(index != ParseResultCommon::IdxArgSlots)
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
    static inline constexpr uint32_t FillSizeOffset = 15;
    static inline constexpr uint32_t FillSizeMask = 0x38000u;
    template<typename T>
    static forceinline void SetFillSizeTo(T& spec, uint32_t size) noexcept
    {
        auto& data = *std::launder(reinterpret_cast<uint32_t*>(&spec));
        data = (data & ~FillSizeMask) | (size << FillSizeOffset);
    }
    template<typename T>
    static forceinline uint32_t GetFillSizeFrom(const T& spec) noexcept
    {
        const auto data = *reinterpret_cast<const uint32_t*>(&spec);
        return (data & FillSizeMask) >> FillSizeOffset;
    }

    COMMON_CONSTINIT inline static const fmt::format_specs BaseFmtSpec = {};
    template<typename T>
    static forceinline void ConvertSpecNoFill(fmt::format_specs& spec, const T& in) noexcept
    {
        static_assert(enum_cast(fmt::align::none)   == enum_cast(FormatSpec::Align::None));
        static_assert(enum_cast(fmt::align::left)   == enum_cast(FormatSpec::Align::Left));
        static_assert(enum_cast(fmt::align::center) == enum_cast(FormatSpec::Align::Middle));
        static_assert(enum_cast(fmt::align::right)  == enum_cast(FormatSpec::Align::Right));
        spec.width = in.Width;
        spec.precision = static_cast<int32_t>(in.Precision);
        if (in.AlterForm) spec.set_alt();
        if (in.ZeroPad)
            spec.set_align(fmt::align::numeric);
        else
        {
            CM_ASSUME(static_cast<uint8_t>(in.Alignment) < 4);
            spec.set_align(static_cast<fmt::align>(in.Alignment));
        }
    }
    template<typename T>
    static forceinline constexpr auto ConvertFill(char32_t ch) noexcept
    {
        std::array<typename T::ElementType, T::MaxOutputUnit> tmp{};
        const auto count = T::To(ch, T::MaxOutputUnit, tmp.data());
        return std::pair{ tmp, count };
    }
    template<typename Char, typename T>
    static forceinline void ConvertSpecFill(fmt::format_specs& spec, const T& in) noexcept
    {
        IF_UNLIKELY(!in.ZeroPad && in.Fill > 127)
        {
            if constexpr (sizeof(Char) == 4)
            {
                spec.set_fill(fmt::basic_string_view<detail::ForceFill4>(reinterpret_cast<const detail::ForceFill4*>(&in.Fill), 1));
                //spec.set_fill(fmt::basic_string_view<char32_t>(reinterpret_cast<const char32_t*>(&in.Fill), 1));
            }
            else if constexpr (sizeof(Char) == 2)
            {
                const auto [tmp, count] = ConvertFill<charset::detail::UTF16>(static_cast<char32_t>(in.Fill));
                spec.set_fill(fmt::basic_string_view<detail::ForceFill4>(reinterpret_cast<const detail::ForceFill4*>(tmp.data()), count));
                //spec.set_fill(fmt::basic_string_view<char16_t>(tmp.data(), count));
            }
            else if constexpr (sizeof(Char) == 1)
            {
                const auto [tmp, count] = ConvertFill<charset::detail::UTF8>(static_cast<char32_t>(in.Fill));
                spec.set_fill(fmt::basic_string_view<detail::ForceFill4>(reinterpret_cast<const detail::ForceFill4*>(tmp.data()), count));
                //spec.set_fill(fmt::basic_string_view<u8ch_t>(tmp.data(), count));
            }
            else
                static_assert(!AlwaysTrue<Char>);
        }
        else
            spec.set_fill(in.ZeroPad ? '0' : static_cast<char>(in.Fill));
    }
    static forceinline void ConvertSpecFillExtra(OpaqueFormatSpec& spec, bool zeroPad, char32_t fill) noexcept
    {
        if (!zeroPad && static_cast<uint32_t>(fill) > 127)
        {
            const auto [tmp16, count16] = ConvertFill<charset::detail::UTF16>(fill);
            CM_ASSUME(count16 > 0);
            CM_ASSUME(count16 <= 2);
            for (uint8_t i = 0; i < 2; ++i)
                spec.Fill16[i] = tmp16[i];
            spec.Fill32[0] = fill;
        }
    }
    static forceinline void ConvertSpecFill(OpaqueFormatSpec& spec, bool zeroPad, char32_t fill) noexcept
    {
        // UTF7 is compatible with UTF8/UTF16/UTF32 in LE 
        if (zeroPad)
        {
            spec.Real.Fill[0] = '0';
            SetFillSizeTo(spec.Real, 1);
        }
        else IF_LIKELY(fill <= 127)
        {
            spec.Real.Fill[0] = static_cast<uint8_t>(fill);
            SetFillSizeTo(spec.Real, 1);
        }
        else
        {
            const auto [tmp8, count8] = ConvertFill<charset::detail::UTF8>(fill);
            CM_ASSUME(count8 > 0);
            CM_ASSUME(count8 <= 4);
            for (uint8_t i = 0; i < count8; ++i)
                spec.Real.Fill[i] = tmp8[i];
            SetFillSizeTo(spec.Real, count8);
            const auto [tmp16, count16] = ConvertFill<charset::detail::UTF16>(fill);
            CM_ASSUME(count16 > 0);
            CM_ASSUME(count16 <= 2);
            // UTF16 surrogates is U+010000 ~ U+10FFFF, which is 4B in UTF8 (11110uvv 10vvwwww 10xxxxyy 10yyzzzz)
            Ensures(count16 == (count8 == 4 ? 2u : 1u));
            for (uint8_t i = 0; i < 2; ++i)
                spec.Fill16[i] = tmp16[i];
            spec.Fill32[0] = fill;
        }
    }
#if COMMON_COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
    //template<typename Char, typename T>
    //static forceinline fmt::format_specs ConvertSpecBasic(const T* in) noexcept
    //{
    //    fmt::format_specs spec;
    //    if (in)
    //    {
    //        memcpy(&spec, &BaseFmtSpec, sizeof(fmt::basic_specs));
    //        ConvertSpecNoFill(spec, *in);
    //        ConvertSpecFill<Char>(spec, *in);
    //    }
    //    else
    //        memcpy(&spec, &BaseFmtSpec, sizeof(fmt::format_specs));
    //    return spec;
    //}
    template<typename Char, typename T>
    static forceinline void ConstructBasicAt(fmt::format_specs& spec, const T& in) noexcept
    {
        memcpy(&static_cast<fmt::basic_specs&>(spec), &BaseFmtSpec, sizeof(fmt::basic_specs));
        // static_cast<fmt::basic_specs&>(spec) = BaseFmtSpec;
        ConvertSpecNoFill(spec, in);
        ConvertSpecFill<Char>(spec, in);
    }
#if COMMON_COMPILER_GCC
#pragma GCC diagnostic pop
#endif
    static_assert(enum_cast(fmt::sign::none)  == enum_cast(FormatSpec::Sign::None));
    static_assert(enum_cast(fmt::sign::minus) == enum_cast(FormatSpec::Sign::Neg));
    static_assert(enum_cast(fmt::sign::plus)  == enum_cast(FormatSpec::Sign::Pos));
    static_assert(enum_cast(fmt::sign::space) == enum_cast(FormatSpec::Sign::Space));
    static forceinline constexpr fmt::sign ConvertSpecSign(const FormatSpec::Sign sign) noexcept
    {
        CM_ASSUME(static_cast<uint8_t>(sign) < 4);
        return static_cast<fmt::sign>(sign);
    }
    static constexpr uint32_t IntPrefixes[4] = { 0, 0, 0x1000000u | '+', 0x1000000u | ' ' };
    static forceinline constexpr uint32_t ConvertSpecIntSign(FormatSpec::Sign sign) noexcept
    {
        CM_ASSUME(static_cast<uint8_t>(sign) < 4);
        return IntPrefixes[enum_cast(sign)];
    }
    static forceinline constexpr uint32_t ConvertSpecIntSign(fmt::sign sign) noexcept
    {
        CM_ASSUME(static_cast<uint8_t>(sign) < 4);
        return IntPrefixes[enum_cast(sign)];
    }
    
    static constexpr auto IntPresentPack = BitsPackFrom<4>(
        fmt::presentation_type::dec, // d
        fmt::presentation_type::bin, // b
        fmt::presentation_type::bin, // B
        fmt::presentation_type::oct, // o
        fmt::presentation_type::hex, // x
        fmt::presentation_type::hex  // X
    );
    template<typename T>
    static forceinline constexpr void FillSpecIntPresent(fmt::format_specs& out, const T& spec) noexcept
    {
        uint8_t typeIdx = 0;
        if constexpr (std::is_same_v<T, FormatSpec>)
        {
            typeIdx = spec.TypeExtra;
        }
        else if constexpr (std::is_same_v<T, FormatterParser::FormatSpec>)
        {
            typeIdx = spec.Type.Extra;
        }
        else
        {
            static_assert(!AlwaysTrue<T>);
        }
        out.set_type(IntPresentPack.Get<fmt::presentation_type>(typeIdx));
        if (typeIdx == 2 || typeIdx == 5) out.set_upper(); // B, X
        return;
    }

    static constexpr auto FloatPresentPack = BitsPackFrom<4>(
        fmt::presentation_type::general,  // gG
        fmt::presentation_type::hexfloat, // aA
        fmt::presentation_type::exp,      // eE
        fmt::presentation_type::fixed     // fF
    );
    template<typename T>
    static forceinline constexpr void FillSpecFloatPresent(fmt::format_specs& out, const T& spec) noexcept
    {
        uint8_t typeIdx = 0;
        if constexpr (std::is_same_v<T, FormatSpec>)
        {
            typeIdx = spec.TypeExtra;
        }
        else if constexpr (std::is_same_v<T, FormatterParser::FormatSpec>)
        {
            typeIdx = spec.Type.Extra;
        }
        else
        {
            static_assert(!AlwaysTrue<T>);
        }
        out.set_type(FloatPresentPack.Get<fmt::presentation_type>(typeIdx >> 1));
        if (typeIdx & 0x1u) out.set_upper();
        return;
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
    static forceinline void PutString(std::basic_string<Dst>& ret, const Src* __restrict str, size_t len, const fmt::format_specs* __restrict spec)
    {
        if constexpr (std::is_same_v<Dst, Src>)
        {
            if (spec)
                fmt::detail::write<Dst>(std::back_inserter(ret), fmt::basic_string_view<Dst>(str, len), *spec);
            else
                ret.append(str, len);
        }
        else if constexpr (sizeof(Dst) == sizeof(Src))
        {
            PutString<Dst, Dst>(ret, reinterpret_cast<const Dst*>(str), len, spec);
        }
        else if constexpr (std::is_same_v<Dst, char>)
        {
            const auto newStr = str::to_string(str, len, Encoding::UTF8);
            PutString<char, char>(ret, newStr.data(), newStr.size(), spec);
        }
        else if constexpr (std::is_same_v<Dst, char16_t>)
        {
            const auto newStr = str::to_u16string(str, len);
            PutString<char16_t, char16_t>(ret, newStr.data(), newStr.size(), spec);
        }
        else if constexpr (std::is_same_v<Dst, char32_t>)
        {
            const auto newStr = str::to_u32string(str, len);
            PutString<char32_t, char32_t>(ret, newStr.data(), newStr.size(), spec);
        }
        else if constexpr (std::is_same_v<Dst, wchar_t>)
        {
            if constexpr (sizeof(wchar_t) == sizeof(char16_t))
            {
                const auto newStr = str::to_u16string(str, len);
                PutString<wchar_t, wchar_t>(ret, reinterpret_cast<const wchar_t*>(newStr.data()), newStr.size(), spec);
            }
            else if constexpr (sizeof(wchar_t) == sizeof(char32_t))
            {
                const auto newStr = str::to_u32string(str, len);
                PutString<wchar_t, wchar_t>(ret, reinterpret_cast<const wchar_t*>(newStr.data()), newStr.size(), spec);
            }
            else
                static_assert(!common::AlwaysTrue<Src>, "not supported");
        }
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
        else if constexpr (std::is_same_v<Dst, char8_t>)
        {
            const auto newStr = str::to_u8string(str, len);
            PutString<char8_t, char8_t>(ret, newStr.data(), newStr.size(), spec);
        }
#endif
        else
            static_assert(!common::AlwaysTrue<Src>, "not supported");
    }
};


bool FormatterExecutor::ConvertSpec(OpaqueFormatSpec& dst, const FormatSpec* src, ArgRealType real, ArgDispType disp) noexcept
{
    IF_UNLIKELY(!ArgChecker::CheckCompatible(disp, real))
        return false;

    auto& spec = *std::launder(reinterpret_cast<fmt::format_specs*>(&dst.Real));
    if (src)
    {
        FormatterHelper::ConvertSpecFillExtra(dst, src->ZeroPad, static_cast<char32_t>(src->Fill));
        FormatterHelper::ConstructBasicAt<char>(spec, *src);
    }
    else
        memcpy(&spec, &FormatterHelper::BaseFmtSpec, sizeof(fmt::format_specs));
    //fmt::format_specs spec = FormatterHelper::ConvertSpecBasic<char>(src);
        
    switch (const auto realType = real & ArgRealType::BaseTypeMask; realType)
    {
    case ArgRealType::String:
        if (disp == ArgDispType::Pointer)
            spec.set_type(fmt::presentation_type::pointer);
        else
            spec.set_type(fmt::presentation_type::string);
        break;
    case ArgRealType::Bool:
        IF_UNLIKELY(disp == ArgDispType::Integer || disp == ArgDispType::Numeric)
        {
            if (src)
                FormatterHelper::FillSpecIntPresent(spec, *src);
        }
        else
            spec.set_type(fmt::presentation_type::string);
        break;
    case ArgRealType::Ptr:
        spec.set_type(fmt::presentation_type::pointer);
        break;
    case ArgRealType::Float:
        if (src)
        {
            FormatterHelper::FillSpecFloatPresent(spec, *src);
            spec.set_sign(FormatterHelper::ConvertSpecSign(src->SignFlag));
        }
        break;
    case ArgRealType::Char:
        IF_LIKELY(disp == ArgDispType::Char || disp == ArgDispType::Any)
        {
            spec.set_type(fmt::presentation_type::string);
            break;
        }
        [[fallthrough]];
    case ArgRealType::SInt:
    case ArgRealType::UInt:
        ABORT_CHECK(disp == ArgDispType::Integer || disp == ArgDispType::Numeric || disp == ArgDispType::Any);
        if (src)
            FormatterHelper::FillSpecIntPresent(spec, *src);
        break;
    default:
        return false;
    }
    return true;
}

bool FormatterExecutor::ConvertSpec(OpaqueFormatSpec& dst, std::u32string_view spectxt, ArgRealType real) noexcept
{
    IF_UNLIKELY(spectxt.empty() || spectxt.size() > UINT16_MAX)
        return false;

    ParseResultBase res;
    FormatterParser::FormatSpec spec_;
    FormatterParserCh<char32_t>::ParseFormatSpec(res, spec_, spectxt.data(), 0, static_cast<uint32_t>(spectxt.size()));
    IF_UNLIKELY(res.ErrorPos != UINT16_MAX)
        return false;
    const auto disp = spec_.Type.Type;
    IF_UNLIKELY(!ArgChecker::CheckCompatible(disp, real))
        return false;

    auto& spec = *std::launder(reinterpret_cast<fmt::format_specs*>(&dst.Real));
    FormatterHelper::ConvertSpecFillExtra(dst, spec_.ZeroPad, static_cast<char32_t>(spec_.Fill));
    FormatterHelper::ConstructBasicAt<char>(spec, spec_);
    //fmt::format_specs spec = FormatterHelper::ConvertSpecBasic<char>(&spec_);

    switch (const auto realType = real & ArgRealType::BaseTypeMask; realType)
    {
    case ArgRealType::String:
        if (disp == ArgDispType::Pointer)
            spec.set_type(fmt::presentation_type::pointer);
        else
            spec.set_type(fmt::presentation_type::string);
        break;
    case ArgRealType::Bool:
        IF_UNLIKELY(disp == ArgDispType::Integer || disp == ArgDispType::Numeric)
            FormatterHelper::FillSpecIntPresent(spec, spec_);
        else
            spec.set_type(fmt::presentation_type::string);
        break;
    case ArgRealType::Ptr:
        spec.set_type(fmt::presentation_type::pointer);
        break;
    case ArgRealType::Float:
        FormatterHelper::FillSpecFloatPresent(spec, spec_);
        spec.set_sign(FormatterHelper::ConvertSpecSign(spec_.SignFlag));
        break;
    case ArgRealType::Char:
        IF_LIKELY(disp == ArgDispType::Char || disp == ArgDispType::Any)
        {
            spec.set_type(fmt::presentation_type::string);
            break;
        }
        [[fallthrough]];
    case ArgRealType::SInt:
    case ArgRealType::UInt:
        ABORT_CHECK(disp == ArgDispType::Integer || disp == ArgDispType::Numeric || disp == ArgDispType::Any);
        FormatterHelper::FillSpecIntPresent(spec, spec_);
        break;
    default:
        return false;
    }
    return true;
}


template<typename Char>
struct WrapSpec
{
    using T = fmt::format_specs;
    const T* Ptr = nullptr;
    uint8_t Data[sizeof(T)] = {};
    WrapSpec(const OpaqueFormatSpec& spec) noexcept
    {
        if constexpr (sizeof(Char) == 1) // always the correct fill
            Ptr = reinterpret_cast<const T*>(&spec);
        else
        {
            const auto fillsize = FormatterHelper::GetFillSizeFrom(spec);
            IF_LIKELY(fillsize <= 1) // already has compatible fill
            {
                Ptr = reinterpret_cast<const T*>(&spec);
            }
            else
            {
                memcpy(&Data, &spec.Real, sizeof(T));
                const auto outspec = std::launder(reinterpret_cast<T*>(Data));
                if constexpr (sizeof(Char) == 4)
                    outspec->set_fill(fmt::basic_string_view<detail::ForceFill4>{ reinterpret_cast<const detail::ForceFill4*>(spec.Fill32), 1 }); // length always 1
                else if constexpr (sizeof(Char) == 2)
                    outspec->set_fill(fmt::basic_string_view<detail::ForceFill4>{ reinterpret_cast<const detail::ForceFill4*>(spec.Fill16), fillsize == 4u ? 2u : 1u }); // surrogates when UTF8 sizxe is 4
                else
                    static_assert(!AlwaysTrue<Char>);
                Ptr = outspec;
                /*
                // plain copy is faster than fmt's constructor
                const auto offset = spec.Real.Fill - spec.Real.Data;
                if constexpr (sizeof(Char) == 4)
                {
                    memcpy(Data + offset, &spec.Fill32, sizeof(spec.Fill32));
                    FormatterHelper::SetFillSizeTo(Data, 1); // always 1
                }
                else if constexpr (sizeof(Char) == 2)
                {
                    memcpy(Data + offset, &spec.Fill16, sizeof(spec.Fill16));
                    FormatterHelper::SetFillSizeTo(Data, fillsize == 4 ? 2 : 1); // surrogates when UTF8 sizxe is 4
                }
                else
                    static_assert(!AlwaysTrue<Char>);
                Ptr = reinterpret_cast<const T*>(Data);
                */
            }
        }
    }
};


template<typename Char>
void CommonFormatter<Char>::PutString(StrType& ret, const void* str, size_t len, FormatterExecutor::StringType type, const OpaqueFormatSpec* spec)
{
    if (spec)
    {
        WrapSpec<Char> fmtSpec(*spec);
        switch (type)
        {
        case FormatterExecutor::StringType::UTF8:  return FormatterHelper::PutString(ret, reinterpret_cast<const char    *>(str), len, fmtSpec.Ptr);
        case FormatterExecutor::StringType::UTF16: return FormatterHelper::PutString(ret, reinterpret_cast<const char16_t*>(str), len, fmtSpec.Ptr);
        case FormatterExecutor::StringType::UTF32: return FormatterHelper::PutString(ret, reinterpret_cast<const char32_t*>(str), len, fmtSpec.Ptr);
        default: CM_UNREACHABLE(); return;
        }
    }
    switch (type)
    {
    case FormatterExecutor::StringType::UTF8:  return FormatterHelper::PutString(ret, reinterpret_cast<const char    *>(str), len, nullptr);
    case FormatterExecutor::StringType::UTF16: return FormatterHelper::PutString(ret, reinterpret_cast<const char16_t*>(str), len, nullptr);
    case FormatterExecutor::StringType::UTF32: return FormatterHelper::PutString(ret, reinterpret_cast<const char32_t*>(str), len, nullptr);
    default: CM_UNREACHABLE(); return;
    }
}

template<typename Char>
void CommonFormatter<Char>::PutInteger(StrType& ret, uint32_t val, bool isSigned, const OpaqueFormatSpec& spec)
{
    WrapSpec<Char> spec_(spec);
    const auto& fmtSpec = *spec_.Ptr;
    const auto prefix = FormatterHelper::ProcessIntSign(val, isSigned, fmtSpec.sign());
    const fmt::detail::write_int_arg<uint32_t> arg{ val, prefix };
    fmt::detail::write_int<Char>(std::back_inserter(ret), arg, fmtSpec);
}
template<typename Char>
void CommonFormatter<Char>::PutInteger(StrType& ret, uint64_t val, bool isSigned, const OpaqueFormatSpec& spec)
{
    WrapSpec<Char> spec_(spec);
    const auto& fmtSpec = *spec_.Ptr;
    const auto prefix = FormatterHelper::ProcessIntSign(val, isSigned, fmtSpec.sign());
    const fmt::detail::write_int_arg<uint64_t> arg{ val, prefix };
    fmt::detail::write_int<Char>(std::back_inserter(ret), arg, fmtSpec);
}

template<typename Char>
void CommonFormatter<Char>::PutFloat(StrType& ret, float  val, const OpaqueFormatSpec& spec)
{
    WrapSpec<Char> spec_(spec);
    const auto& fmtSpec = *spec_.Ptr;
    fmt::detail::write_float<Char>(std::back_inserter(ret), val, fmtSpec, {});
}
template<typename Char>
void CommonFormatter<Char>::PutFloat(StrType& ret, double val, const OpaqueFormatSpec& spec)
{
    WrapSpec<Char> spec_(spec);
    const auto& fmtSpec = *spec_.Ptr;
    fmt::detail::write_float<Char>(std::back_inserter(ret), val, fmtSpec, {});
}

template<typename Char>
void CommonFormatter<Char>::PutPointer(StrType& ret, uintptr_t val, const OpaqueFormatSpec& spec)
{
    WrapSpec<Char> spec_(spec);
    const auto& fmtSpec = *spec_.Ptr;
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
        SWITCH_LIKELY(color.Type, ScreenColor::ColorType::Common)
        {
        CASE_LIKELY(ScreenColor::ColorType::Common):
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
                fmt::format_specs fmtSpec = {};
                fmtSpec.set_type(fmt::presentation_type::hex);
                //fmtSpec.upper = false;
                fmtSpec.set_fill('0');
                fmtSpec.width = 6;
                const uint32_t color24 = (color.Value[0] << 0) | (color.Value[1] << 8) | (color.Value[2] << 16);
                fmt::detail::write_int_arg<uint32_t> arg{ color24, 0 };
                fmt::detail::write_int<Char>(std::back_inserter(ret), arg, fmtSpec);
            }
            else
            {
                char head[] = "rgb(";
                ret.append(&head[0], &head[4]);
                fmt::format_specs fmtSpec = {};
                fmtSpec.set_type(fmt::presentation_type::dec);
                fmt::detail::write_int_arg<uint32_t> arg{ 0, 0 };

                arg.abs_value = color.Value[0];
                fmt::detail::write_int<Char>(std::back_inserter(ret), arg, fmtSpec);
                ret.push_back(',');

                arg.abs_value = color.Value[1];
                fmt::detail::write_int<Char>(std::back_inserter(ret), arg, fmtSpec);
                ret.push_back(',');

                arg.abs_value = color.Value[2];
                fmt::detail::write_int<Char>(std::back_inserter(ret), arg, fmtSpec);
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
void Formatter<Char>::PutString(StrType& ret, const void* str, size_t len, FormatterExecutor::StringType type, const FormatSpec* spec)
{
    if (spec)
    {
        fmt::format_specs fmtSpec;
        FormatterHelper::ConstructBasicAt<Char>(fmtSpec, *spec);
        fmtSpec.set_type(fmt::presentation_type::string);
        switch (type)
        {
        case FormatterExecutor::StringType::UTF8:  return FormatterHelper::PutString(ret, reinterpret_cast<const char    *>(str), len, &fmtSpec);
        case FormatterExecutor::StringType::UTF16: return FormatterHelper::PutString(ret, reinterpret_cast<const char16_t*>(str), len, &fmtSpec);
        case FormatterExecutor::StringType::UTF32: return FormatterHelper::PutString(ret, reinterpret_cast<const char32_t*>(str), len, &fmtSpec);
        default: CM_UNREACHABLE(); return;
        }
    }
    switch (type)
    {
    case FormatterExecutor::StringType::UTF8:  return FormatterHelper::PutString(ret, reinterpret_cast<const char    *>(str), len, nullptr);
    case FormatterExecutor::StringType::UTF16: return FormatterHelper::PutString(ret, reinterpret_cast<const char16_t*>(str), len, nullptr);
    case FormatterExecutor::StringType::UTF32: return FormatterHelper::PutString(ret, reinterpret_cast<const char32_t*>(str), len, nullptr);
    default: CM_UNREACHABLE(); return;
    }
}

template<typename Char>
void Formatter<Char>::PutInteger(StrType& ret, uint32_t val, bool isSigned, const FormatSpec* spec)
{
    const auto prefix = FormatterHelper::ProcessIntSign(val, isSigned, spec ? spec->SignFlag : FormatSpec::Sign::None);
    const fmt::detail::write_int_arg<uint32_t> arg{ val, prefix };
    if (spec)
    {
        fmt::format_specs fmtSpec;
        FormatterHelper::ConstructBasicAt<Char>(fmtSpec, *spec);
        FormatterHelper::FillSpecIntPresent(fmtSpec, *spec);
        fmt::detail::write_int<Char>(std::back_inserter(ret), arg, fmtSpec);
    }
    else
        fmt::detail::write_int<Char>(std::back_inserter(ret), arg, FormatterHelper::BaseFmtSpec);
}
template<typename Char>
void Formatter<Char>::PutInteger(StrType& ret, uint64_t val, bool isSigned, const FormatSpec* spec)
{
    const auto prefix = FormatterHelper::ProcessIntSign(val, isSigned, spec ? spec->SignFlag : FormatSpec::Sign::None);
    const fmt::detail::write_int_arg<uint64_t> arg{ val, prefix };
    if (spec)
    {
        fmt::format_specs fmtSpec;
        FormatterHelper::ConstructBasicAt<Char>(fmtSpec, *spec);
        FormatterHelper::FillSpecIntPresent(fmtSpec, *spec);
        fmt::detail::write_int<Char>(std::back_inserter(ret), arg, fmtSpec);
    }
    else
        fmt::detail::write_int<Char>(std::back_inserter(ret), arg, FormatterHelper::BaseFmtSpec);
}

template<typename Char>
void Formatter<Char>::PutFloat(StrType& ret, float val, const FormatSpec* spec)
{
    if (spec)
    {
        fmt::format_specs fmtSpec;
        FormatterHelper::ConstructBasicAt<Char>(fmtSpec, *spec);
        FormatterHelper::FillSpecFloatPresent(fmtSpec, *spec);
        fmtSpec.set_sign(FormatterHelper::ConvertSpecSign(spec->SignFlag));
        fmt::detail::write_float<Char>(std::back_inserter(ret), val, fmtSpec, {});
    }
    else
        fmt::detail::write_float<Char>(std::back_inserter(ret), val, FormatterHelper::BaseFmtSpec, {});
}
template<typename Char>
void Formatter<Char>::PutFloat(StrType& ret, double val, const FormatSpec* spec)
{
    if (spec)
    {
        fmt::format_specs fmtSpec;
        FormatterHelper::ConstructBasicAt<Char>(fmtSpec, *spec);
        FormatterHelper::FillSpecFloatPresent(fmtSpec, *spec);
        fmtSpec.set_sign(FormatterHelper::ConvertSpecSign(spec->SignFlag));
        fmt::detail::write_float<Char>(std::back_inserter(ret), val, fmtSpec, {});
    }
    else
        fmt::detail::write_float<Char>(std::back_inserter(ret), val, FormatterHelper::BaseFmtSpec, {});
}

template<typename Char>
void Formatter<Char>::PutPointer(StrType& ret, uintptr_t val, const FormatSpec* spec)
{
    if (spec)
    {
        fmt::format_specs fmtSpec;
        FormatterHelper::ConstructBasicAt<Char>(fmtSpec, *spec);
        fmtSpec.set_type(fmt::presentation_type::pointer);
        fmt::detail::write_ptr<Char>(std::back_inserter(ret), val, &fmtSpec);
    }
    else
        fmt::detail::write_ptr<Char>(std::back_inserter(ret), val, &FormatterHelper::BaseFmtSpec);
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
struct DateWriter : public fmt::detail::tm_writer<std::back_insert_iterator<std::basic_string<Char>>, Char, std::chrono::microseconds>
{
    using TMT = fmt::detail::tm_writer<std::back_insert_iterator<std::basic_string<Char>>, Char, std::chrono::microseconds>;
    const DateStructure& Date;
    std::basic_string<Char>& Str;
    DateWriter(const DateStructure& date, const std::chrono::microseconds* subsec, std::basic_string<Char>& ret) noexcept :
        TMT({}, std::back_inserter(ret), date.Base, subsec), Date(date), Str(ret)
    { }

    void Write2(int32_t value) 
    {
        const char* d = fmt::detail::digits2(static_cast<uint32_t>(value) % 100);
        Str.append(d, d + 2);
    }

    void on_utc_offset(fmt::detail::numeric_system ns)
    {
        auto offset = Date.GMTOffset;
        if (offset < 0) 
        {
            Str.push_back('-');
            offset = -offset;
        }
        else 
        {
            Str.push_back('+');
        }
        offset /= 60;
        Write2(offset / 60);
        if (ns != fmt::detail::numeric_system::standard)
            Str.push_back(':'); 
        Write2(offset % 60);
    }

    void on_tz_name()
    {
        if (Date.Zone.empty())
            Str.push_back('Z');
        else
            Str.append(Date.Zone.begin(), Date.Zone.end());
    }
    
    static void Format(std::basic_string<Char>& ret, std::basic_string_view<Char> fmtStr, const DateStructure& date)
    {
        if (fmtStr.empty())
            fmtStr = GetDateStr<Char>();
        std::chrono::microseconds duration{ date.MicroSeconds };
        const auto subsec = date.MicroSeconds != UINT32_MAX ? &duration : nullptr;
        DateWriter writer(date, subsec, ret);
        fmt::detail::parse_chrono_format(fmtStr.data(), fmtStr.data() + fmtStr.size(), writer);
    }
};

template<typename Char>
void Formatter<Char>::PutDate(StrType& ret, std::basic_string_view<Char> fmtStr, const DateStructure& date)
{
#if COMMON_OS_ANDROID || COMMON_OS_IOS // android's std::time_put is limited to char/wchar_t
    if constexpr (std::is_same_v<Char, char> || std::is_same_v<Char, wchar_t>)
    {
        DateWriter<Char>::Format(ret, fmtStr, date);
    }
    else if constexpr (sizeof(Char) == sizeof(wchar_t))
    {
        DateWriter<wchar_t>::Format(*reinterpret_cast<std::wstring*>(&ret), *reinterpret_cast<std::wstring_view*>(&fmtStr), date);
    }
    else if constexpr (sizeof(Char) == sizeof(char))
    {
        DateWriter<char>::Format(*reinterpret_cast<std::string*>(&ret), *reinterpret_cast<std::string_view*>(&fmtStr), date);
    }
    else
    {
        const auto fmtStr_ = to_string(fmtStr, Encoding::UTF8);
        std::string tmp;
        DateWriter<char>::Format(tmp, fmtStr_, date);
        if constexpr (std::is_same_v<Char, char16_t>)
            ret.append(to_u16string(tmp, Encoding::UTF8));
        else
            ret.append(to_u32string(tmp, Encoding::UTF8));
    }
#else
    DateWriter<Char>::Format(ret, fmtStr, date);
#endif
}
template<typename Char>
void Formatter<Char>::PutDateBase(StrType& ret, std::string_view fmtStr, const DateStructure& date)
{
    if constexpr (sizeof(Char) == sizeof(char))
    {
        DateWriter<char>::Format(*reinterpret_cast<std::string*>(&ret), fmtStr, date);
    }
    else
    {
        std::string tmp;
        DateWriter<char>::Format(tmp, fmtStr, date);
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
    const auto result = FormatterParser::ParseString<Char>(format);
    ParseResultBase::CheckErrorRuntime(result.ErrorPos, result.ErrorNum);
    const auto res = result.ToInfo(format);
    const auto mapping = ArgChecker::CheckDD(res, argInfo);
    FormatterBase::FormatTo<Char>(*this, dst, res, argInfo, argStore, mapping);
}


static constexpr auto ShortLenMap = []() 
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
        IF_UNLIKELY(hasExtraFmt)
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
        const auto opid    = op & FormatterParser::OpIdMask;
        const auto opfield = op & FormatterParser::OpFieldMask;
        const auto opdata  = op & FormatterParser::OpDataMask;
        switch (opid)
        {
        case FormatterParser::BuiltinOp::Op:
        {
            switch (opfield)
            {
            case FormatterParser::BuiltinOp::FieldFmtStr:
            {
                uint32_t offset = opcodes[opOffset++];
                if (opdata & FormatterParser::BuiltinOp::DataOffset16)
                    offset = (offset << 8) + opcodes[opOffset++];
                uint32_t length = opcodes[opOffset++];
                if (opdata & FormatterParser::BuiltinOp::DataLength16)
                    length = (length << 8) + opcodes[opOffset++];
                executor.OnFmtStr(context, offset, length);
            } break;
            case FormatterParser::BuiltinOp::FieldBrace:
            {
                executor.OnBrace(context, opdata == 0);
            } break;
            default:
                break;
            }
        } break;
        case FormatterParser::ColorOp::Op:
        {
            const bool isBG = opfield & FormatterParser::ColorOp::FieldBackground;
            IF_UNLIKELY(opfield & FormatterParser::ColorOp::FieldSpecial)
            {
                switch (opdata)
                {
                case FormatterParser::ColorOp::DataDefault:
                    executor.OnColor(context, { isBG });
                    break;
                case FormatterParser::ColorOp::DataBit8:
                    executor.OnColor(context, { isBG, opcodes[opOffset++] });
                    break;
                case FormatterParser::ColorOp::DataBit24:
                    executor.OnColor(context, { isBG, *reinterpret_cast<const std::array<uint8_t, 3>*>(&opcodes[opOffset]) });
                    opOffset += 3;
                    break;
                default:
                    break;
                }
            }
            ELSE_LIKELY
            {
                executor.OnColor(context, { isBG, static_cast<CommonColor>(opdata) });
            }
        } break;
        case FormatterParser::ArgOp::Op:
        {
            const auto argIdx = opcodes[opOffset++];
            const bool hasSpec = opfield & FormatterParser::ArgOp::FieldHasSpec;
            reader.Reset(hasSpec ? opcodes.data() + opOffset : nullptr);
            const bool isNamed = opfield & FormatterParser::ArgOp::FieldNamed;
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

    void PutString(CTX& ctx, const void* str, size_t len, StringType type, const OpaqueFormatSpec& spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutString(context.Dst, str, len, type, &spec);
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
    
    void PutString(CTX& ctx, const void* str, size_t len, StringType type, const FormatSpec* spec) final
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter.PutString(context.Dst, str, len, type, spec);
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
    void PutDate(CTX& ctx, std::string_view fmtStr, const DateStructure& date) final
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
template<typename T>
forceinline static std::basic_string_view<T> BuildStr(uintptr_t ptr, const size_t* lenPtr) noexcept
{
    const auto arg = reinterpret_cast<const T*>(ptr);
    const auto len = lenPtr ? *lenPtr : std::char_traits<T>::length(arg);
    return { arg, len };
}

template<typename T>
forceinline std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> SetZoneInfo(const date::time_zone& zone, std::chrono::time_point<date::local_t, std::chrono::microseconds> ltime, T& date) noexcept
{
    static_assert(std::is_same_v<T, DateStructure>);
    const auto systime = zone.to_sys(ltime);
    const auto info = zone.get_info(systime);
    date.Base.tm_isdst = info.save == std::chrono::minutes::zero() ? 0 : 1;
    date.Zone = info.abbrev;
    date.GMTOffset = gsl::narrow_cast<int32_t>(info.offset.count());
    if constexpr (detail::tm_has_all_zone_info)
    {
        date.Base.tm_zone = date.Zone.c_str();
        date.Base.tm_gmtoff = date.GMTOffset;
    }
    return systime;
}

static void FillTime(const date::time_zone* timezone, uint64_t encodedUs, DateStructure& date) noexcept
{
    constexpr auto MonthDaysAcc = []()
        {
            uint32_t mdays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
            std::array<uint32_t, 12> days = {};
            uint32_t day = 0;
            for (uint32_t i = 0; i < 12; ++i)
            {
                days[i] = day;
                day += mdays[i];
            }
            return days;
        }();
    const auto microseconds = (static_cast<uint64_t>(0x8000000000000000) & (encodedUs << 1)) | (static_cast<uint64_t>(0x7fffffffffffffff) & encodedUs);
    const std::chrono::microseconds timeUs{ static_cast<int64_t>(microseconds) };
    const std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> tp{ timeUs };
    const auto tpday = std::chrono::floor<
#if SYSCOMMON_DATE == 1
        std::chrono::days
#else
        std::chrono::duration<int, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>
#endif
    >(tp);
    date::year_month_weekday ymwd{ tpday };
    date.Base.tm_year = static_cast<int>(ymwd.year()) - 1900;
    date.Base.tm_mon = static_cast<int>(static_cast<unsigned>(ymwd.month()) - 1);
    date.Base.tm_wday = static_cast<int>(ymwd.weekday().c_encoding());
    date::year_month_day ymd{ tpday };
    date.Base.tm_mday = static_cast<int>(static_cast<unsigned>(ymd.day()));
    date.Base.tm_yday = static_cast<int>(MonthDaysAcc[date.Base.tm_mon] + date.Base.tm_mday - 1);
    if (date.Base.tm_mon > 1 && ymd.year().is_leap())
        date.Base.tm_yday++;
    date::hh_mm_ss<std::chrono::microseconds> hms{ tp - tpday };
    date.Base.tm_hour = hms.hours().count();
    date.Base.tm_min = hms.minutes().count();
    date.Base.tm_sec = static_cast<int>(hms.seconds().count());
    date.MicroSeconds = gsl::narrow_cast<uint32_t>(hms.subseconds().count());
    if (timezone)
    {
        SetZoneInfo(*timezone, std::chrono::time_point<date::local_t, std::chrono::microseconds>(timeUs), date);
    }
    else
    {
        date.Base.tm_isdst = -1;
    }
}
void ZonedDate::ConvertToDate(const common::date::time_zone* zone, uint64_t encodedUS, DateStructure& date) noexcept
{
    FillTime(zone, encodedUS, date);
}

struct FormatCookie
{
    const ParseResultCommon::NamedArgType* NamedArg = nullptr;
    int16_t ArgIdx = 0;
};


template<typename Char, typename Host>
forceinline static void OnCustomArg(Host& host, std::basic_string<Char>& dst, const detail::FmtWithPair& fmtPair, const FormatSpec* spec)
{
    WrapExecutor<Char> executor{ host };
    typename WrapExecutor<Char>::Context context{ dst };
    fmtPair.Executor(fmtPair.Data, executor, context, spec);
}


template<typename Char>
void Formatter<Char>::DirectFormatTo_(std::basic_string<Char>& dst, const detail::FmtWithPair& fmtPair, const FormatSpec* spec)
{
    OnCustomArg<Char>(*this, dst, fmtPair, spec);
}
template struct Formatter<char>;
template struct Formatter<wchar_t>;
template struct Formatter<char16_t>;
template struct Formatter<char32_t>;
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template struct Formatter<char8_t>;
#endif

static_assert(enum_cast(RealSizeInfo::Byte1) == enum_cast(FormatterExecutor::StringType::UTF8));
static_assert(enum_cast(RealSizeInfo::Byte2) == enum_cast(FormatterExecutor::StringType::UTF16));
static_assert(enum_cast(RealSizeInfo::Byte4) == enum_cast(FormatterExecutor::StringType::UTF32));

template<typename Char, typename Host, typename Spec>
forceinline static void UniversalOnArg(Host& host, std::basic_string<Char>& dst, std::basic_string_view<Char> fmtString, ArgDispType fmtType, const uint16_t* argPtr, ArgRealType argType, const Spec& spec, const FormatCookie& cookie) try
{
    const auto realType = argType & ArgRealType::BaseTypeMask;
    const auto intSize = static_cast<RealSizeInfo>(enum_cast(argType & ArgRealType::TypeSizeMask) >> 4);
    switch (realType)
    {
    case ArgRealType::String:
    {
        const bool noLen = enum_cast(argType & ArgRealType::StrPtrBit);
        const auto ptr = noLen ? *reinterpret_cast<const uintptr_t*>(argPtr) : reinterpret_cast<const std::pair<uintptr_t, size_t>*>(argPtr)->first;
        IF_UNLIKELY(fmtType == ArgDispType::Pointer)
        {
            host.PutPointer(dst, ptr, spec);
        }
        else
        {
            ABORT_CHECK(fmtType == ArgDispType::String || fmtType == ArgDispType::Any);
            const auto lenPtr = noLen ? nullptr : &reinterpret_cast<const std::pair<uintptr_t, size_t>*>(argPtr)->second;
            size_t len = 0;
            const auto sizeType = static_cast<RealSizeInfo>(enum_cast(intSize) & 0x3); // remove StrPtrBit
            switch (sizeType)
            {
            case RealSizeInfo::Byte1: len = lenPtr ? *lenPtr : std::char_traits<char    >::length(reinterpret_cast<const char    *>(ptr)); break;
            case RealSizeInfo::Byte2: len = lenPtr ? *lenPtr : std::char_traits<char16_t>::length(reinterpret_cast<const char16_t*>(ptr)); break;
            case RealSizeInfo::Byte4: len = lenPtr ? *lenPtr : std::char_traits<char32_t>::length(reinterpret_cast<const char32_t*>(ptr)); break;
            default: CM_UNREACHABLE(); break;
            }
            host.PutString(dst, reinterpret_cast<const void*>(ptr), len, static_cast<FormatterExecutor::StringType>(sizeType), spec);
        }
    } return;
    case ArgRealType::Bool:
    {
        const auto val = *reinterpret_cast<const uint16_t*>(argPtr);
        if (fmtType == ArgDispType::Char)
        {
            if constexpr (sizeof(Char) == 1)
            {
                const auto str = val ? "Y"sv : "N"sv;
                host.PutString(dst, str.data(), str.size(), FormatterExecutor::StringType::UTF8, spec);
            }
            else if constexpr (sizeof(Char) == 2)
            {
                const auto str = val ? u"Y"sv : u"N"sv;
                host.PutString(dst, str.data(), str.size(), FormatterExecutor::StringType::UTF16, spec);
            }
            else if constexpr (sizeof(Char) == 4)
            {
                const auto str = val ? U"Y"sv : U"N"sv;
                host.PutString(dst, str.data(), str.size(), FormatterExecutor::StringType::UTF32, spec);
            }
            else
                static_assert(!AlwaysTrue<Char>, "unsupported char size");
        }
        else IF_UNLIKELY(fmtType == ArgDispType::Integer || fmtType == ArgDispType::Numeric)
        {
            host.PutInteger(dst, val ? 1u : 0u, false, spec);
        }
        ELSE_LIKELY
        {
            ABORT_CHECK(fmtType == ArgDispType::String || fmtType == ArgDispType::Any);
            if constexpr (sizeof(Char) == 1)
            {
                const auto str = val ? "true"sv : "false"sv;
                host.PutString(dst, str.data(), str.size(), FormatterExecutor::StringType::UTF8, spec);
            }
            else if constexpr (sizeof(Char) == 2)
            {
                const auto str = val ? u"true"sv : u"false"sv;
                host.PutString(dst, str.data(), str.size(), FormatterExecutor::StringType::UTF16, spec);
            }
            else if constexpr (sizeof(Char) == 4)
            {
                const auto str = val ? U"true"sv : U"false"sv;
                host.PutString(dst, str.data(), str.size(), FormatterExecutor::StringType::UTF32, spec);
            }
            else
                static_assert(!AlwaysTrue<Char>, "unsupported char size");
        }
    } return;
    case ArgRealType::Ptr:
    {
        ABORT_CHECK(fmtType == ArgDispType::Pointer || fmtType == ArgDispType::Any);
        const auto val = *reinterpret_cast<const uintptr_t*>(argPtr);
        host.PutPointer(dst, val, spec);
    } return;
    case ArgRealType::Date:
        if constexpr (std::is_same_v<Spec, const FormatSpec*>)
        {
            ABORT_CHECK(fmtType == ArgDispType::Date || fmtType == ArgDispType::Any);
            std::basic_string_view<Char> fmtStr{};
            if (spec && spec->FmtLen)
                fmtStr = fmtString.substr(spec->FmtOffset, spec->FmtLen);
            DateStructure date;
            if (HAS_FIELD(argType, ArgRealType::DateDeltaBit))
            {
                const date::time_zone* timezone = nullptr;
                uint64_t us = 0;
                if (HAS_FIELD(argType, ArgRealType::DateZoneBit))
                {
                    const auto& val = *reinterpret_cast<const ZonedDate*>(argPtr);
                    us = val.Microseconds;
                    timezone = val.TimeZone;
                }
                else
                {
                    us = *reinterpret_cast<const uint64_t*>(argPtr);
                }
                FillTime(timezone, us, date);
            }
            else
            {
                if (HAS_FIELD(argType, ArgRealType::DateZoneBit))
                    reinterpret_cast<const CompactDateEx*>(argPtr)->FillInto(date);
                else
                    reinterpret_cast<const CompactDate*>(argPtr)->FillInto(date);
            }
            host.PutDate(dst, fmtStr, date);
        }
        else
        {
            Expects(false);
        } return;
    case ArgRealType::Color:
        if constexpr (std::is_same_v<Spec, const FormatSpec*>)
        {
            ABORT_CHECK(fmtType == ArgDispType::Color || fmtType == ArgDispType::Any);
            switch (intSize)
            {
            case RealSizeInfo::Byte1: host.PutColorArg(dst, { false, *reinterpret_cast<const CommonColor*>(argPtr) }, spec); break;
            case RealSizeInfo::Byte4: host.PutColorArg(dst,          *reinterpret_cast<const ScreenColor*>(argPtr)  , spec); break;
            default: CM_UNREACHABLE(); break;
            }
        }
        else
        {
            Expects(false);
        } return;
    case ArgRealType::Custom:
        if constexpr (std::is_same_v<Spec, const FormatSpec*>)
        {
            const auto& pair = *reinterpret_cast<const detail::FmtWithPair*>(argPtr);
            WrapExecutor<Char> executor{ host };
            typename WrapExecutor<Char>::Context context2{ dst };
            pair.Executor(pair.Data, executor, context2, spec);
        }
        else
        {
            Expects(false);
        } return;
    case ArgRealType::Float:
    {
        switch (intSize)
        {
        case RealSizeInfo::Byte2: host.PutFloat(dst, static_cast<float>(*reinterpret_cast<const half_float::half*>(argPtr)), spec); break;
        case RealSizeInfo::Byte4: host.PutFloat(dst,                    *reinterpret_cast<const            float*>(argPtr) , spec); break;
        case RealSizeInfo::Byte8: host.PutFloat(dst,                    *reinterpret_cast<const           double*>(argPtr) , spec); break;
        default: CM_UNREACHABLE(); break;
        }
    } return;
    // Below are integer
    case ArgRealType::Char:
    {
        IF_LIKELY(fmtType == ArgDispType::Char || fmtType == ArgDispType::Any)
        {
            host.PutString(dst, argPtr, 1, static_cast<FormatterExecutor::StringType>(intSize), spec);
            return;
        }
    }
    [[fallthrough]];
    case ArgRealType::SInt:
    case ArgRealType::UInt:
    {
        ABORT_CHECK(fmtType == ArgDispType::Integer || fmtType == ArgDispType::Numeric || fmtType == ArgDispType::Any);
        const auto isSigned = realType == ArgRealType::SInt;
        switch (intSize)
        {
        case RealSizeInfo::Byte1: host.PutInteger(dst, static_cast<uint32_t>(*reinterpret_cast<const uint8_t *>(argPtr)), isSigned, spec); break;
        case RealSizeInfo::Byte2: host.PutInteger(dst, static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(argPtr)), isSigned, spec); break;
        case RealSizeInfo::Byte4: host.PutInteger(dst, static_cast<uint32_t>(*reinterpret_cast<const uint32_t*>(argPtr)), isSigned, spec); break;
        case RealSizeInfo::Byte8: host.PutInteger(dst, static_cast<uint64_t>(*reinterpret_cast<const uint64_t*>(argPtr)), isSigned, spec); break;
        default: CM_UNREACHABLE(); break;
        }
    } return;
    default:
        CM_UNREACHABLE(); return;
    }
}
catch (const fmt::format_error& err)
{
    std::u16string argName;
    if (cookie.ArgIdx < 0 && cookie.NamedArg)
    {
        argName = to_u16string(fmtString.substr(cookie.NamedArg->Offset, cookie.NamedArg->Length));
    }
    else
    {
        const auto idxtxt = std::to_string(cookie.ArgIdx);
        argName.assign(idxtxt.begin(), idxtxt.end());
    }
    std::u16string msg = u"Error when formatting [" + argName + u"]: ";
    const auto fmtMsg = err.what();
    msg.append(fmtMsg, fmtMsg + std::char_traits<char>::length(fmtMsg));
    auto ex = CREATE_EXCEPTIONEX(ArgFormatException, msg, cookie.ArgIdx, fmtType, argType);
    ex.Attach("argName", argName);
    throw ex;
}

template<typename Char>
forceinline void FormatterBase::StaticExecutor<Char>::OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader)
{
    ArgDispType fmtType = ArgDispType::Any;
    const uint16_t* argPtr = nullptr;
    ArgRealType argType = ArgRealType::Error;
    FormatCookie cookie;
    if (isNamed)
    {
        cookie.ArgIdx = static_cast<int16_t>(-argIdx);
        cookie.NamedArg = &context.StrInfo.NamedTypes[argIdx];
        fmtType = cookie.NamedArg->Type;
        const auto mapIdx = context.Mapping[argIdx];
        argType = context.TheArgInfo.NamedTypes[mapIdx];
        const auto argSlot = mapIdx + context.TheArgInfo.IdxArgCount;
        argPtr = context.ArgStore.data() + context.ArgStore[argSlot];
    }
    else
    {
        cookie.ArgIdx = argIdx;
        fmtType = context.StrInfo.IndexTypes[argIdx];
        argType = context.TheArgInfo.IndexTypes[argIdx];
        const auto argSlot = argIdx;
        argPtr = context.ArgStore.data() + context.ArgStore[argSlot];
    }
    const auto spec = reader.ReadSpec();
    UniversalOnArg<Char>(Fmter, context.Dst, context.StrInfo.FormatString, fmtType, argPtr, argType, spec, cookie);
}
template SYSCOMMONTPL void FormatterBase::StaticExecutor<char>    ::OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader);
template SYSCOMMONTPL void FormatterBase::StaticExecutor<wchar_t> ::OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader);
template SYSCOMMONTPL void FormatterBase::StaticExecutor<char16_t>::OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader);
template SYSCOMMONTPL void FormatterBase::StaticExecutor<char32_t>::OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader);
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template SYSCOMMONTPL void FormatterBase::StaticExecutor<char8_t> ::OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader);
#endif


bool FormatSpecCacher::Cache(const StrArgInfo& strInfo, const ArgInfo& argInfo, const NamedMapper& mapper) noexcept
{
    struct RecordExecutor : private FormatterBase
    {
        struct Context {};
        FormatSpecCacher& Cache;
        const StrArgInfo& StrInfo;
        const ArgInfo& RealArgInfo;
        const NamedMapper& Mapper;

        void OnFmtStr(Context&, uint32_t, uint32_t) {}
        void OnBrace(Context&, bool) {}
        void OnColor(Context&, ScreenColor) {}
        void OnArg(Context&, uint8_t argIdx, bool isNamed, SpecReader& reader)
        {
            const auto dispType = isNamed ? StrInfo.NamedTypes[argIdx].Type : StrInfo.IndexTypes[argIdx];
            const auto realType = isNamed ? RealArgInfo.NamedTypes[Mapper[argIdx]] : RealArgInfo.IndexTypes[argIdx];
            if (const auto realType2 = realType & ArgRealType::BaseTypeMask; 
                realType2 != ArgRealType::Custom && realType2 != ArgRealType::Color && realType2 != ArgRealType::Date)
            {
                const auto bitIndex = (isNamed ? StrInfo.NamedTypes.size() : 0) + argIdx;
                auto& target = isNamed ? Cache.NamedSpec : Cache.IndexSpec;
                target.resize(std::max<size_t>(target.size(), argIdx + 1));
                Cache.CachedBitMap.Set(bitIndex, FormatterExecutor::ConvertSpec(target[argIdx], reader.ReadSpec(), realType, dispType));
            }
        }

        constexpr RecordExecutor(FormatSpecCacher& cache, const StrArgInfo& strInfo, const ArgInfo& argInfo, const NamedMapper& mapper) noexcept :
            Cache(cache), StrInfo(strInfo), RealArgInfo(argInfo), Mapper(mapper) {}
        void Execute()
        {
            Context context;
            auto opcodes = StrInfo.Opcodes;
            FormatterBase::Execute(opcodes, *this, context);
        }
        using FormatterBase::Execute;
    };
    IndexSpec.clear();
    IndexSpec.reserve(strInfo.IndexTypes.size());
    NamedSpec.clear();
    NamedSpec.reserve(strInfo.NamedTypes.size());
    CachedBitMap = SmallBitset(strInfo.IndexTypes.size() + strInfo.NamedTypes.size(), false);
    RecordExecutor executor{ *this, strInfo, argInfo, mapper };
    executor.Execute();
    return true;
}

template<typename Char>
void FormatSpecCacherCh<Char>::FormatTo(std::basic_string<Char>& dst, span<const uint16_t> argStore)
{
    struct RecordedSpecExecutor final : public Formatter<Char>
    {
        struct Context
        {
            std::basic_string<Char>& Dst;
            span<const uint16_t> ArgStore;
        };
        const FormatSpecCacherCh<Char>& Cache;

        void PutString(std::basic_string<Char>& ret, const void* str, size_t len, FormatterExecutor::StringType type, const OpaqueFormatSpec& spec)
        {
            CommonFormatter<Char>::PutString(ret, str, len, type, &spec);
        }
        void PutInteger(std::basic_string<Char>& ret, uint32_t val, bool isSigned, const OpaqueFormatSpec& spec)
        {
            CommonFormatter<Char>::PutInteger(ret, val, isSigned, spec);
        }
        void PutInteger(std::basic_string<Char>& ret, uint64_t val, bool isSigned, const OpaqueFormatSpec& spec)
        {
            CommonFormatter<Char>::PutInteger(ret, val, isSigned, spec);
        }
        void PutFloat  (std::basic_string<Char>& ret, float  val, const OpaqueFormatSpec& spec)
        {
            CommonFormatter<Char>::PutFloat(ret, val, spec);
        }
        void PutFloat  (std::basic_string<Char>& ret, double val, const OpaqueFormatSpec& spec)
        {
            CommonFormatter<Char>::PutFloat(ret, val, spec);
        }
        void PutPointer(std::basic_string<Char>& ret, uintptr_t val, const OpaqueFormatSpec& spec)
        {
            CommonFormatter<Char>::PutPointer(ret, val, spec);
        }
        using Formatter<Char>::PutString;
        using Formatter<Char>::PutInteger;
        using Formatter<Char>::PutFloat;
        using Formatter<Char>::PutPointer;

        forceinline void OnFmtStr(Context& context, uint32_t offset, uint32_t length)
        {
            context.Dst.append(Cache.StrInfo.FormatString.data() + offset, length);
        }
        forceinline void OnBrace(Context& context, bool isLeft)
        {
            context.Dst.push_back(ParseLiterals<Char>::BracePair[isLeft ? 0 : 1]);
        }
        forceinline void OnColor(Context& context, ScreenColor color)
        {
            this->PutColor(context.Dst, color);
        }
        void OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader)
        {
            ArgDispType fmtType = ArgDispType::Any;
            const uint16_t* argPtr = nullptr;
            ArgRealType argType = ArgRealType::Error;
            FormatCookie cookie;
            if (isNamed)
            {
                cookie.ArgIdx = static_cast<int16_t>(-argIdx);
                cookie.NamedArg = &Cache.StrInfo.NamedTypes[argIdx];
                fmtType = cookie.NamedArg->Type;
                const auto mapIdx = Cache.Mapping[argIdx];
                argType = Cache.TheArgInfo.NamedTypes[mapIdx];
                const auto argSlot = mapIdx + Cache.TheArgInfo.IdxArgCount;
                argPtr = context.ArgStore.data() + context.ArgStore[argSlot];
            }
            else
            {
                cookie.ArgIdx = argIdx;
                fmtType = Cache.StrInfo.IndexTypes[argIdx];
                argType = Cache.TheArgInfo.IndexTypes[argIdx];
                const auto argSlot = argIdx;
                argPtr = context.ArgStore.data() + context.ArgStore[argSlot];
            }
            const auto bitIndex = (isNamed ? Cache.StrInfo.NamedTypes.size() : 0) + argIdx;
            if (Cache.CachedBitMap.Get(bitIndex))
            {
                auto& target = isNamed ? Cache.NamedSpec : Cache.IndexSpec;
                UniversalOnArg<Char>(*this, context.Dst, Cache.StrInfo.FormatString, fmtType, argPtr, argType, target[argIdx], cookie);
            }
            else
            {
                const auto spec = reader.ReadSpec();
                UniversalOnArg<Char>(*this, context.Dst, Cache.StrInfo.FormatString, fmtType, argPtr, argType, spec, cookie);
            }
        }

        constexpr RecordedSpecExecutor(const FormatSpecCacherCh<Char>& cache) noexcept : Cache(cache) {}
        using FormatterBase::Execute;
    };
    typename RecordedSpecExecutor::Context context{ dst, argStore };
    RecordedSpecExecutor executor{ *this };
    auto opcodes = StrInfo.Opcodes;
    executor.Execute(opcodes, executor, context);
}
template SYSCOMMONTPL void FormatSpecCacherCh<char>    ::FormatTo(std::basic_string<char>    & dst, span<const uint16_t> argStore);
template SYSCOMMONTPL void FormatSpecCacherCh<wchar_t> ::FormatTo(std::basic_string<wchar_t> & dst, span<const uint16_t> argStore);
template SYSCOMMONTPL void FormatSpecCacherCh<char16_t>::FormatTo(std::basic_string<char16_t>& dst, span<const uint16_t> argStore);
template SYSCOMMONTPL void FormatSpecCacherCh<char32_t>::FormatTo(std::basic_string<char32_t>& dst, span<const uint16_t> argStore);
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template SYSCOMMONTPL void FormatSpecCacherCh<char8_t> ::FormatTo(std::basic_string<char8_t> & dst, span<const uint16_t> argStore);
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
#if COMMON_COMPILER_GCC && COMMON_GCC_VER < 100000
#   pragma GCC diagnostic pop
#endif

