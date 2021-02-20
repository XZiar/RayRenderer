#pragma once
#include "DxRely.h"
#include "DxDevice.h"
#include "DxPromise.h"


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
using DxCmdQue          = std::shared_ptr<const DxCmdQue_>;
class DxCopyCmdQue_;
using DxCopyCmdQue      = std::shared_ptr<const DxCopyCmdQue_>;
class DxComputeCmdQue_;
using DxComputeCmdQue   = std::shared_ptr<const DxComputeCmdQue_>;
class DxDirectCmdQue_;
using DxDirectCmdQue    = std::shared_ptr<const DxDirectCmdQue_>;

class DxResource_;
class DxProgram_;


class DXUAPI DxCmdList_ : public DxNamable
{
    friend DxCmdQue_;
    friend DxResource_;
    friend DxProgram_;
private:
    [[nodiscard]] void* GetD3D12Object() const noexcept final;
protected:
    enum class ListType { Copy, Compute, Bundle, Direct };
    struct ResStateRecord
    {
        void* Resource;
        ResourceState State;
        bool IsPromote;
        bool IsBufOrSATex;
        ResStateRecord(const DxResource_* res, const ResourceState state) noexcept;
    };
    COMMON_NO_COPY(DxCmdList_)
    COMMON_NO_MOVE(DxCmdList_)
    DxCmdList_(DxDevice device, ListType type, const DxCmdList_* prevList = nullptr);
    PtrProxy<detail::CmdAllocator> CmdAllocator;
    PtrProxy<detail::CmdList> CmdList;
    std::vector<ResStateRecord> ResStateTable;
    const ListType Type;
    std::atomic<bool> HasClosed;

    bool UpdateResState(const DxResource_* res, const ResourceState newState, bool fromInitState);
public:
    enum class Capability : uint32_t { Copy = 0x1, Compute = 0x2, Graphic = 0x4 };
    ~DxCmdList_() override;
    bool IsClosed() const noexcept;
    void EnsureClosed();
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
    [[nodiscard]] static DxCopyCmdList Create(DxDevice device, const DxCmdList& prevList = {});
};

class DXUAPI COMMON_EMPTY_BASES DxComputeCmdList_ : public DxCmdList_
{
private:
    MAKE_ENABLER();
    using DxCmdList_::DxCmdList_;
public:
    static constexpr auto CmdCaps = Capability::Copy | Capability::Compute;
    [[nodiscard]] static DxComputeCmdList Create(DxDevice device, const DxCmdList& prevList = {});
};

class DXUAPI COMMON_EMPTY_BASES DxDirectCmdList_ : public DxCmdList_
{
private:
    MAKE_ENABLER();
    using DxCmdList_::DxCmdList_;
public:
    static constexpr auto CmdCaps = Capability::Copy | Capability::Compute | Capability::Graphic;
    [[nodiscard]] static DxDirectCmdList Create(DxDevice device, const DxCmdList& prevList = {});
};


class DXUAPI COMMON_EMPTY_BASES DxCmdQue_ : public DxNamable, public std::enable_shared_from_this<DxCmdQue_>
{
    friend class DxPromiseCore;
private:
    mutable std::atomic<uint64_t> FenceNum;
    [[nodiscard]] void* GetD3D12Object() const noexcept final;
protected:
    enum class QueType { Copy, Compute, Direct };
    COMMON_NO_COPY(DxCmdQue_)
    COMMON_NO_MOVE(DxCmdQue_)
    DxCmdQue_(DxDevice device, QueType type, bool diableTimeout);
    const DxDevice Device;
    PtrProxy<detail::CmdQue> CmdQue;
    PtrProxy<detail::Fence> Fence; 

    virtual QueType GetType() const noexcept = 0;
    void ExecuteList(DxCmdList_& list) const;
    common::PromiseResult<void> Signal() const;
    void Wait(const common::PromiseProvider& pms) const;
public:
    ~DxCmdQue_() override;

    DxCmdList CreateList(const DxCmdList& prevList = {}) const;
    common::PromiseResult<void> ExecuteAnyList(const DxCmdList& list) const
    {
        ExecuteList(*list);
        return Signal();
    }
    template<typename T>
    void Wait(const common::PromiseResult<T>& pms) const
    {
        Wait(pms->GetPromise());
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

    [[nodiscard]] static DxDirectCmdQue Create(DxDevice device, bool diableTimeout = false);
};


#define DXU_CMD_CHECK(T, Cap, Type) \
    static_assert(HAS_FIELD(T::CmdCaps, DxCmdList_::Capability::Cap) && std::is_base_of_v<DxCmd##Type##_, T>, "need a " #Cap " " #Type)



}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
