#include "oclRely.h"
#include "GLInterOP.h"
#include "oclException.h"

namespace oclu
{
namespace detail
{


cl_mem GLInterOP::CreateMemFromGLBuf(const oclContext ctx, const cl_mem_flags flag, const oglu::oglBuffer& buf)
{
    cl_int errcode;
    const auto id = clCreateFromGLBuffer(ctx->context, flag, buf->bufferID, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot create clMem from glBuffer", errcode));
    return id;
}
cl_mem GLInterOP::CreateMemFromGLTex(const oclContext ctx, const cl_mem_flags flag, const oglu::oglTexBase& tex)
{
    cl_int errcode;
    if (oglu::TexFormatUtil::IsCompressType(tex->GetInnerFormat()))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"OpenCL does not support Comressed Texture");
    const auto id = clCreateFromGLTexture(ctx->context, flag, (GLenum)tex->Type, 0, tex->textureID, &errcode);
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