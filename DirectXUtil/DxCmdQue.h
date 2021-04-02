#pragma once
#include "DxRely.h"
#include "DxDevice.h"
#include "DxPromise.h"
#include "DxQuery.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DxCmdList_;
using DxCmdList         = std::shared_ptr<DxCmdList_>;
class DxCopyCmdList_;
using DxCopyCmdList     = std::shared_ptr<DxCopyCmdList_>;
class DxComputeCmdList_;
using DxComputeCmdList  = std::shared_ptr<DxComputeCmdList_>;
class DxDirectCmdList_;
using DxDirectCmdList   = std::shared_ptr<DxDirectCmdList_>;
class DxCmdQue_;
using DxCmdQue          = std::shared_ptr<DxCmdQue_>;
class DxCopyCmdQue_;
using DxCopyCmdQue      = std::shared_ptr<DxCopyCmdQue_>;
class DxComputeCmdQue_;
using DxComputeCmdQue   = std::shared_ptr<DxComputeCmdQue_>;
class DxDirectCmdQue_;
using DxDirectCmdQue    = std::shared_ptr<DxDirectCmdQue_>;

class DxResource_;
class DxProgram_;


namespace detail
{
struct ResStateRecord
{
    const DxResource_* Resource;
    ResourceState State;
};
struct DXUAPI DebugEventHolder : public std::enable_shared_from_this<DebugEventHolder>
{
private:
    struct Range;
    std::weak_ptr<Range> CurrentRange;
    virtual void BeginEvent(std::u16string_view msg) const = 0;
    virtual void EndEvent() const = 0;
protected:
    bool CheckRangeEmpty() const noexcept;
public:
    virtual ~DebugEventHolder();
    virtual void AddMarker(std::u16string name) const = 0;
    std::shared_ptr<void> DeclareRange(std::u16string msg);
};
}

class DXUAPI ResStateList
{
    friend DxCmdList_;
    std::vector<detail::ResStateRecord> Records;
public:
    void AddState(const DxResource_* res, ResourceState state);
};


class DXUAPI DxCmdList_ : public DxNamable, public detail::DebugEventHolder
{
    friend DxQueryProvider;
    friend DxCmdQue_;
    friend DxResource_;
    friend DxProgram_;
private:
    [[nodiscard]] void* GetD3D12Object() const noexcept final;
    void BeginEvent(std::u16string_view msg) const final;
    void EndEvent() const final;
protected:
    enum class ListType { Copy, Compute, Bundle, Direct };
    struct ResStateRecord
    {
        const DxResource_* Resource;
        ResourceState State;
        ResourceState FromState;
        bool IsPromote;
        bool IsBufOrSATex;
        ResStateRecord(const DxResource_* res, const ResourceState state) noexcept;
    };
    PtrProxy<detail::CmdAllocator> CmdAllocator;
    PtrProxy<detail::CmdList> CmdList;
    DxDevice Device;
    std::shared_ptr<DxQueryProvider> QueryProvider;
    std::vector<ResStateRecord> ResStateTable;
    ResStateList EndResStates;
    const ListType Type;
    const bool SupportTimestamp;
    bool HasClosed;
    
    DxCmdList_(DxDevice device, ListType type);
    COMMON_NO_COPY(DxCmdList_)
    COMMON_NO_MOVE(DxCmdList_)
    void InitResTable(const ResStateList& list);
    void InitResTable(const DxCmdList_& list);
    void InitResTable(const DxCmdList& list) { InitResTable(*list); }
    template<typename T>
    void HandleBeginEndStates(T&& begin)
    {
        InitResTable(std::forward<T>(begin));
    }
    template<typename T, typename U>
    void HandleBeginEndStates(T&& begin, U&& end)
    {
        InitResTable(std::forward<T>(begin));
        EndResStates = std::forward<T>(end);
    }
    [[nodiscard]] DxQueryProvider& GetQueryProvider();
    bool UpdateResState(const DxResource_* res, const ResourceState newState, bool fromInitState);
public:
    enum class Capability : uint32_t { Copy = 0x1, Compute = 0x2, Graphic = 0x4 };
    ~DxCmdList_() override;
    void AddMarker(std::u16string name) const final;
    void FlushResourceState();
    [[nodiscard]] ResStateList GenerateStateList() const;
    [[nodiscard]] DxTimestampToken CaptureTimestamp();
    [[nodiscard]] bool IsClosed() const noexcept { return HasClosed; }
    void Close();
    void Reset(const bool resetResState = true);
};
MAKE_ENUM_BITFIELD(DxCmdList_::Capability)

