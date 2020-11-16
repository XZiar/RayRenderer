#include "NailangStruct.h"
#include "StringUtil/Format.h"
#include "common/AlignedBase.hpp"
#include <cassert>

namespace xziar::nailang
{
using namespace std::string_view_literals;


std::pair<size_t, size_t> MemoryPool::Usage() const noexcept
{
    size_t used = 0, unused = 0;
    for (const auto& trunk : Trunks)
    {
        used += trunk.Offset, unused += trunk.Avaliable;
    }
    return { used, used + unused };
}


static constexpr PartedName::PartType DummyPart[1] = { {(uint16_t)0, (uint16_t)0} };
TempPartedNameBase::TempPartedNameBase(std::u32string_view name, common::span<const PartedName::PartType> parts, uint16_t info)
    : Var(name, parts, info)
{
    Expects(parts.size() > 0);
    constexpr auto SizeP = sizeof(PartedName::PartType);
    const auto partSize = parts.size() * SizeP;
    switch (parts.size())
    {
    case 1:
        break;
    case 2:
    case 3:
    case 4:
        memcpy_s(Extra.Parts, partSize, parts.data(), partSize);
        break;
    default:
    {
        const auto ptr = reinterpret_cast<PartedName*>(malloc(sizeof(PartedName) + partSize));
        Extra.Ptr = ptr;
        new (ptr) PartedName(name, parts, info);
        memcpy_s(ptr + 1, partSize, parts.data(), partSize);
        break;
    }
    }
}
TempPartedNameBase::TempPartedNameBase(const PartedName* var) noexcept : Var(*var, DummyPart, var->ExternInfo)
{
    Extra.Ptr = var;
}
TempPartedNameBase::TempPartedNameBase(TempPartedNameBase&& other) noexcept :
    Var(other.Var, DummyPart, other.Var.ExternInfo), Extra(other.Extra)
{
    Var.PartCount = other.Var.PartCount;
    other.Var.Ptr = nullptr;
    other.Var.PartCount = 0;
    other.Extra.Ptr = nullptr;
}
TempPartedNameBase::~TempPartedNameBase()
{
    if (Var.PartCount > 4)
        free(const_cast<PartedName*>(Extra.Ptr));
}
TempPartedNameBase TempPartedNameBase::Copy() const noexcept
{
    common::span<const PartedName::PartType> parts;
    if (Var.PartCount > 4 || Var.PartCount == 0)
    {
        parts = { reinterpret_cast<const PartedName::PartType*>(Extra.Ptr + 1), Extra.Ptr->PartCount };
    }
    else
    {
        parts = { Extra.Parts, Var.PartCount };
    }
    return TempPartedNameBase(Var, parts, Var.ExternInfo);
}


std::u32string_view EmbedOpHelper::GetOpName(EmbedOps op) noexcept
{
    switch (op)
    {
    #define RET_NAME(op) case EmbedOps::op: return U"" STRINGIZE(op)
    RET_NAME(Equal);
    RET_NAME(NotEqual);
    RET_NAME(Less);
    RET_NAME(LessEqual);
    RET_NAME(Greater);
    RET_NAME(GreaterEqual);
    RET_NAME(And);
    RET_NAME(Or);
    RET_NAME(Not);
    RET_NAME(Add);
    RET_NAME(Sub);
    RET_NAME(Mul);
    RET_NAME(Div);
    RET_NAME(Rem);
    default: return U"Unknwon"sv;
    }
}


std::u32string_view RawArg::TypeName(const RawArg::Type type) noexcept
{
    switch (type)
    {
    case RawArg::Type::Empty:   return U"empty"sv;
    case RawArg::Type::Func:    return U"func-call"sv;
    case RawArg::Type::Unary:   return U"unary-expr"sv;
    case RawArg::Type::Binary:  return U"binary-expr"sv;
    case RawArg::Type::Indexer: return U"indexer-expr"sv;
    case RawArg::Type::Var:     return U"variable"sv;
    case RawArg::Type::Str:     return U"string"sv;
    case RawArg::Type::Uint:    return U"uint"sv;
    case RawArg::Type::Int:     return U"int"sv;
    case RawArg::Type::FP:      return U"fp"sv;
    case RawArg::Type::Bool:    return U"bool"sv;
    default:                    return U"error"sv;
    }
}


Arg::Arg(const Arg& other) noexcept :
    Data0(other.Data0), Data1(other.Data1), Data2(other.Data2), Data3(other.Data3), TypeData(other.TypeData)
{
    if (TypeData == Type::U32Str && Data1 > 0)
    {
        auto str = other.GetVar<Type::U32Str>();
        common::SharedString<char32_t>::PlacedIncrease(str);
    }
    else if (TypeData == Type::Var)
    {
        auto var = GetVar<Type::Var>();
        var.Host->IncreaseRef(var);
    }
}

Arg& Arg::operator=(const Arg& other) noexcept
{
    Release();
    Data0 = other.Data0;
    Data1 = other.Data1;
    Data2 = other.Data2;
    Data3 = other.Data3;
    TypeData = other.TypeData;
    if (TypeData == Type::U32Str && Data1 > 0)
    {
        auto str = other.GetVar<Type::U32Str>();
        common::SharedString<char32_t>::PlacedIncrease(str);
    }
    else if (TypeData == Type::Var)
    {
        auto var = GetVar<Type::Var>();
        var.Host->IncreaseRef(var);
    }
    return *this;
}

void Arg::Release() noexcept
{
    if (TypeData == Type::U32Str && Data1 > 0)
    {
        auto str = GetVar<Type::U32Str>();
        common::SharedString<char32_t>::PlacedDecrease(str);
    }
    else if (TypeData == Type::Var)
    {
        auto var = GetVar<Type::Var>();
        var.Host->DecreaseRef(var);
    }
    TypeData = Type::Empty;
}

std::optional<bool> Arg::GetBool() const noexcept
{
    switch (TypeData)
    {
    case Type::Uint:    return  GetVar<Type::Uint>() != 0;
    case Type::Int:     return  GetVar<Type::Int>() != 0;
    case Type::FP:      return  GetVar<Type::FP>() != 0;
    case Type::Bool:    return  GetVar<Type::Bool>();
    case Type::U32Str:  return !GetVar<Type::U32Str>().empty();
    case Type::U32Sv:   return !GetVar<Type::U32Sv>().empty();
    case Type::Var:     return  ConvertCustomType(Type::Bool).GetBool();
    default:            return {};
    }
}
std::optional<uint64_t> Arg::GetUint() const noexcept
{
    switch (TypeData)
    {
    case Type::Uint:    return GetVar<Type::Uint>();
    case Type::Int:     return static_cast<uint64_t>(GetVar<Type::Int>());
    case Type::FP:      return static_cast<uint64_t>(GetVar<Type::FP>());
    case Type::Bool:    return GetVar<Type::Bool>() ? 1 : 0;
    case Type::Var:     return ConvertCustomType(Type::Uint).GetUint();
    default:            return {};
    }
}
std::optional<int64_t> Arg::GetInt() const noexcept
{
    switch (TypeData)
    {
    case Type::Uint:    return static_cast<int64_t>(GetVar<Type::Uint>());
    case Type::Int:     return GetVar<Type::Int>();
    case Type::FP:      return static_cast<int64_t>(GetVar<Type::FP>());
    case Type::Bool:    return GetVar<Type::Bool>() ? 1 : 0;
    case Type::Var:     return ConvertCustomType(Type::Int).GetInt();
    default:            return {};
    }
}
std::optional<double> Arg::GetFP() const noexcept
{
    switch (TypeData)
    {
    case Type::Uint:    return static_cast<double>(GetVar<Type::Uint>());
    case Type::Int:     return static_cast<double>(GetVar<Type::Int>());
    case Type::FP:      return GetVar<Type::FP>();
    case Type::Bool:    return GetVar<Type::Bool>() ? 1. : 0.;
    case Type::Var:     return ConvertCustomType(Type::FP).GetFP();
    default:            return {};
    }
}
std::optional<std::u32string_view> Arg::GetStr() const noexcept
{
    switch (TypeData)
    {
    case Type::U32Str:  return GetVar<Type::U32Str>();
    case Type::U32Sv:   return GetVar<Type::U32Sv>();
    case Type::Var:     return ConvertCustomType(Type::U32Sv).GetStr();
    default:            return {};
    }
}

common::str::StrVariant<char32_t> Arg::ToString() const noexcept
{
    return Visit([](const auto val) -> common::str::StrVariant<char32_t>
        {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, CustomVar>)
                return val.Host->ToString(val);
            else if constexpr (std::is_same_v<T, std::nullopt_t>)
                return {};
            else if constexpr (std::is_same_v<T, std::u32string_view>)
                return val;
            else
                return fmt::format(FMT_STRING(U"{}"), val);
        });
}

