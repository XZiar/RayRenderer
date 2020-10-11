#pragma once

#include "XCompRely.h"
#include "common/ContainerEx.hpp"
#include "common/StringPool.hpp"
#include "common/AlignedBuffer.hpp"
#include "3rdParty/half/half.hpp"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xcomp::debug
{


struct NLCLDebugExtension;
struct KernelDebugExtension;


class XCOMPBASAPI ArgsLayout : private common::StringPool<char32_t>
{
public:
    template<typename T>
    struct VecItem
    {
        const T* Ptr;
        uint32_t Count;
        VecItem(common::span<const std::byte> data, common::simd::VecDataInfo info) noexcept :
            Ptr(reinterpret_cast<const T*>(data.data())), Count(info.Dim0) { }
    };
private:
    struct ArgItem
    {
        common::simd::VecDataInfo Info;
        common::StringPiece<char32_t> Name;
        uint16_t Offset;
        uint16_t ArgIdx;
        constexpr ArgItem() noexcept : Info({}), Offset(0), ArgIdx(0) { }
        constexpr ArgItem(common::simd::VecDataInfo info, uint16_t offset, uint16_t idx, common::StringPiece<char32_t> name) noexcept :
            Info(info), Name(name), Offset(offset), ArgIdx(idx) { }
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
                case 16: return func(VecItem<  half>(data, Info));
                case 32: return func(VecItem< float>(data, Info));
                case 64: return func(VecItem<double>(data, Info));
                default: break;
                }
                break;
            case VecDataInfo::DataTypes::Unsigned:
                switch (Info.Bit)
                {
                case  8: return func(VecItem< uint8_t>(data, Info));
                case 16: return func(VecItem<uint16_t>(data, Info));
                case 32: return func(VecItem<uint32_t>(data, Info));
                case 64: return func(VecItem<uint64_t>(data, Info));
                default: break;
                }
                break;
            case VecDataInfo::DataTypes::Signed:
                switch (Info.Bit)
                {
                case  8: return func(VecItem< int8_t>(data, Info));
                case 16: return func(VecItem<int16_t>(data, Info));
                case 32: return func(VecItem<int32_t>(data, Info));
                case 64: return func(VecItem<int64_t>(data, Info));
                default: break;
                }
                break;
            default: break;
            }
            return func(std::nullopt);
        }
    };
    const ArgItem& GetByIndex(const size_t idx) const noexcept
    {
        return Args[idx];
    }
    const ArgItem& GetByLayout(const size_t idx) const noexcept
    {
        return Args[ArgLayout[idx]];
    }
    using ItTypeIndexed  = common::container::IndirectIterator<const ArgsLayout, const ArgItem&, &ArgsLayout::GetByIndex>;
    using ItTypeLayouted = common::container::IndirectIterator<const ArgsLayout, const ArgItem&, &ArgsLayout::GetByLayout>;
    friend ItTypeIndexed;
    friend ItTypeLayouted;
    class Indexed
    {
        friend class ArgsLayout;
        const ArgsLayout* Host;
        constexpr Indexed(const ArgsLayout* host) noexcept : Host(host) { }
    public:
        constexpr ItTypeIndexed begin() const noexcept { return { Host, 0 }; }
        constexpr ItTypeIndexed end()   const noexcept { return { Host, Host->ArgCount }; }
    };
    class Layouted
    {
        friend class ArgsLayout;
        const ArgsLayout* Host;
        constexpr Layouted(const ArgsLayout* host) noexcept : Host(host) { }
    public:
        constexpr ItTypeLayouted begin() const noexcept { return { Host, 0 }; }
        constexpr ItTypeLayouted end()   const noexcept { return { Host, Host->ArgCount }; }
    };
    std::unique_ptr<ArgItem[]> Args;
    std::unique_ptr<uint16_t[]> ArgLayout;
public:
    uint32_t TotalSize;
    uint32_t ArgCount;

    using InputType = std::pair<std::u32string_view, common::simd::VecDataInfo>;

    ArgsLayout(common::span<const InputType> infos, const uint16_t align = 4);
    ArgsLayout(ArgsLayout&& other) = default;
    constexpr Indexed  ByIndex()  const noexcept { return this; }
    constexpr Layouted ByLayout() const noexcept { return this; }
    const ArgItem& operator[](size_t idx) const noexcept { return Args[idx]; }
    std::u32string_view GetName(const ArgItem& item) const noexcept { return GetStringView(item.Name); }
};


struct XCOMPBASAPI MessageBlock
{
    ArgsLayout Layout;
    std::u32string Name;
    std::u32string Formatter;
    uint8_t DebugId;
    template<typename... Args>
    MessageBlock(const uint8_t idx, const std::u32string_view name, const std::u32string_view formatter,
        common::span<const ArgsLayout::InputType> infos) :
        Layout(infos, 4), Name(name), Formatter(formatter), DebugId(idx) {}
    
    common::str::u8string GetString(common::span<const std::byte> data) const;
    template<typename T>
    common::str::u8string GetString(common::span<T> data) const
    {
        return GetString(common::as_bytes(data));
    }
};


