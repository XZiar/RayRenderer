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
using FuncArgRaw = std::variant<LateBindVar, std::u32string_view, uint64_t, int64_t, double, bool>;

struct MetaFunc
{
    std::u32string_view Name;
    std::vector<FuncArgRaw> Args;
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