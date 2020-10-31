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
using DxCmdList         = std::shared_ptr<const DxCmdList_>;
class DxCopyCmdList_;
using DxCopyCmdList     = std::shared_ptr<const DxCopyCmdList_>;
class DxComputeCmdList_;
using DxComputeCmdList  = std::shared_ptr<const DxComputeCmdList_>;
class DxDirectCmdList_;
using DxDirectCmdList   = std::shared_ptr<const DxDirectCmdList_>;
class DxCmdQue_;
using DxCmdQue          = std::shared_ptr<const DxCmdQue_>;
class DxCopyCmdQue_;
using DxCopyCmdQue      = std::shared_ptr<const DxCopyCmdQue_>;
class DxComputeCmdQue_;
using DxComputeCmdQue   = std::shared_ptr<const DxComputeCmdQue_>;
class DxDirectCmdQue_;
using DxDirectCmdQue    = std::shared_ptr<const DxDirectCmdQue_>;


class DXUAPI DxCmdList_ : public common::NonCopyable
{
    friend class DxCmdQue_;
protected:
    enum class ListType { Copy, Compute, Bundle, Direct };
    DxCmdList_(DxDevice device, ListType type, const detail::IIDPPVPair& thelist);
    DxCmdList_(DxDevice device, ListType type);
    struct CmdAllocatorProxy;
    struct CmdListProxy;
    PtrProxy<CmdAllocatorProxy> CmdAllocator;
    PtrProxy<CmdListProxy> CmdList;
public:
    virtual ~DxCmdList_();
};

class DXUAPI DxCopyCmdList_ : public DxCmdList_
{
private:
    MAKE_ENABLER();
protected:
    using DxCmdList_::DxCmdList_;
public:
    [[nodiscard]] static DxCopyCmdList Create(DxDevice device);
};

class DXUAPI DxComputeCmdList_ : public DxCmdList_
{
private:
    MAKE_ENABLER();
protected:
    using DxCmdList_::DxCmdList_;
public:
    [[nodiscard]] static DxComputeCmdList Create(DxDevice device);
};

class DXUAPI DxDirectCmdList_ : public DxCmdList_
{
private:
    MAKE_ENABLER();
    struct GraphicsCmdListProxy;
    DxDirectCmdList_(DxDevice device);
public:
    [[nodiscard]] static DxDirectCmdList Create(DxDevice device);
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
    PtrProxy<CmdQueProxy> CmdQue;
    PtrProxy<FenceProxy> Fence;

    void ExecuteList(const DxCmdList_& list) const;
    common::PromiseResult<void> Signal() const;
    void Wait(const common::PromiseProvider& pms) const;
public:
    virtual ~DxCmdQue_();
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
public:
    common::PromiseResult<void> Execute(const DxCopyCmdList& list) const
    { 
        ExecuteList(*list);
        return Signal();
    }

    [[nodiscard]] static DxCopyCmdQue Create(DxDevice device, bool diableTimeout = false);
};

class DXUAPI DxComputeCmdQue_ : public DxCopyCmdQue_
{
private:
    MAKE_ENABLER();
    using DxCopyCmdQue_::DxCopyCmdQue_;
public:
    using DxCopyCmdQue_::Execute;
    void Execute(const DxComputeCmdList& list) const { return ExecuteList(*list); }

    [[nodiscard]] static DxComputeCmdQue Create(DxDevice device, bool diableTimeout = false);
};

class DXUAPI DxDirectCmdQue_ : public DxComputeCmdQue_
{
private:
    MAKE_ENABLER();
    using DxComputeCmdQue_::DxComputeCmdQue_;
public:
    using DxComputeCmdQue_::Execute;
    void Execute(const DxDirectCmdList& list) const { return ExecuteList(*list); }

    [[nodiscard]] static DxDirectCmdQue Create(DxDevice device, bool diableTimeout = false);
};



}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