std::u32string_view Arg::TypeName(const Arg::Type type) noexcept
{
    switch (type)
    {
    case Arg::Type::Empty:    return U"empty"sv;
    case Arg::Type::Var:      return U"variable"sv;
    case Arg::Type::U32Str:   return U"string"sv;
    case Arg::Type::U32Sv:    return U"string_view"sv;
    case Arg::Type::Uint:     return U"uint"sv;
    case Arg::Type::Int:      return U"int"sv;
    case Arg::Type::FP:       return U"fp"sv;
    case Arg::Type::Bool:     return U"bool"sv;
    case Arg::Type::Custom:   return U"custom"sv;
    case Arg::Type::Boolable: return U"boolable"sv;
    case Arg::Type::String:   return U"string-ish"sv;
    case Arg::Type::Number:   return U"number"sv;
    case Arg::Type::Integer:  return U"integer"sv;
    default:                  return U"error"sv;
    }
}


void Serializer::Stringify(std::u32string& output, const RawArg& arg, const bool requestParenthese)
{
    arg.Visit([&](const auto& data)
        {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, std::nullopt_t>)
                Expects(false);
            else if constexpr (std::is_same_v<T, const BinaryExpr*>)
                Stringify(output, data, requestParenthese);
            else
                Stringify(output, data);
        });
}

