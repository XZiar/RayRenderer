#pragma once

#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace oclu
{

namespace detail
{


class OCLUAPI _oclCmdQue : public NonCopyable
{
    friend class _oclContext;
    friend class _oclMem;
    friend class _oclMapPtr;
    friend class GLResLocker;
    friend class GLInterOP;
    friend class _oclBuffer;
    friend class _oclImage;
    friend class _oclProgram;
    friend class _oclKernel;
    template<typename> friend class GLShared;
private:
    const oclContext Context;
    const oclDevice Device;
    const cl_command_queue cmdque;
public:
    _oclCmdQue(const oclContext& ctx, const oclDevice& dev, const bool enableProfiling = true, const bool enableOutOfOrder = false);
    ~_oclCmdQue();
    void Flush() const;
    void Finish() const;
    bool SupportImplicitGLSync() const { return Device->SupportImplicitGLSync; }
};


}
using oclCmdQue = Wrapper<detail::_oclCmdQue>;

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif