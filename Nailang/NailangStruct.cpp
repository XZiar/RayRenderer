#include "NailangStruct.h"
#include "3rdParty/fmt/utfext.h"
#include "common/AlignedBase.hpp"
#include <boost/range/adaptor/reversed.hpp>
#include <cassert>

namespace xziar::nailang
{
using namespace std::string_view_literals;

MemoryPool::~MemoryPool()
{
    for ([[maybe_unused]] const auto& [ptr, offset, avaliable] : Trunks)
    {
        common::free_align(ptr);
    }
}

common::span<std::byte> MemoryPool::Alloc(const size_t size, const size_t align)
{
    Expects(align > 0);
    for (auto& [ptr, offset, avaliable] : boost::adaptors::reverse(Trunks))
    {
        if (avaliable > size)
        {
            const auto ptr_ = reinterpret_cast<uintptr_t>(ptr + offset + align - 1) / align * align;
            const auto offset_ = reinterpret_cast<std::byte*>(ptr_) - ptr;
            if (offset_ + size <= offset + avaliable)
            {
                const common::span<std::byte> ret(ptr + offset_, size);
                avaliable -= (offset_ - offset) + size;
                offset = offset_ + size;
                return ret;
            }
        }
    }
    if (size > 16 * TrunkSize)
    {
        auto ptr = reinterpret_cast<std::byte*>(common::malloc_align(size, align));
        Trunks.emplace_back(ptr, size, 0);
        return common::span<std::byte>(ptr, size);
    }
    else
    {
        const auto total = (size + TrunkSize - 1) / TrunkSize * TrunkSize;
        auto ptr = reinterpret_cast<std::byte*>(common::malloc_align(total, align));
        Trunks.emplace_back(ptr, size, total - size);
        return common::span<std::byte>(ptr, size);
    }
}

std::pair<size_t, size_t> MemoryPool::Usage() const noexcept
{
    size_t used = 0, unused = 0;
    for (auto& [ptr, offset, avaliable] : Trunks)
        used += offset, unused += avaliable;
    return { used, used + unused };
}


common::str::StrVariant<char32_t> Arg::ToString() const noexcept
{
    return Visit([](const auto val) -> common::str::StrVariant<char32_t>
        {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, CustomVar>)
                return {};
            else if constexpr (std::is_same_v<T, std::optional<bool>>)
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
            if constexpr (std::is_same_v<T, std::optional<bool>>)
                Expects(false);
            else if constexpr (std::is_same_v<T, const BinaryExpr*>)
                Stringify(output, data, requestParenthese);
            else
                Stringify(output, data);
        });
}

void Serializer::Stringify(std::u32string& output, const FuncCall* call)
{
    output.append(call->Name).push_back(U'(');
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

void Serializer::Stringify(std::u32string& output, const LateBindVar var)
{
    output.append(var.Name);
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
