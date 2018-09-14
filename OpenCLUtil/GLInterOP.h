#pragma once
#include "oclRely.h"
#include "oclCmdQue.h"

namespace oclu
{
namespace detail
{

class OCLUAPI GLInterOP
{
protected:
    static cl_mem CreateMemFromGLBuf(const oclContext ctx, const cl_mem_flags flag, const oglu::oglBuffer& bufId);
    static cl_mem CreateMemFromGLTex(const oclContext ctx, const cl_mem_flags flag, const oglu::oglTexBase& tex);
    void Lock(const cl_command_queue que, const cl_mem mem, const bool needSync = true) const;
    void Unlock(const cl_command_queue que, const cl_mem mem, const bool needSync = true) const;
};
template<typename Child>
class COMMONTPL GLShared : protected GLInterOP
{
protected:
public:
    void Lock(const oclCmdQue& que) const
    {
        GLInterOP::Lock(que->cmdque, static_cast<const Child*>(this)->memID, !que->SupportImplicitGLSync());
    }
    void Unlock(const oclCmdQue& que) const
    {
        GLInterOP::Unlock(que->cmdque, static_cast<const Child*>(this)->memID, !que->SupportImplicitGLSync());
    }
};


}
}