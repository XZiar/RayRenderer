#pragma once

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include <vector>
#include <variant>
#include <string>
#include <string_view>
#include <cstdio>


namespace xziar::sectorlang
{

class MemoryPool : public common::NonCopyable
{
public:
    std::vector<std::tuple<std::byte*, size_t, size_t>> Trunks;
    size_t TrunkSize;
public:
    MemoryPool(const size_t trunkSize = 2 * 1024 * 1024) : TrunkSize(trunkSize) { }
    MemoryPool(MemoryPool&& other) noexcept = default;
    ~MemoryPool();

    common::span<std::byte> Alloc(const size_t size, const size_t align = 64);
    std::pair<size_t, size_t> Usage() const noexcept;

    template<typename T>
    forceinline common::span<std::byte> Alloc()
    {
        return Alloc(sizeof(T), alignof(T));
    }

    template<typename T, typename... Args>
    forceinline T* Create(Args&&... args)
    {
        const auto space = Alloc<T>();
        new (space.data()) T(std::forward<Args>(args)...);
        return reinterpret_cast<T*>(space.data());
    }
    template<typename C>
    forceinline auto CreateArray(C&& container)
    {
        const auto data = common::to_span(container);
        using T = std::remove_const_t<typename decltype(data)::element_type>;

        if (data.size() == 0) return common::span<T>{};
        const auto space = Alloc(sizeof(T) * data.size(), alignof(T));
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            memcpy_s(space.data(), space.size(), data.data(), sizeof(T) * data.size());
        }
        else
        {
            for (size_t i = 0; i < static_cast<size_t>(data.size()); ++i)
                new (space.data() + sizeof(T) * i) T(data[i]);
        }
        return common::span<T>(reinterpret_cast<T*>(space.data()), data.size());
    }
};


struct LateBindVar
{
    std::u32string_view Name;
};

enum class EmbedOps : uint8_t { Equal = 0, NotEqual, Less, LessEqual, Greater, GreaterEqual, And, Or, Not, Add, Sub, Mul, Div, Rem };
struct EmbedOpHelper
{
    static constexpr bool IsUnaryOp(EmbedOps op) noexcept
    {
        return op == EmbedOps::Not;
    }
};


struct FuncCall;
struct UnaryStatement;
struct BinaryStatement;

using FuncArgRaw = std::variant<FuncCall*, UnaryStatement*, BinaryStatement*,
    LateBindVar, std::u32string_view, uint64_t, int64_t, double, bool>;

struct FuncCall
{
    std::u32string_view Name;
    common::span<FuncArgRaw> Args;
};
struct UnaryStatement
{
    FuncArgRaw Oprend;
    EmbedOps Operator;
    UnaryStatement(const EmbedOps op, const FuncArgRaw oprend) noexcept :
        Oprend(oprend), Operator(op) { }
};
struct BinaryStatement
{
    FuncArgRaw LeftOprend, RightOprend;
    EmbedOps Operator;
    BinaryStatement(const EmbedOps op, const FuncArgRaw left, const FuncArgRaw right) noexcept :
        LeftOprend(left), RightOprend(right), Operator(op) { }
};


template<typename T>
struct WithMeta : public T
{
    common::span<const FuncCall> MetaFunctions;
};

struct RawBlock
{
    std::u32string_view Type;
    std::u32string_view Name;
    std::u32string_view Source;
    std::u16string FileName;
    std::pair<size_t, size_t> Position;
};


struct Assignment
{
    LateBindVar Variable;
    FuncArgRaw Statement;
};

using AssignmentWithMeta = WithMeta<Assignment>;
using FuncCallWithMeta   = WithMeta<FuncCall>;
using RawBlockWithMeta   = WithMeta<RawBlock>;

struct BlockContent
{
    enum class Type : uint8_t { Assignment = 0, FuncCall = 1, RawBlock = 2 };
    uintptr_t Pointer;

    template<typename T>
    forceinline const T* Get() const noexcept
    {
        return reinterpret_cast<const T*>(Pointer & ~(uintptr_t)(0x7));
    }

    constexpr Type GetType() const
    {
        const auto type = static_cast<Type>(Pointer & 0x7);
        switch (type)
        {
        case Type::Assignment:
        case Type::FuncCall:
        case Type::RawBlock:
            return type;
        default:
            Expects(false);
            return type;
        }
    }
    std::variant<const AssignmentWithMeta*, const FuncCallWithMeta*, const RawBlockWithMeta*>
        GetStatement() const
    {
        switch (GetType())
        {
        case Type::Assignment:  return Get<AssignmentWithMeta>();
        case Type::FuncCall:    return Get<  FuncCallWithMeta>();
        case Type::RawBlock:    return Get<  RawBlockWithMeta>();
        default:                Expects(false); return {};
        }
    }
    template<typename Visitor> 
    auto Visit(Visitor&& visitor) const
    {
        switch (GetType())
        {
        case Type::Assignment:  return visitor(Get<AssignmentWithMeta>());
        case Type::FuncCall:    return visitor(Get<  FuncCallWithMeta>());
        case Type::RawBlock:    return visitor(Get<  RawBlockWithMeta>());
        default:Expects(false); return visitor(static_cast<const AssignmentWithMeta*>(nullptr));
        }
    }
    common::span<const FuncCall> GetMetaFunctions() const
    {
        switch (GetType())
        {
        case Type::Assignment:  return Get<AssignmentWithMeta>()->MetaFunctions;
        case Type::FuncCall:    return Get<  FuncCallWithMeta>()->MetaFunctions;
        case Type::RawBlock:    return Get<  RawBlockWithMeta>()->MetaFunctions;
        default:                Expects(false); return {};
        }
    }
    static BlockContent Generate(const AssignmentWithMeta* ptr)
    {
        const auto pointer = reinterpret_cast<uintptr_t>(ptr);
        Expects(pointer % 4 == 0); // should be at least 4 bytes aligned
        return BlockContent{ pointer | common::enum_cast(Type::Assignment) };
    }
    static BlockContent Generate(const FuncCallWithMeta* ptr)
    {
        const auto pointer = reinterpret_cast<uintptr_t>(ptr);
        Expects(pointer % 4 == 0); // should be at least 4 bytes aligned
        return BlockContent{ pointer | common::enum_cast(Type::FuncCall) };
    }
    static BlockContent Generate(const RawBlockWithMeta* ptr)
    {
        const auto pointer = reinterpret_cast<uintptr_t>(ptr);
        Expects(pointer % 4 == 0); // should be at least 4 bytes aligned
        return BlockContent{ pointer | common::enum_cast(Type::RawBlock) };
    }
};
struct Block : RawBlock
{
    common::span<BlockContent> Content;
};


}