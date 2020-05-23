#pragma once

#include "oclRely.h"
#include "common/SIMD.hpp"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{

class oclDebugBlock
{
private:
    struct DataBlock
    {
        common::simd::VecDataInfo Info;
        uint16_t Offset;
        constexpr DataBlock() noexcept : Info({}), Offset(0) { }
        constexpr DataBlock(common::simd::VecDataInfo info, uint16_t offset) noexcept :
            Info(info), Offset(offset) { }
    };
    std::unique_ptr<DataBlock[]> Blocks;
    std::unique_ptr<uint16_t[]> ArgIdxs;
public:
    std::u32string Formatter;
    uint32_t TotalSize;
    uint32_t ArgCount;

    oclDebugBlock(common::span<const common::simd::VecDataInfo> infos, const std::u32string_view formatter, const uint16_t align = 4);
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

