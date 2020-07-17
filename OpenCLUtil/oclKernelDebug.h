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

OCLUAPI std::pair<common::simd::VecDataInfo, bool> ParseVDataType(const std::u32string_view type) noexcept;
OCLUAPI std::u32string_view StringifyVDataType(const common::simd::VecDataInfo vtype) noexcept;


struct NLCLDebugExtension;
struct KernelDebugExtension;
OCLUAPI void SetAllowDebug(const NLCLContext& context) noexcept;


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
    //DebugDataLayout(const DebugDataLayout& other);
    DebugDataLayout(DebugDataLayout&& other) = default;
    constexpr Indexed  ByIndex()  const noexcept { return this; }
    constexpr Layouted ByLayout() const noexcept { return this; }
    const DataBlock& operator[](size_t idx) const noexcept { return Blocks[idx]; }
};


struct oclThreadInfo
{
    uint32_t ThreadId;
    uint32_t GlobalId[3];
    uint32_t GroupId[3];
    uint32_t LocalId[3];
    uint16_t SubgroupId, SubgroupLocalId;
};
class oclDebugInfoMan
{
protected:
    oclThreadInfo BasicInfo(const uint32_t(&gsize)[3], const uint32_t(&lsize)[3], const uint32_t tid) const noexcept
    {
        oclThreadInfo info;
        info.ThreadId = tid;
        const auto gs0 = gsize[0] * gsize[1];
        info.GlobalId[2] = tid / gs0;
        info.GlobalId[1] = (tid % gs0) / gsize[0];
        info.GlobalId[0] = tid % gsize[0];
        info.GroupId[2] = info.GlobalId[2] / lsize[2];
        info.GroupId[1] = info.GlobalId[1] / lsize[1];
        info.GroupId[0] = info.GlobalId[0] / lsize[0];
        info.LocalId[2] = info.GlobalId[2] % lsize[2];
        info.LocalId[1] = info.GlobalId[1] % lsize[1];
        info.LocalId[0] = info.GlobalId[0] % lsize[0];
        return info;
    }
public:
    virtual ~oclDebugInfoMan();

    virtual uint32_t GetInfoBufferSize(const size_t* worksize, const uint32_t dims) const noexcept;
    virtual oclThreadInfo GetThreadInfo(common::span<const uint32_t> space, const uint32_t tid) const noexcept = 0;
};
class NonSubgroupInfoMan : public oclDebugInfoMan
{
    oclThreadInfo GetThreadInfo(common::span<const uint32_t> space, const uint32_t tid) const noexcept override;
};
class SubgroupInfoMan : public oclDebugInfoMan
{
    oclThreadInfo GetThreadInfo(common::span<const uint32_t> space, const uint32_t tid) const noexcept override;
};

struct OCLUAPI oclDebugBlock
{
    DebugDataLayout Layout;
    std::u32string Name;
    std::u32string Formatter;
    uint8_t DebugId;
    template<typename... Args>
    oclDebugBlock(const uint8_t idx, const std::u32string_view name, const std::u32string_view formatter, common::span<const common::simd::VecDataInfo> infos) :
        Layout(infos, 4), Name(name), Formatter(formatter), DebugId(idx) {}
    
    common::str::u8string GetString(common::span<const std::byte> data) const;
    template<typename T>
    common::str::u8string GetString(common::span<T> data) const
    {
        return GetString(common::as_bytes(data));
    }
};


class OCLUAPI oclDebugManager
{
    friend struct NLCLDebugExtension;
    friend struct KernelDebugExtension;
    friend class oclKernel_;
protected:
    std::vector<oclDebugBlock> Blocks;
    std::unique_ptr<oclDebugInfoMan> InfoMan;
    void CheckNewBlock(const std::u32string_view name) const;
    template<typename... Args>
    const oclDebugBlock& AppendBlock(const std::u32string_view name, const std::u32string_view formatter, Args&&... args)
    {
        CheckNewBlock(name);
        return Blocks.emplace_back(static_cast<uint8_t>(Blocks.size()), name, formatter, std::forward<Args>(args)...);
    }
    std::pair<const oclDebugBlock*, uint32_t> RetriveMessage(common::span<const uint32_t> data) const;
public:

    common::span<const oclDebugBlock> GetBlocks() const noexcept { return Blocks; }
    const oclDebugInfoMan& GetInfoMan() const noexcept { return *InfoMan; }

    template<typename F>
    void VisitData(common::span<const std::byte> space, F&& func) const
    {
        common::span<const uint32_t> data(reinterpret_cast<const uint32_t*>(space.data()), space.size() / sizeof(uint32_t));
        while (data.size() > 0)
        {
            const auto [block, tid] = RetriveMessage(data);
            if (block == nullptr) break;
            const auto u32cnt = block->Layout.TotalSize / sizeof(uint32_t);
            func(tid, *InfoMan, *block, data.subspan(1, u32cnt));
            data = data.subspan(1 + u32cnt);
        }
    }
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

