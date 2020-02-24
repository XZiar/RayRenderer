#pragma once

#include "common/CommonRely.hpp"
#include <vector>


namespace xziar::sectorlang
{

struct SectorRaw
{
    std::u32string Type;
    std::u32string Name;
    std::u32string_view Content;

    SectorRaw(std::u32string_view type, std::u32string_view name, std::u32string_view content)
        : Type(type), Name(name), Content(content) { }
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