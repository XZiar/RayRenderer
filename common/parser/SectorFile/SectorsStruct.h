#pragma once

#include "common/CommonRely.hpp"
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
    template<typename T, typename RealT = std::remove_const_t<T>>
    forceinline common::span<RealT> CreateArray(common::span<T> data)
    {
        if (data.size() == 0) return {};
        const auto space = Alloc(sizeof(T) * data.size(), alignof(T));
        for (size_t i = 0; i < static_cast<size_t>(data.size()); ++i)
            new (space.data() + sizeof(T) * i) T(data[i]);
        return common::span<RealT>(reinterpret_cast<RealT*>(space.data()), data.size());
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
    UnaryStatement(EmbedOps op, FuncArgRaw&& oprend) noexcept : 
        Oprend(std::move(oprend)), Operator(std::move(op)) { }
};
struct BinaryStatement
{
    FuncArgRaw LeftOprend, RightOprend;
    EmbedOps Operator;
    BinaryStatement(EmbedOps op, FuncArgRaw&& left, FuncArgRaw&& right) noexcept :
        LeftOprend(std::move(left)), RightOprend(std::move(right)), Operator(std::move(op)) { }
};

using BasicFuncArgRaw = std::variant<LateBindVar, std::u32string_view, uint64_t, int64_t, double, bool>;


struct MetaFunc
{
    std::u32string_view Name;
    std::vector<BasicFuncArgRaw> Args;
};

struct SectorRaw
{
    std::u32string_view Type;
    std::u32string_view Name;
    std::u32string_view Content;
    std::vector<MetaFunc> MetaFunctions;
};

struct Block
{
    std::u32string Type;
    std::u32string Name;
    std::u32string_view Content;

    Block(std::u32string_view type, std::u32string_view name, std::u32string_view content)
        : Type(type), Name(name), Content(content) { }
};



}