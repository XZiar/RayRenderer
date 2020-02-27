#pragma once

#include "common/CommonRely.hpp"
#include <vector>
#include <variant>
#include <string>
#include <string_view>


namespace xziar::sectorlang
{

class MemoryPool
{
private:
    std::vector<std::tuple<std::byte*, size_t, size_t>> Trunks;
    size_t TrunkSize;
public:
    MemoryPool(const size_t trunkSize = 2 * 1024 * 1024) : TrunkSize(trunkSize) { }
    common::span<std::byte> Alloc(const size_t size);
    template<typename T>
    forceinline common::span<std::byte> Alloc()
    {
        return Alloc(sizeof(T));
    }
    template<typename T, typename... Args>
    forceinline T* Create(Args&&... args)
    {
        const auto space = Alloc<T>();
        new (space.data()) T(std::forward<Args>(args)...);
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

using FuncArgRaw = std::variant<std::unique_ptr<FuncCall>, std::unique_ptr<UnaryStatement>, std::unique_ptr<BinaryStatement>,
    LateBindVar, std::u32string_view, uint64_t, int64_t, double, bool>;

struct FuncCall
{
    std::u32string_view Name;
    std::vector<FuncArgRaw> Args;
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