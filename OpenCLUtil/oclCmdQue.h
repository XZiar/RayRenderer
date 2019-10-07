#pragma once

#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclCmdQue_;
using oclCmdQue = std::shared_ptr<const oclCmdQue_>;


class OCLUAPI oclCmdQue_ : public common::NonCopyable
{
    friend class GLInterop;
    friend class oclPromiseCore;
    friend class oclContext_;
    friend class oclMem_;
    friend class oclMapPtr_;
    friend class oclBuffer_;
    friend class oclImage_;
    friend class oclProgram_;
    friend class oclKernel_;
private:
    MAKE_ENABLER();
    const oclContext Context;
    const oclDevice Device;
    cl_command_queue CmdQue;
    oclCmdQue_(const oclContext& ctx, const oclDevice& dev, const bool enableProfiling, const bool enableOutOfOrder);
public:
    ~oclCmdQue_();
    void Flush() const;
    void Finish() const;
    bool SupportImplicitGLSync() const { return Device->SupportImplicitGLSync; }

    static oclCmdQue Create(const oclContext& ctx, const oclDevice& dev, const bool enableProfiling = true, const bool enableOutOfOrder = false);
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif