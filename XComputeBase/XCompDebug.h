#pragma once

#include "XCompRely.h"
#include "common/ContainerEx.hpp"
#include "common/StringPool.hpp"
#include "common/AlignedBuffer.hpp"
#include "common/Stream.hpp"
#include "3rdParty/half/half.hpp"
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <array>


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xcomp::debug
{
struct XCNLDebugExt;

using NamedVecPair = std::pair<std::u32string_view, common::simd::VecDataInfo>;


class XCOMPBASAPI ArgsLayout
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
    using ItTypeIndexed = common::container::IndirectIterator<const ArgsLayout, const ArgItem&, &ArgsLayout::GetByIndex>;
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
    common::StringPool<char32_t> ArgNames;
    std::unique_ptr<ArgItem[]> Args;
    std::unique_ptr<uint16_t[]> ArgLayout;
public:
    uint32_t TotalSize;
    uint32_t ArgCount;

    ArgsLayout(common::span<const NamedVecPair> infos, const uint16_t align = 4);
    ArgsLayout(ArgsLayout&& other) = default;
    constexpr Indexed  ByIndex()  const noexcept { return this; }
    constexpr Layouted ByLayout() const noexcept { return this; }
    const ArgItem& operator[](size_t idx) const noexcept { return Args[idx]; }
    std::u32string_view GetName(const ArgItem& item) const noexcept { return ArgNames.GetStringView(item.Name); }
};


struct XCOMPBASAPI MessageBlock
{
    ArgsLayout Layout;
    std::u32string Name;
    std::u32string Formatter;
    uint8_t DebugId;
    template<typename... Args>
    MessageBlock(const uint8_t idx, const std::u32string_view name, const std::u32string_view formatter,
        common::span<const NamedVecPair> infos) :
        Layout(infos, 4), Name(name), Formatter(formatter), DebugId(idx) {}

    common::str::u8string GetString(common::span<const std::byte> data) const;
};


struct WorkItemInfo
{
    struct InfoField
    {
        std::string_view Name;
        std::ptrdiff_t Offset;
        common::simd::VecDataInfo VecType;
    };
    uint32_t ThreadId;
    uint32_t GlobalId[3];
    uint32_t GroupId[3];
    uint32_t LocalId[3];
};

struct WGInfoHelper
{
    template<typename T, typename U>
    static std::ptrdiff_t Distance() noexcept
    {
        static_assert(std::is_base_of_v<T, U>);
        U tmp;
        const auto ptrU = reinterpret_cast<const std::byte*>(&tmp);
        const T& obj = static_cast<const T&>(tmp);
        const auto ptrT = reinterpret_cast<const std::byte*>(&obj);
        return ptrU - ptrT;
    }
    template<typename T>
    static constexpr common::simd::VecDataInfo GenerateVType() noexcept
    {
        using common::simd::VecDataInfo;
        using half_float::half;
#define CaseType(t, type, bit) if constexpr (std::is_same_v<T, t>) return { VecDataInfo::DataTypes::type, bit, 0, 0 }
             CaseType(uint8_t,  Unsigned, 8);
        else CaseType(uint16_t, Unsigned, 16);
        else CaseType(uint32_t, Unsigned, 32);
        else CaseType(uint64_t, Unsigned, 64);
        else CaseType( int8_t,    Signed, 8);
        else CaseType( int16_t,   Signed, 16);
        else CaseType( int32_t,   Signed, 32);
        else CaseType( int64_t,   Signed, 64);
        else CaseType(half,        Float, 16);
        else CaseType(float,       Float, 32);
        else CaseType(double,      Float, 64);
        else return { VecDataInfo::DataTypes::Float, 0, 0, 0 };
#undef CaseType
    }
    template<typename T, typename U, size_t N>
    static WorkItemInfo::InfoField GenerateField(const T* obj, std::string_view name, U(T::* field)[N]) noexcept
    {
        static_assert(N >= 1 && N <= 4, "Length of a array cannot be larger than 4");
        constexpr auto vtype = GenerateVType<common::remove_cvref_t<U>>();
        static_assert(vtype.Bit > 0, "Unsupportted datatype");
        auto type = vtype; type.Dim0 = N;
        const auto offset = reinterpret_cast<const std::byte*>(&(obj->*field)) - reinterpret_cast<const std::byte*>(obj);
        Ensures(offset >= 0);
        return { name, offset, type };
    }
    template<typename T, typename U, size_t N>
    static WorkItemInfo::InfoField GenerateField(const T* obj, std::string_view name, std::array<U, N> T::* field) noexcept
    {
        static_assert(N >= 1 && N <= 4, "Length of a array cannot be larger than 4");
        constexpr auto vtype = GenerateVType<common::remove_cvref_t<U>>();
        static_assert(vtype.Bit > 0, "Unsupportted datatype");
        auto type = vtype; type.Dim0 = N;
        const auto offset = reinterpret_cast<const std::byte*>(&(obj->*field)) - reinterpret_cast<const std::byte*>(obj);
        Ensures(offset >= 0);
        return { name, offset, type };
    }
    template<typename T, typename U>
    static WorkItemInfo::InfoField GenerateField(const T* obj, std::string_view name, U T::* field) noexcept
    {
        constexpr auto vtype = GenerateVType<common::remove_cvref_t<U>>();
        static_assert(vtype.Bit > 0, "Unsupportted datatype");
        auto type = vtype; type.Dim0 = 1;
        const auto offset = reinterpret_cast<const std::byte*>(&(obj->*field)) - reinterpret_cast<const std::byte*>(obj);
        Ensures(offset >= 0);
        return { name, offset, type };
    }
    template<typename T, typename U, typename... Args>
    static void GenerateField(std::vector<WorkItemInfo::InfoField>& output, const T* obj, std::string_view name, U field, Args... args) noexcept
    {
        output.push_back(GenerateField(obj, name, field));
        if constexpr (sizeof...(Args) > 0)
            GenerateField(output, obj, args...);
    }
    template<typename T, typename... Args>
    static std::vector<WorkItemInfo::InfoField> GenerateFields(Args... args) noexcept
    {
        static_assert(sizeof...(Args) % 2 == 0, "need to pass a name and member pointer for each field");
        std::vector<WorkItemInfo::InfoField> fields;
        T tmp;
        GenerateField(fields, &tmp, args...);
        return fields;
    }
    template<typename T>
    static common::span<const WorkItemInfo::InfoField> Fields() noexcept;
};


