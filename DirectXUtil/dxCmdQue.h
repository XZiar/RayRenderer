#pragma once
#include "dxRely.h"
#include "dxDevice.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DXCmdQue_;
using DXCmdQue        = std::shared_ptr<const DXCmdQue_>;
class DXComputeCmdQue_;
using DXComputeCmdQue = std::shared_ptr<const DXComputeCmdQue_>;
class DXDirectCmdQue_;
using DXDirectCmdQue  = std::shared_ptr<const DXDirectCmdQue_>;

class DXUAPI DXCmdQue_ : public common::NonCopyable
{
protected:
    MAKE_ENABLER();
    enum class QueType { Copy, Compute, Direct };
    DXCmdQue_(DXDevice device, QueType type, bool diableTimeout);
    struct CmdQueProxy;
    CmdQueProxy* CmdQue;
public:
    virtual ~DXCmdQue_();
    
    static DXCmdQue Create(DXDevice device, bool diableTimeout = false);
};

class DXUAPI DXComputeCmdQue_ : public DXCmdQue_
{
private:
    MAKE_ENABLER();
    using DXCmdQue_::DXCmdQue_;
public:
    static DXComputeCmdQue Create(DXDevice device, bool diableTimeout = false);

};

class DXUAPI DXDirectCmdQue_ : public DXComputeCmdQue_
{
private:
    MAKE_ENABLER();
    using DXComputeCmdQue_::DXComputeCmdQue_;
public:
    static DXDirectCmdQue Create(DXDevice device, bool diableTimeout = false);

};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