class DXUAPI COMMON_EMPTY_BASES DxCopyCmdList_ : public DxCmdList_
{
private:
    MAKE_ENABLER();
    using DxCmdList_::DxCmdList_;
public:
    static constexpr auto CmdCaps = Capability::Copy;
    [[nodiscard]] static DxCopyCmdList Create(DxDevice device);
    template<typename T, typename... Args>
    [[nodiscard]] static DxCopyCmdList Create(DxDevice device, T&& arg, Args&&... args)
    {
        auto list = Create(device);
        list->HandleBeginEndStates(std::forward<T>(arg), std::forward<Args>(args)...);
        return list;
    }
};

class DXUAPI COMMON_EMPTY_BASES DxComputeCmdList_ : public DxCmdList_
{
private:
    MAKE_ENABLER();
    using DxCmdList_::DxCmdList_;
public:
    static constexpr auto CmdCaps = Capability::Copy | Capability::Compute;
    [[nodiscard]] static DxComputeCmdList Create(DxDevice device);
    template<typename T, typename... Args>
    [[nodiscard]] static DxComputeCmdList Create(DxDevice device, T&& arg, Args&&... args)
    {
        auto list = Create(device);
        list->HandleBeginEndStates(std::forward<T>(arg), std::forward<Args>(args)...);
        return list;
    }
};

class DXUAPI COMMON_EMPTY_BASES DxDirectCmdList_ : public DxCmdList_
{
private:
    MAKE_ENABLER();
    using DxCmdList_::DxCmdList_;
public:
    static constexpr auto CmdCaps = Capability::Copy | Capability::Compute | Capability::Graphic;
    [[nodiscard]] static DxDirectCmdList Create(DxDevice device);
    template<typename T, typename... Args>
    [[nodiscard]] static DxDirectCmdList Create(DxDevice device, T&& arg, Args&&... args)
    {
        auto list = Create(device);
        list->HandleBeginEndStates(std::forward<T>(arg), std::forward<Args>(args)...);
        return list;
    }
};


class DXUAPI COMMON_EMPTY_BASES DxCmdQue_ : public DxNamable, public detail::DebugEventHolder
{
    friend DxQueryProvider;
    friend DxPromiseCore;
private:
    mutable std::atomic<uint64_t> FenceNum;
    uint64_t TimestampFreq;
    [[nodiscard]] void* GetD3D12Object() const noexcept final;
    void BeginEvent(std::u16string_view msg) const final;
    void EndEvent() const final;
    static void EnsureClosed(const DxCmdList_& list);
protected:
    enum class QueType { Copy, Compute, Direct };
    COMMON_NO_COPY(DxCmdQue_)
    COMMON_NO_MOVE(DxCmdQue_)
    DxCmdQue_(DxDevice device, QueType type, bool diableTimeout);
    const DxDevice Device;
    PtrProxy<detail::CmdQue> CmdQue;
    PtrProxy<detail::Fence> Fence; 

