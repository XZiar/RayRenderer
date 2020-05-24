#pragma once

#include "oclRely.h"
#include "common/SIMD.hpp"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{

std::pair<common::simd::VecDataInfo, bool> ParseVDataType(const std::u32string_view type) noexcept;
std::u32string_view StringifyVDataType(const common::simd::VecDataInfo vtype) noexcept;

class DebugDataLayout
{
private:
    struct DataBlock
    {
        common::simd::VecDataInfo Info;
        uint16_t Offset;
        uint16_t ArgIdx;
        constexpr DataBlock() noexcept : Info({}), Offset(0), ArgIdx(0) { }
        constexpr DataBlock(common::simd::VecDataInfo info, uint16_t offset, uint16_t idx) noexcept :
            Info(info), Offset(offset), ArgIdx(idx) { }
    };
    const DataBlock& GetByIndex(const size_t idx) const noexcept
    {
        return Blocks[idx];
    }
    const DataBlock& GetByLayout(const size_t idx) const noexcept
    {
        return Blocks[ArgLayout[idx]];
    }
    using ItTypeIndexed  = common::container::IndirectIterator<DebugDataLayout, const DataBlock&, &DebugDataLayout::GetByIndex>;
    using ItTypeLayouted = common::container::IndirectIterator<DebugDataLayout, const DataBlock&, &DebugDataLayout::GetByLayout>;
    friend ItTypeIndexed;
    friend ItTypeLayouted;
    class Indexed
    {
        friend class DebugDataLayout;
        const DebugDataLayout* Host;
        constexpr Indexed(const DebugDataLayout* host) noexcept : Host(host) { }
    public:
        constexpr ItTypeIndexed begin() const noexcept { return { Host, 0 }; }
        constexpr ItTypeIndexed end()   const noexcept { return { Host, Host->ArgCount }; }
    };
    class Layouted
    {
        friend class DebugDataLayout;
        const DebugDataLayout* Host;
        constexpr Layouted(const DebugDataLayout* host) noexcept : Host(host) { }
    public:
        constexpr ItTypeLayouted begin() const noexcept { return { Host, 0 }; }
        constexpr ItTypeLayouted end()   const noexcept { return { Host, Host->ArgCount }; }
    };
    std::unique_ptr<DataBlock[]> Blocks;
    std::unique_ptr<uint16_t[]> ArgLayout;
public:
    std::u32string Formatter;
    uint32_t TotalSize;
    uint32_t ArgCount;

    DebugDataLayout(common::span<const common::simd::VecDataInfo> infos, const std::u32string_view formatter, const uint16_t align = 4);
    constexpr Indexed  ByIndex()  const noexcept { return this; }
    constexpr Layouted ByLayout() const noexcept { return this; }
};

struct oclDebugBlock
{
    DebugDataLayout Layout;
    uint8_t DebugId;
    template<typename... Args>
    oclDebugBlock(const uint8_t idx, Args&&... args) : Layout(std::forward<Args>(args)...), DebugId(idx) {}
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