struct WorkItemInfo
{
    uint32_t ThreadId;
    uint32_t GlobalId[3];
    uint32_t GroupId[3];
    uint32_t LocalId[3];
};
class InfoProvider
{
protected:
    static constexpr void SetBasicInfo(const uint32_t(&gsize)[3], const uint32_t(&lsize)[3], const uint32_t id, WorkItemInfo& info) noexcept
    {
        info.ThreadId = id;
        const auto gs0 = gsize[0] * gsize[1];
        info.GlobalId[2] = id / gs0;
        info.GlobalId[1] = (id % gs0) / gsize[0];
        info.GlobalId[0] = id % gsize[0];
        info.GroupId[2] = info.GlobalId[2] / lsize[2];
        info.GroupId[1] = info.GlobalId[1] / lsize[1];
        info.GroupId[0] = info.GlobalId[0] / lsize[0];
        info.LocalId[2] = info.GlobalId[2] % lsize[2];
        info.LocalId[1] = info.GlobalId[1] % lsize[1];
        info.LocalId[0] = info.GlobalId[0] % lsize[0];
    }
public:
    virtual ~InfoProvider();

    virtual uint32_t GetInfoBufferSize(const size_t* worksize, const uint32_t dims) const noexcept;
    virtual std::unique_ptr<WorkItemInfo> GetThreadInfo(common::span<const uint32_t> space, const uint32_t tid) const noexcept = 0;
};


class XCOMPBASAPI DebugManager
{
    friend struct NLCLDebugExtension;
    friend struct KernelDebugExtension;
protected:
    std::vector<MessageBlock> Blocks;
    std::unique_ptr<InfoProvider> InfoProv;
    void CheckNewBlock(const std::u32string_view name) const;
    template<typename... Args>
    const MessageBlock& AppendBlock(const std::u32string_view name, const std::u32string_view formatter, Args&&... args)
    {
        CheckNewBlock(name);
        return Blocks.emplace_back(static_cast<uint8_t>(Blocks.size()), name, formatter, std::forward<Args>(args)...);
    }
    std::pair<const MessageBlock*, uint32_t> RetriveMessage(common::span<const uint32_t> data) const;
public:

    common::span<const MessageBlock> GetBlocks() const noexcept { return Blocks; }
    const InfoProvider& GetInfoProvider() const noexcept { return *InfoProv; }

    template<typename F>
    void VisitData(common::span<const std::byte> space, F&& func) const
    {
        common::span<const uint32_t> data(reinterpret_cast<const uint32_t*>(space.data()), space.size() / sizeof(uint32_t));
        while (data.size() > 0)
        {
            const auto [block, tid] = RetriveMessage(data);
            if (block == nullptr) break;
            const auto u32cnt = block->Layout.TotalSize / sizeof(uint32_t);
            func(tid, *InfoProv, *block, data.subspan(1, u32cnt));
            data = data.subspan(1 + u32cnt);
        }
    }
};


class CachedDebugPackage;
class XCOMPBASAPI DebugPackage
{
protected:
    std::shared_ptr<DebugManager> Manager;
    common::AlignedBuffer InfoBuffer;
    common::AlignedBuffer DataBuffer;
public:
    DebugPackage(std::shared_ptr<DebugManager> manager, common::AlignedBuffer&& info, common::AlignedBuffer&& data) noexcept
        : Manager(manager), InfoBuffer(std::move(info)), DataBuffer(std::move(data)) { }
    virtual ~DebugPackage();
    virtual void ReleaseRuntime() {}
    template<typename F>
    void VisitData(F&& func) const
    {
        if (!Manager) return;
        const auto infoData = InfoSpan();
        Manager->VisitData(DataBuffer.AsSpan(),
            [&](const uint32_t tid, const InfoProvider& infoProv, const MessageBlock& block, const auto& dat)
            {
                func(tid, infoProv, block, infoData, dat);
            });
    }
    CachedDebugPackage GetCachedData() const;
    const InfoProvider& InfoMan() const noexcept { return Manager->GetInfoProvider(); }
    common::span<const uint32_t> InfoSpan() const noexcept { return InfoBuffer.AsSpan<uint32_t>(); }
};

class XCOMPBASAPI CachedDebugPackage : protected DebugPackage, private common::StringPool<u8ch_t>
{
    struct MessageItem
    {
        common::StringPiece<u8ch_t> Str;
        uint32_t ThreadId;
        uint32_t DataOffset;
        uint16_t DataLength;
        uint16_t BlockId;
    };
    class XCOMPBASAPI MessageItemWrapper
    {
        friend CachedDebugPackage;
        CachedDebugPackage& Host;
        MessageItem& Item;
        constexpr MessageItemWrapper(CachedDebugPackage* host, MessageItem& item) noexcept : Host(*host), Item(item) { }
    public:
        common::str::u8string_view Str();
        constexpr uint32_t ThreadId() const noexcept { return Item.ThreadId; }
    };
    MessageItemWrapper GetByIndex(const size_t idx) noexcept
    {
        return { this, Items[idx] };
    }
    std::vector<MessageItem> Items;
public:
    CachedDebugPackage(std::shared_ptr<DebugManager> manager, common::AlignedBuffer&& info, common::AlignedBuffer&& data);
    ~CachedDebugPackage() override;

    using ItType = common::container::IndirectIterator<CachedDebugPackage, MessageItemWrapper, &CachedDebugPackage::GetByIndex>;
    friend ItType;
    constexpr ItType begin() noexcept { return { this, 0 }; }
              ItType end()   noexcept { return { this, Items.size() }; }
    size_t Count() const noexcept { return Items.size(); }

};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

