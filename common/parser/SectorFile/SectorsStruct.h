#pragma once

#include "common/CommonRely.hpp"
#include <vector>
#include <variant>
#include <string>
#include <string_view>


namespace xziar::sectorlang
{

struct LateBindVar
{
    std::u32string_view Name;
};

enum class EmbedOps : uint8_t { Equal = 0, NotEqual, Less, LessEqual, Greater, GreaterEqual, And, Or, Not, Add, Sub, Mul, Div, Rem };

using BasicFuncArgRaw = std::variant<LateBindVar, std::u32string_view, uint64_t, int64_t, double, bool>;

struct EmbedStatement
{
    BasicFuncArgRaw LeftOprend, RightOprend;
    EmbedOps Operator;
};

using ComplexFuncArgRaw = std::variant<LateBindVar, EmbedStatement, std::u32string_view, uint64_t, int64_t, double, bool>;

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