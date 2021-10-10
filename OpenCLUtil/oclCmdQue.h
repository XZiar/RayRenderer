#pragma once

#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{


class OCLUAPI oclCmdQue_ : public detail::oclCommon
{
    friend class GLInterop;
    friend class oclPromiseCore;
    friend class oclContext_;
    friend class oclMem_;
    friend class oclMapPtr_;
    friend class oclSubBuffer_;
    friend class oclImage_;
    friend class oclProgram_;
    friend class oclKernel_;
private:
    MAKE_ENABLER();
    CLHandle<detail::CLCmdQue> CmdQue;
    oclContext Context;
    oclDevice Device;
    bool IsProfiling, IsOutOfOrder;
    oclCmdQue_(const oclContext& ctx, const oclDevice& dev, const bool enableProfiling, const bool enableOutOfOrder);
public:
    ~oclCmdQue_();
    void AddBarrier(const bool force = false) const;
    void Flush() const;
    void Finish() const;
    [[nodiscard]] bool SupportImplicitGLSync() const { return Device->SupportImplicitGLSync; }

    [[nodiscard]] static oclCmdQue Create(const oclContext& ctx, const oclDevice& dev, const bool enableProfiling = true, const bool enableOutOfOrder = false);
};

}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif