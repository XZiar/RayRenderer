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
    size_t TrunkSize = 0;
public:
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
struct EmbedStatement;

using FuncArgRaw = std::variant<std::unique_ptr<FuncCall>, std::unique_ptr<EmbedStatement>, 
    LateBindVar, std::u32string_view, uint64_t, int64_t, double, bool>;

struct FuncCall
{
    std::u32string_view Name;
    std::vector<FuncArgRaw> Args;
};
struct EmbedStatement
{
    FuncArgRaw LeftOprend, RightOprend;
    EmbedOps Operator;
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