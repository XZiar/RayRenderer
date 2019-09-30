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


class OCLUAPI oclCmdQue_ : public NonCopyable
{
    friend class oclContext_;
    friend class oclMem_;
    friend class oclMapPtr_;
    friend class GLResLocker;
    friend class GLInterOP;
    friend class oclBuffer_;
    friend class oclImage_;
    friend class oclProgram_;
    friend class oclKernel_;
    template<typename> friend class GLShared;
private:
    MAKE_ENABLER();
    const oclContext Context;
    const oclDevice Device;
    const cl_command_queue cmdque;
    oclCmdQue_(const oclContext& ctx, const oclDevice& dev, const bool enableProfiling, const bool enableOutOfOrder);
public:
    ~oclCmdQue_();
    void Flush() const;
    void Finish() const;
    bool SupportImplicitGLSync() const { return Device->SupportImplicitGLSync; }

    static std::shared_ptr<oclCmdQue_> Create(const oclContext& ctx, const oclDevice& dev, const bool enableProfiling = true, const bool enableOutOfOrder = false);
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif