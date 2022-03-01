#include "SystemCommonPch.h"
#include "Format.h"
#include "Exceptions.h"
#include "StringFormat.h"
#include "StringConvert.h"

namespace common::str::exp
{
using namespace std::string_view_literals;


ArgPack::NamedMapper ArgChecker::CheckDD(const StrArgInfo& strInfo, const ArgInfo& argInfo)
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
    ArgPack::NamedMapper mapper = { 0 };
    if (strNamedArgCount > 0)
    {
        const auto namedRet = ArgChecker::GetNamedArgMismatch(
            strInfo.NamedTypes.data(), strInfo.FormatString, strNamedArgCount,
            argInfo.NamedTypes, argInfo.Names, argInfo.NamedArgCount);
        if (namedRet.AskIndex)
        {
            const auto& namedArg = strInfo.NamedTypes[*namedRet.AskIndex];
            const auto name = strInfo.FormatString.substr(namedArg.Offset, namedArg.Length);
            if (namedRet.GiveIndex) // type mismatch
                COMMON_THROW(BaseException, FMTSTR(u"NamedArg [{}] type mismatch"sv, name)).Attach("arg", name)
                    .Attach("argType", std::pair{ namedArg.Type, ArgInfo::CleanRealType(argInfo.NamedTypes[*namedRet.GiveIndex]) });
            else // arg not found
                COMMON_THROW(BaseException, FMTSTR(u"NamedArg [{}] not found in args"sv, name)).Attach("arg", name);
        }
        Ensures(!namedRet.GiveIndex);
        mapper = namedRet.Mapper;
    }
    return mapper;
}


struct FormatterHelper : public FormatterBase
{
    template<typename Char>
    static constexpr fmt::basic_format_specs<Char> ConvertSpec(const FormatSpec& in)
    {
        fmt::basic_format_specs<Char> spec;
        spec.width = in.Width;
        spec.precision = static_cast<int32_t>(in.Precision);
        spec.alt = in.AlterForm;
        spec.localized = false;
        spec.fill[0] = static_cast<Char>(in.Fill);
        switch (in.Alignment)
        {
        case FormatSpec::Align::None:   spec.align = fmt::align::none;   break;
        case FormatSpec::Align::Left:   spec.align = fmt::align::left;   break;
        case FormatSpec::Align::Middle: spec.align = fmt::align::center; break;
        case FormatSpec::Align::Right:  spec.align = fmt::align::right;  break;
        default: break;
        }
        switch (in.SignFlag)
        {
        case FormatSpec::Sign::None:  spec.sign = fmt::sign::none;  break;
        case FormatSpec::Sign::Neg:   spec.sign = fmt::sign::minus; break;
        case FormatSpec::Sign::Pos:   spec.sign = fmt::sign::plus;  break;
        case FormatSpec::Sign::Space: spec.sign = fmt::sign::space; break;
        default: break;
        }
        return spec;
    }
};

void Formatter::PutColor(std::string&, bool, Color) const { }
void Formatter::PutString(std::string& ret, std::   string_view str, const FormatSpec* spec) const
{
    if (spec)
    {
        auto fmtSpec = FormatterHelper::ConvertSpec<char>(*spec);
        fmtSpec.type = fmt::presentation_type::string;
        fmt::detail::write<char>(std::back_inserter(ret), str, fmtSpec);
        //fmt::format_to(std::back_inserter(ret), "");
    }
    else
        ret.append(str);
}
void Formatter::PutString(std::string& ret, std::u16string_view str, const FormatSpec* spec) const
{
    return PutString(ret, str::to_string(str, str::Encoding::UTF8), spec);
}
void Formatter::PutString(std::string& ret, std::u32string_view str, const FormatSpec* spec) const
{
    return PutString(ret, str::to_string(str, str::Encoding::UTF8), spec);
}

void FormatterBase::FormatTo(const Formatter& formatter, std::string& ret, const StrArgInfo& strInfo, const ArgInfo& argInfo, const ArgPack& argPack)
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
                case 0:  formatter.PutString(ret, "{"sv, nullptr); break;
                case 1:  formatter.PutString(ret, "}"sv, nullptr); break;
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
            ArgDispType fmtType = ArgDispType::Any;
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
            switch (realType)
            {
            case ArgRealType::String:
            {
                const auto specPtr = spec ? &*spec : nullptr;
                const auto [ptr, len] = *reinterpret_cast<const std::pair<uintptr_t, size_t>*>(argPtr);
                switch (enum_cast(argType & ArgRealType::SizeMask4))
                {
                case 0x0:
                    formatter.PutString(ret, std::   string_view{ reinterpret_cast<const char    *>(ptr), len }, specPtr);
                    break;
                case 0x1:
                    formatter.PutString(ret, std::u16string_view{ reinterpret_cast<const char16_t*>(ptr), len }, specPtr);
                    break;
                case 0x2:
                    formatter.PutString(ret, std::u32string_view{ reinterpret_cast<const char32_t*>(ptr), len }, specPtr);
                    break;
                default:
                    break;
                }
            } break;
            case ArgRealType::Char:
            {
                const auto specPtr = spec ? &*spec : nullptr;
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
            default: break;
            }
        } break;
        default:
            break;
        }
    }
}



}