#define XCOMP_WGINFO_FIELD(r, type, i, field) BOOST_PP_COMMA_IF(i) STRINGIZE(field), &type::field
#define XCOMP_WGINFO_REG(type, ...) \
    ::xcomp::debug::WGInfoHelper::GenerateFields<type>(BOOST_PP_SEQ_FOR_EACH_I(XCOMP_WGINFO_FIELD, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)))
#define XCOMP_WGINFO_REG_INHERIT(type, parent, ...) []()        \
    {                                                           \
        using namespace ::xcomp::debug;                         \
        static_assert(std::is_base_of_v<WorkItemInfo, type>);   \
        Expects((WGInfoHelper::Distance<parent, type>() == 0)); \
        auto tmp = XCOMP_WGINFO_REG(type, __VA_ARGS__);         \
        auto prt = WGInfoHelper::Fields<parent>();              \
        tmp.insert(tmp.begin(), prt.begin(), prt.end());        \
        return tmp;                                             \
    }()


class InfoProvider;
class XCOMPBASAPI InfoPack
{
    std::vector<uint32_t> IndexData;
    virtual const WorkItemInfo& GetInfo(const uint32_t idx) = 0;
    virtual uint32_t Generate(common::span<const uint32_t> space, const uint32_t tid) = 0;
protected:
    const InfoProvider& Provider;
    InfoPack(const InfoProvider& provider, const uint32_t count);
public:
    virtual ~InfoPack();
    const WorkItemInfo& GetInfo(common::span<const uint32_t> space, const uint32_t tid);
};

template<typename T>
class InfoPackT final : public InfoPack
{
    friend InfoProvider;
    static_assert(std::is_base_of_v<WorkItemInfo, T>, "T should be derived from WorkItemInfo");
    std::vector<T> RealData;
    const WorkItemInfo& GetInfo(const uint32_t idx) override
    {
        return RealData[idx];
    }
    uint32_t Generate(common::span<const uint32_t> space, const uint32_t tid) override;
public:
    InfoPackT(const InfoProvider& provider, const uint32_t count) : InfoPack(provider, count) {}
    ~InfoPackT() override {}
};

class CachedDebugPackage;
class XCOMPBASAPI InfoProvider
{
    template<typename> friend class InfoPackT;
    friend CachedDebugPackage;
public:
    struct ExecuteInfo
    {
        uint32_t GlobalSize[3];
        uint32_t MinTid, ThreadCount;
    };
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
    static ExecuteInfo GetBasicExeInfo(common::span<const uint32_t> space) noexcept;
    virtual void GetThreadInfo(WorkItemInfo& dst, common::span<const uint32_t> space, const uint32_t tid) const noexcept = 0;
    virtual std::unique_ptr<InfoPack> GetInfoPack(common::span<const uint32_t> space) const = 0;
public:
    virtual ~InfoProvider();

    virtual uint32_t GetInfoBufferSize(const size_t* worksize, const uint32_t dims) const noexcept;
    virtual ExecuteInfo GetExecuteInfo(common::span<const uint32_t> space) const noexcept = 0;
    virtual std::unique_ptr<WorkItemInfo> GetThreadInfo(common::span<const uint32_t> space, const uint32_t tid) const noexcept = 0;
    virtual common::span<const WorkItemInfo::InfoField> QueryFields() const noexcept = 0;
    virtual common::span<const std::byte> GetFullInfoSpace(const WorkItemInfo& info) const noexcept = 0;
};

template<typename T>
inline uint32_t InfoPackT<T>::Generate(common::span<const uint32_t> space, const uint32_t tid)
{
    RealData.push_back({});
    Provider.GetThreadInfo(RealData.back(), space, tid);
    return gsl::narrow_cast<uint32_t>(RealData.size() - 1);
}