void Serializer::Stringify(std::u32string& output, const FuncCall* call)
{
    output.append(*call->Name).push_back(U'(');
    size_t i = 0;
    for (const auto& arg : call->Args)
    {
        if (i++ > 0)
            output.append(U", "sv);
        Stringify(output, arg);
    }
    output.push_back(U')');
}

void Serializer::Stringify(std::u32string& output, const UnaryExpr* expr)
{
    Expects(EmbedOpHelper::IsUnaryOp(expr->Operator));
    switch (expr->Operator)
    {
    case EmbedOps::Not:
        output.append(U"!"sv);
        Stringify(output, expr->Oprend, true);
        break;
    default:
        Expects(false);
        break;
    }
}

void Serializer::Stringify(std::u32string& output, const BinaryExpr* expr, const bool requestParenthese)
{
    Expects(!EmbedOpHelper::IsUnaryOp(expr->Operator));
    std::u32string_view opStr;
#define SET_OP_STR(op, str) case EmbedOps::op: opStr = U##str##sv; break;
    switch (expr->Operator)
    {
        SET_OP_STR(Equal,       " == ");
        SET_OP_STR(NotEqual,    " != ");
        SET_OP_STR(Less,        " < ");
        SET_OP_STR(LessEqual,   " <= ");
        SET_OP_STR(Greater,     " > ");
        SET_OP_STR(GreaterEqual," >= ");
        SET_OP_STR(And,         " && ");
        SET_OP_STR(Or,          " || ");
        SET_OP_STR(Add,         " + ");
        SET_OP_STR(Sub,         " - ");
        SET_OP_STR(Mul,         " * ");
        SET_OP_STR(Div,         " / ");
        SET_OP_STR(Rem,         " % ");
    default:
        assert(false); // Expects(false);
        return;
    }
#undef SET_OP_STR
    if (requestParenthese)
        output.push_back(U'(');
    Stringify(output, expr->LeftOprend, true);
    output.append(opStr);
    Stringify(output, expr->RightOprend, true);
    if (requestParenthese)
        output.push_back(U')');
}

void Serializer::Stringify(std::u32string& output, const IndexerExpr* expr)
{
    Stringify(output, expr->Target, true);
    output.push_back(U'[');
    Stringify(output, expr->Index, false);
    output.push_back(U']');
}

void Serializer::Stringify(std::u32string& output, const LateBindVar* var)
{
    if (HAS_FIELD(var->Info(), LateBindVar::VarInfo::Root))
        output.push_back(U'`');
    else if (HAS_FIELD(var->Info(), LateBindVar::VarInfo::Local))
        output.push_back(U':');
    output.append(static_cast<std::u32string_view>(*var));
}

void Serializer::Stringify(std::u32string& output, const std::u32string_view str)
{
    output.push_back(U'"');
    output.append(str); //TODO:
    output.push_back(U'"');
}

void Serializer::Stringify(std::u32string& output, const uint64_t u64)
{
    const auto str = std::to_string(u64);
    output.insert(output.end(), str.begin(), str.end());
}

void Serializer::Stringify(std::u32string& output, const int64_t i64)
{
    const auto str = std::to_string(i64);
    output.insert(output.end(), str.begin(), str.end());
}

void Serializer::Stringify(std::u32string& output, const double f64)
{
    const auto str = std::to_string(f64);
    output.insert(output.end(), str.begin(), str.end());
}

void Serializer::Stringify(std::u32string& output, const bool boolean)
{
    output.append(boolean ? U"true"sv : U"false"sv);
}


}
