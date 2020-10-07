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


common::str::StrVariant<char32_t> Arg::ToString() const noexcept
{
    return Visit([](const auto val) -> common::str::StrVariant<char32_t>
        {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, CustomVar>)
                return {};
            else if constexpr (std::is_same_v<T, std::nullopt_t>)
                return {};
            else if constexpr (std::is_same_v<T, std::u32string_view>)
                return val;
            else
                return fmt::format(FMT_STRING(U"{}"), val);
        });
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