template<typename T>
class InfoProviderT : public InfoProvider
{
    static_assert(std::is_base_of_v<WorkItemInfo, T>);
    common::span<const WorkItemInfo::InfoField> QueryFields() const noexcept override
    {
        return xcomp::debug::WGInfoHelper::Fields<T>();
    }
    common::span<const std::byte> GetFullInfoSpace(const WorkItemInfo& info) const noexcept final
    {
        const auto& obj = static_cast<const T&>(info);
        return common::as_bytes(common::span<const T>(&obj, 1));
    }
};


class XCOMPBASAPI DebugManager : public common::NonCopyable
{
    friend XCNLDebugExt;
protected:
    std::vector<MessageBlock> Blocks;
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

    template<typename F>
    void VisitData(common::span<const std::byte> space, F&& func) const
    {
        common::span<const uint32_t> data(reinterpret_cast<const uint32_t*>(space.data()), space.size() / sizeof(uint32_t));
        while (data.size() > 0)
        {
            const auto [block, tid] = RetriveMessage(data);
            if (block == nullptr) break;
            const auto u32cnt = block->Layout.TotalSize / sizeof(uint32_t);
            func(tid, *block, data.subspan(1, u32cnt));
            data = data.subspan(1 + u32cnt);
        }
    }
};


class CachedDebugPackage;
class XCOMPBASAPI DebugPackage
{
protected:
    std::shared_ptr<DebugManager> Manager;
    std::shared_ptr<InfoProvider> InfoProv;
    common::AlignedBuffer InfoBuffer;
    common::AlignedBuffer DataBuffer;
public:
    DebugPackage(std::shared_ptr<DebugManager> manager, std::shared_ptr<InfoProvider> infoProv,
        common::AlignedBuffer&& info, common::AlignedBuffer&& data) noexcept;
    DebugPackage(const DebugPackage& package) noexcept;
    DebugPackage(DebugPackage&& package) noexcept;
    virtual ~DebugPackage();
    virtual void ReleaseRuntime() {}
    template<typename F>
    forceinline void VisitData(F&& func) const
    {
        if (!Manager) return;
        const auto infoData = InfoSpan();
        const auto& infoProv = InfoMan();
        Manager->VisitData(DataBuffer.AsSpan(),
            [&](const uint32_t tid, const MessageBlock& block, const auto& dat)
            {
                func(tid, infoProv, block, infoData, dat);
            });
    }
    CachedDebugPackage GetCachedData() const;
    const DebugManager& DebugMan() const noexcept { return *Manager; }
    const InfoProvider& InfoMan() const noexcept { return *InfoProv; }
    common::span<const uint32_t> InfoSpan() const noexcept { return InfoBuffer.AsSpan<uint32_t>(); }
};

class XCOMPBASAPI CachedDebugPackage : protected DebugPackage
{
private:
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
        const CachedDebugPackage& Host;
        MessageItem& Item;
        constexpr MessageItemWrapper(const CachedDebugPackage* host, MessageItem& item) noexcept : Host(*host), Item(item) { }
    public:
        [[nodiscard]] common::str::u8string_view Str() const noexcept;
        [[nodiscard]] const WorkItemInfo& Info() const noexcept;
        [[nodiscard]] common::span<const uint32_t> GetDataSpan() const noexcept;
        [[nodiscard]] const MessageBlock& Block() const noexcept;
        [[nodiscard]] constexpr uint32_t ThreadId() const noexcept { return Item.ThreadId; }
    };
    MessageItemWrapper GetByIndex(const size_t idx) const noexcept
    {
        return { this, Items[idx] };
    }
    mutable common::StringPool<u8ch_t> MsgTexts;
    mutable std::vector<MessageItem> Items;
    std::unique_ptr<InfoPack> Infos;
    CachedDebugPackage(const CachedDebugPackage& package) noexcept = default;
public:
    CachedDebugPackage(std::shared_ptr<DebugManager> manager, std::shared_ptr<InfoProvider> infoProv, common::AlignedBuffer&& info, common::AlignedBuffer&& data);
    CachedDebugPackage(CachedDebugPackage&& package) noexcept;
    ~CachedDebugPackage() override;
    using DebugPackage::DebugMan;
    using DebugPackage::InfoMan;
    using DebugPackage::InfoSpan;

    using ItType = common::container::IndirectIterator<const CachedDebugPackage, MessageItemWrapper, &CachedDebugPackage::GetByIndex>;
    friend ItType;
    constexpr ItType begin() const noexcept { return { this, 0 }; }
              ItType end()   const noexcept { return { this, Items.size() }; }
    size_t Count() const noexcept { return Items.size(); }
};


class XCOMPBASAPI ExcelXmlPrinter
{
private:
    common::io::OutputStream& Stream;
    void PrintFileHeader();
    void PrintFileFooter();
public:
    ExcelXmlPrinter(common::io::OutputStream& stream);
    ~ExcelXmlPrinter();
    void PrintPackage(const DebugPackage& package);
    void PrintPackage(const CachedDebugPackage& package);
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

