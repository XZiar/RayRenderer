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


class DXUAPI DxCmdList_ : public common::NonCopyable
{
    friend class DxCmdQue_;
    friend class DxResource_;
protected:
    enum class ListType { Copy, Compute, Bundle, Direct };
    DxCmdList_(DxDevice device, ListType type, const DxCmdList_* prevList = nullptr);
    struct CmdAllocatorProxy;
    struct CmdListProxy;
    PtrProxy<CmdAllocatorProxy> CmdAllocator;
    PtrProxy<CmdListProxy> CmdList;
    std::vector<std::pair<void*, ResourceState>> ResStateTable;
    std::atomic_flag HasClosed;

    std::optional<ResourceState> UpdateResState(void* res, const ResourceState state);
public:
    virtual ~DxCmdList_();
    void EnsureClosed();
    void Reset(const bool resetResState = true);
};

class DXUAPI DxCopyCmdList_ : public DxCmdList_
{
private:
    MAKE_ENABLER();
    using DxCmdList_::DxCmdList_;
public:
    [[nodiscard]] static DxCopyCmdList Create(DxDevice device, const DxCmdList& prevList = {});
};

class DXUAPI DxComputeCmdList_ : public DxCmdList_
{
private:
    MAKE_ENABLER();
    using DxCmdList_::DxCmdList_;
public:
    [[nodiscard]] static DxComputeCmdList Create(DxDevice device, const DxCmdList& prevList = {});
};

class DXUAPI DxDirectCmdList_ : public DxCmdList_
{
private:
    MAKE_ENABLER();
    using DxCmdList_::DxCmdList_;
public:
    [[nodiscard]] static DxDirectCmdList Create(DxDevice device, const DxCmdList& prevList = {});
};


class DXUAPI COMMON_EMPTY_BASES DxCmdQue_ : public common::NonCopyable, public std::enable_shared_from_this<DxCmdQue_>
{
    friend class DxPromiseCore;
private:
    mutable std::atomic<uint64_t> FenceNum;
protected:
    enum class QueType { Copy, Compute, Direct };
    DxCmdQue_(DxDevice device, QueType type, bool diableTimeout);
    struct FenceProxy;
    struct CmdQueProxy;
    const DxDevice Device;
    PtrProxy<CmdQueProxy> CmdQue;
    PtrProxy<FenceProxy> Fence; 

    virtual QueType GetType() const noexcept = 0;
    void ExecuteList(DxCmdList_& list) const;
    common::PromiseResult<void> Signal() const;
    void Wait(const common::PromiseProvider& pms) const;
public:
    virtual ~DxCmdQue_();

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
    QueType GetType() const noexcept override { return QueType::Copy; };
public:
    common::PromiseResult<void> Execute(const DxCopyCmdList& list) const { return ExecuteAnyList(list); }

    [[nodiscard]] static DxCopyCmdQue Create(DxDevice device, bool diableTimeout = false);
};

class DXUAPI DxComputeCmdQue_ : public DxCmdQue_
{
private:
    MAKE_ENABLER();
    using DxCmdQue_::DxCmdQue_;
    QueType GetType() const noexcept override { return QueType::Compute; };
public:
    common::PromiseResult<void> Execute(const DxComputeCmdList& list) const { return ExecuteAnyList(list); }

    [[nodiscard]] static DxComputeCmdQue Create(DxDevice device, bool diableTimeout = false);
};

class DXUAPI DxDirectCmdQue_ : public DxCmdQue_
{
private:
    MAKE_ENABLER();
    using DxCmdQue_::DxCmdQue_;
    QueType GetType() const noexcept override { return QueType::Direct; };
public:
    common::PromiseResult<void> Execute(const DxDirectCmdList& list) const { return ExecuteAnyList(list); }

    [[nodiscard]] static DxDirectCmdQue Create(DxDevice device, bool diableTimeout = false);
};



}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
