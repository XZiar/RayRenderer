#pragma once

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include <vector>
#include <variant>
#include <string>
#include <string_view>
#include <cstdio>


namespace xziar::nailang
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
    constexpr bool operator==(const LateBindVar& other) const noexcept
    {
        return Name == other.Name;
    }
    constexpr bool operator==(const std::u32string_view other) const noexcept
    {
        return Name == other;
    }
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


struct RawArg
{
    enum class Type : uint32_t { Empty = 0, Func = 1, Unary = 2, Binary = 3, Var = 4, Str = 5, Uint = 6, Int = 7, FP = 8, Bool = 9 };
    using Variant = std::variant<const FuncCall*, const UnaryStatement*, const BinaryStatement*, LateBindVar, std::u32string_view, uint64_t, int64_t, double, bool>;
    uint64_t Data1;
    uint32_t Data2;
    Type TypeData;

    constexpr RawArg() noexcept : Data1(0), Data2(0), TypeData(Type::Empty) {}
    RawArg(const FuncCall* ptr) noexcept : 
        Data1(reinterpret_cast<uint64_t>(ptr)), Data2(1), TypeData(Type::Func) {}
    RawArg(const UnaryStatement* ptr) noexcept :
        Data1(reinterpret_cast<uint64_t>(ptr)), Data2(2), TypeData(Type::Unary) {}
    RawArg(const BinaryStatement* ptr) noexcept :
        Data1(reinterpret_cast<uint64_t>(ptr)), Data2(3), TypeData(Type::Binary) {}
    RawArg(const LateBindVar var) noexcept :
        Data1(reinterpret_cast<uint64_t>(var.Name.data())), Data2(static_cast<uint32_t>(var.Name.size())), TypeData(Type::Var) 
    {
        Expects(var.Name.size() <= UINT32_MAX);
    }
    RawArg(const std::u32string_view str) noexcept :
        Data1(reinterpret_cast<uint64_t>(str.data())), Data2(static_cast<uint32_t>(str.size())), TypeData(Type::Str)
    {
        Expects(str.size() <= UINT32_MAX);
    }
    RawArg(const uint64_t num) noexcept :
        Data1(num), Data2(6), TypeData(Type::Uint) {}
    RawArg(const int64_t num) noexcept :
        Data1(static_cast<uint64_t>(num)), Data2(7), TypeData(Type::Int) {}
    RawArg(const double num) noexcept :
        Data1(*reinterpret_cast<const uint64_t*>(&num)), Data2(8), TypeData(Type::FP) {}
    RawArg(const bool boolean) noexcept :
        Data1(boolean ? 1 : 0), Data2(9), TypeData(Type::Bool) {}

    template<Type T>
    constexpr auto GetVar() const
    {
        Expects(TypeData == T);
        if constexpr (T == Type::Func)
            return reinterpret_cast<const FuncCall*>(Data1);
        else if constexpr (T == Type::Unary)
            return reinterpret_cast<const UnaryStatement*>(Data1);
        else if constexpr (T == Type::Binary)
            return reinterpret_cast<const BinaryStatement*>(Data1);
        else if constexpr (T == Type::Var)
            return LateBindVar{ {reinterpret_cast<const char32_t*>(Data1), Data2} };
        else if constexpr (T == Type::Str)
            return std::u32string_view{ reinterpret_cast<const char32_t*>(Data1), Data2 };
        else if constexpr (T == Type::Uint)
            return Data1;
        else if constexpr (T == Type::Int)
            return static_cast<int64_t>(Data1);
        else if constexpr (T == Type::FP)
            return *reinterpret_cast<const double*>(&Data1);
        else if constexpr (T == Type::Bool)
            return Data1 == 1;
        else
            static_assert(!common::AlwaysTrue<decltype(T)>, "");
    }

    forceinline Variant GetVar() const
    {
        switch (TypeData)
        {
        case Type::Func:    return GetVar<Type::Func>();
        case Type::Unary:   return GetVar<Type::Unary>();
        case Type::Binary:  return GetVar<Type::Binary>();
        case Type::Var:     return GetVar<Type::Var>();
        case Type::Str:     return GetVar<Type::Str>();
        case Type::Uint:    return GetVar<Type::Uint>();
        case Type::Int:     return GetVar<Type::Int>();
        case Type::FP:      return GetVar<Type::FP>();
        case Type::Bool:    return GetVar<Type::Bool>();
        default:            Expects(false); return {};
        }
    }
    template<typename Visitor>
    forceinline auto Visit(Visitor&& visitor) const
    {
        switch (TypeData)
        {
        case Type::Func:    return visitor(GetVar<Type::Func>());
        case Type::Unary:   return visitor(GetVar<Type::Unary>());
        case Type::Binary:  return visitor(GetVar<Type::Binary>());
        case Type::Var:     return visitor(GetVar<Type::Var>());
        case Type::Str:     return visitor(GetVar<Type::Str>());
        case Type::Uint:    return visitor(GetVar<Type::Uint>());
        case Type::Int:     return visitor(GetVar<Type::Int>());
        case Type::FP:      return visitor(GetVar<Type::FP>());
        case Type::Bool:    return visitor(GetVar<Type::Bool>());
        default:            Expects(false); return visitor(Data1);
        }
    }
};


struct FuncCall
{
    std::u32string_view Name;
    common::span<RawArg> Args;
};
struct UnaryStatement
{
    RawArg Oprend;
    EmbedOps Operator;
    UnaryStatement(const EmbedOps op, const RawArg oprend) noexcept :
        Oprend(oprend), Operator(op) { }
};
struct BinaryStatement
{
    RawArg LeftOprend, RightOprend;
    EmbedOps Operator;
    BinaryStatement(const EmbedOps op, const RawArg left, const RawArg right) noexcept :
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
    RawArg Statement;
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