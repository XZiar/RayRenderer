#pragma once

#include "oclRely.h"
#include "common/SIMD.hpp"
#include "3rdParty/half/half.hpp"

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
    private:
        template<typename T>
        constexpr common::span<const T> VisitT(common::span<const std::byte> data) const noexcept
        {
            return common::span<const T>(reinterpret_cast<const T*>(data.data()), Info.Dim0);
        }
    public:
        struct 
        common::simd::VecDataInfo Info;
        uint16_t Offset;
        uint16_t ArgIdx;
        constexpr DataBlock() noexcept : Info({}), Offset(0), ArgIdx(0) { }
        constexpr DataBlock(common::simd::VecDataInfo info, uint16_t offset, uint16_t idx) noexcept :
            Info(info), Offset(offset), ArgIdx(idx) { }
        template<typename F>
        auto VisitData(common::span<const std::byte> space, F&& func) const
        {
            using common::simd::VecDataInfo;
            using half_float::half;
            const auto data = space.subspan(Offset, Info.Bit * Info.Dim0 / 8);
            switch (Info.Type)
            {
            case VecDataInfo::DataTypes::Float:
                switch (Info.Bit)
                {
                case 16: return func(VisitT<  half>(data));
                case 32: return func(VisitT< float>(data));
                case 64: return func(VisitT<double>(data));
                default: break;
                }
                break;
            case VecDataInfo::DataTypes::Unsigned:
                switch (Info.Bit)
                {
                case  8: return func(VisitT< uint8_t>(data));
                case 16: return func(VisitT<uint16_t>(data));
                case 32: return func(VisitT<uint32_t>(data));
                case 64: return func(VisitT<uint64_t>(data));
                default: break;
                }
                break;
            case VecDataInfo::DataTypes::Signed:
                switch (Info.Bit)
                {
                case  8: return func(VisitT< int8_t>(data));
                case 16: return func(VisitT<int16_t>(data));
                case 32: return func(VisitT<int32_t>(data));
                case 64: return func(VisitT<int64_t>(data));
                default: break;
                }
                break;
            default: break;
            }
            return func(std::nullopt);
        }
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
    uint32_t TotalSize;
    uint32_t ArgCount;

    DebugDataLayout(common::span<const common::simd::VecDataInfo> infos, const uint16_t align = 4);
    DebugDataLayout(const DebugDataLayout& other);
    constexpr Indexed  ByIndex()  const noexcept { return this; }
    constexpr Layouted ByLayout() const noexcept { return this; }
    const DataBlock& operator[](size_t idx) const noexcept { return Blocks[idx]; }
};

struct oclDebugBlock
{
    DebugDataLayout Layout;
    std::u32string Formatter;
    uint8_t DebugId;
    template<typename... Args>
    oclDebugBlock(const uint8_t idx, const std::u32string_view formatter, Args&&... args) : 
        Layout(std::forward<Args>(args)...), Formatter(formatter), DebugId(idx) {}
    
    common::str::u8string GetString(common::span<const std::byte> data) const;
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

