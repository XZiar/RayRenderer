#include "oclRely.h"
#include "GLInterOP.h"
#include "oclException.h"

namespace oclu
{
namespace detail
{


cl_mem GLInterOP::CreateMemFromGLBuf(const cl_context ctx, const cl_mem_flags flag, const GLuint bufId)
{
    cl_int errcode;
    const auto id = clCreateFromGLBuffer(ctx, flag, bufId, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot create clMem from glBuffer", errcode));
    return id;
}
cl_mem GLInterOP::CreateMemFromGLTex(const cl_context ctx, const cl_mem_flags flag, const cl_GLenum texType, const GLuint texId)
{
    cl_int errcode;
    const auto id = clCreateFromGLTexture(ctx, flag, texType, 0, texId, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot create clMem from glTexture", errcode));
    return id;
}

void GLInterOP::Lock(const cl_command_queue que, const cl_mem mem, const bool needSync) const
{
    if (needSync)
        glFinish();
    cl_int ret = clEnqueueAcquireGLObjects(que, 1, &mem, 0, nullptr, nullptr);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot lock oglObject for oclMemObject", ret));
}

void GLInterOP::Unlock(const cl_command_queue que, const cl_mem mem, const bool needSync) const
{
    if (needSync)
        clFlush(que); // assume promise is correctly waited before relase lock
    cl_int ret = clEnqueueReleaseGLObjects(que, 1, &mem, 0, nullptr, nullptr);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot unlock oglObject for oclMemObject", ret));
}


}
}