    virtual QueType GetType() const noexcept = 0;
    void ExecuteList(const DxCmdList_& list) const;
    std::variant<uint64_t, DxException> SignalNum() const;
    template<typename T = void, typename... Args>
    common::PromiseResult<T> Signal(Args&&... args) const
    {
        auto ret = SignalNum();
        if (ret.index() == 0)
            return DxPromise<T>::Create(*this, std::get<0>(ret), std::forward<Args>(args)...);
        else
            return DxPromise<T>::CreateError(std::get<1>(ret));
    }
    void Wait(const common::PromiseProvider& pms) const;
public:
    ~DxCmdQue_() override;
    constexpr uint64_t GetTimestampFreq() const noexcept { return TimestampFreq; }
    void AddMarker(std::u16string name) const final;
    template<typename... Args>
    DxCmdList CreateList(Args&&... args) const
    {
        const auto type = GetType();
        switch (type)
        {
        case QueType::Copy:     return DxCopyCmdList_   ::Create(Device, std::forward<Args>(args)...);
        case QueType::Compute:  return DxComputeCmdList_::Create(Device, std::forward<Args>(args)...);
        case QueType::Direct:   return DxDirectCmdList_ ::Create(Device, std::forward<Args>(args)...);
        default:                return {};
        }
    }
    common::PromiseResult<void> ExecuteAnyList(const DxCmdList& list) const
    {
        ExecuteList(*list);
        return Signal();
    }
    common::PromiseResult<DxQueryResolver> ExecuteAnyListWithQuery(const DxCmdList& list) const;
    template<typename T>
    void Wait(const common::PromiseResult<T>& pms) const
    {
        Wait(pms->GetRootPromise());
    }
};

class DXUAPI COMMON_EMPTY_BASES DxCopyCmdQue_ : public DxCmdQue_
{
protected:
    MAKE_ENABLER();
    using DxCmdQue_::DxCmdQue_;
    QueType GetType() const noexcept final { return QueType::Copy; };
public:
    static constexpr auto CmdCaps = DxCmdList_::Capability::Copy;
    common::PromiseResult<void> Execute(const DxCopyCmdList& list) const { return ExecuteAnyList(list); }
    common::PromiseResult<DxQueryResolver> ExecuteWithQuery(const DxCopyCmdList& list) const { return ExecuteAnyListWithQuery(list); }

    [[nodiscard]] static DxCopyCmdQue Create(DxDevice device, bool diableTimeout = false);
};

class DXUAPI COMMON_EMPTY_BASES DxComputeCmdQue_ : public DxCmdQue_
{
private:
    MAKE_ENABLER();
    using DxCmdQue_::DxCmdQue_;
    QueType GetType() const noexcept final { return QueType::Compute; };
public:
    static constexpr auto CmdCaps = DxCmdList_::Capability::Copy | DxCmdList_::Capability::Compute;
    common::PromiseResult<void> Execute(const DxComputeCmdList& list) const { return ExecuteAnyList(list); }
    common::PromiseResult<DxQueryResolver> ExecuteWithQuery(const DxComputeCmdList& list) const { return ExecuteAnyListWithQuery(list); }

    [[nodiscard]] static DxComputeCmdQue Create(DxDevice device, bool diableTimeout = false);
};

class DXUAPI COMMON_EMPTY_BASES DxDirectCmdQue_ : public DxCmdQue_
{
private:
    MAKE_ENABLER();
    using DxCmdQue_::DxCmdQue_;
    QueType GetType() const noexcept final { return QueType::Direct; };
public:
    static constexpr auto CmdCaps = DxCmdList_::Capability::Copy | DxCmdList_::Capability::Compute | DxCmdList_::Capability::Graphic;
    common::PromiseResult<void> Execute(const DxDirectCmdList& list) const { return ExecuteAnyList(list); }
    common::PromiseResult<DxQueryResolver> ExecuteWithQuery(const DxDirectCmdList& list) const { return ExecuteAnyListWithQuery(list); }

    [[nodiscard]] static DxDirectCmdQue Create(DxDevice device, bool diableTimeout = false);
};


#define DXU_CMD_CHECK(T, Cap, Type) \
    static_assert(HAS_FIELD(T::CmdCaps, DxCmdList_::Capability::Cap) && std::is_base_of_v<DxCmd##Type##_, T>, "need a " #Cap " " #Type)



}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
