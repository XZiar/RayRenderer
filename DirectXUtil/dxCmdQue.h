#pragma once
#include "dxRely.h"
#include "dxDevice.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DXCmdList_;
using DXCmdList         = std::shared_ptr<const DXCmdList_>;
class DXCopyCmdList_;
using DXCopyCmdList     = std::shared_ptr<const DXCopyCmdList_>;
class DXComputeCmdList_;
using DXComputeCmdList  = std::shared_ptr<const DXComputeCmdList_>;
class DXDirectCmdList_;
using DXDirectCmdList   = std::shared_ptr<const DXDirectCmdList_>;
class DXCopyCmdQue_;
using DXCopyCmdQue      = std::shared_ptr<const DXCopyCmdQue_>;
class DXComputeCmdQue_;
using DXComputeCmdQue   = std::shared_ptr<const DXComputeCmdQue_>;
class DXDirectCmdQue_;
using DXDirectCmdQue    = std::shared_ptr<const DXDirectCmdQue_>;


class DXUAPI DXCmdList_ : public common::NonCopyable
{
    friend class DXCopyCmdQue_;
protected:
    enum class ListType { Copy, Compute, Bundle, Direct };
    DXCmdList_(DXDevice device, ListType type, const detail::IIDPPVPair& thelist);
    DXCmdList_(DXDevice device, ListType type);
    struct CmdAllocatorProxy;
    struct CmdListProxy;
    PtrProxy<CmdAllocatorProxy> CmdAllocator;
    PtrProxy<CmdListProxy> CmdList;
public:
    virtual ~DXCmdList_();
};

class DXUAPI DXCopyCmdList_ : public DXCmdList_
{
private:
    MAKE_ENABLER();
protected:
    using DXCmdList_::DXCmdList_;
public:
    [[nodiscard]] static DXCopyCmdList Create(DXDevice device);
};

class DXUAPI DXComputeCmdList_ : public DXCopyCmdList_
{
private:
    MAKE_ENABLER();
protected:
    using DXCopyCmdList_::DXCopyCmdList_;
public:
    [[nodiscard]] static DXComputeCmdList Create(DXDevice device);
};

class DXUAPI DXDirectCmdList_ : public DXComputeCmdList_
{
private:
    MAKE_ENABLER();
    struct GraphicsCmdListProxy;
    DXDirectCmdList_(DXDevice device);
public:
    [[nodiscard]] static DXDirectCmdList Create(DXDevice device);
};


class DXUAPI DXCopyCmdQue_ : public common::NonCopyable
{
protected:
    MAKE_ENABLER();
    enum class QueType { Copy, Compute, Direct };
    DXCopyCmdQue_(DXDevice device, QueType type, bool diableTimeout);
    struct CmdQueProxy;
    PtrProxy<CmdQueProxy> CmdQue;

    void ExecuteList(const DXCmdList_& list) const;
public:
    virtual ~DXCopyCmdQue_();
    void Execute(const DXCopyCmdList& list) const { return ExecuteList(*list); }

    [[nodiscard]] static DXCopyCmdQue Create(DXDevice device, bool diableTimeout = false);
};

class DXUAPI DXComputeCmdQue_ : public DXCopyCmdQue_
{
private:
    MAKE_ENABLER();
    using DXCopyCmdQue_::DXCopyCmdQue_;
public:
    using DXCopyCmdQue_::Execute;
    void Execute(const DXComputeCmdList& list) const { return ExecuteList(*list); }

    [[nodiscard]] static DXComputeCmdQue Create(DXDevice device, bool diableTimeout = false);
};

class DXUAPI DXDirectCmdQue_ : public DXComputeCmdQue_
{
private:
    MAKE_ENABLER();
    using DXComputeCmdQue_::DXComputeCmdQue_;
public:
    using DXComputeCmdQue_::Execute;
    void Execute(const DXDirectCmdList& list) const { return ExecuteList(*list); }

    [[nodiscard]] static DXDirectCmdQue Create(DXDevice device, bool diableTimeout = false);
};



}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
