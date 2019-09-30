#include "oclRely.h"
#include "oclBuffer.h"
#include "oclException.h"
#include "oclUtil.h"
#include "oclPromise.hpp"


namespace oclu
{
MAKE_ENABLER_IMPL(oclBuffer_)
MAKE_ENABLER_IMPL(oclGLInterBuf_)


static cl_mem CreateMem(const cl_context ctx, const MemFlag flag, const size_t size, const void* ptr)
{
    cl_int errcode;
    const auto id = clCreateBuffer(ctx, (cl_mem_flags)flag, size, const_cast<void*>(ptr), &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create memory");
    return id;
}

oclBuffer_::oclBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const cl_mem id)
    : oclMem_(ctx, id, flag), Size(size)
{
}

oclBuffer_::oclBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr)
    : oclBuffer_(ctx, flag, size, CreateMem(ctx->context, flag, size, ptr))
{
}

oclBuffer_::~oclBuffer_()
{ 
    oclLog().debug(u"oclBuffer {:p} with size {}, being destroyed.\n", (void*)MemID, Size);
}

void* oclBuffer_::MapObject(const oclCmdQue& que, const MapFlag mapFlag)
{
    cl_event e;
    cl_int ret;
    const auto ptr = clEnqueueMapBuffer(que->cmdque, MemID, CL_TRUE, (cl_map_flags)mapFlag, 0, Size, 0, nullptr, &e, &ret);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot map clBuffer");
    return ptr;
}

PromiseResult<void> oclBuffer_::Read(const oclCmdQue& que, void *buf, const size_t size, const size_t offset, const bool shouldBlock) const
{
    if (offset >= Size)
        COMMON_THROW(BaseException, u"offset overflow");
    else if (offset + size > Size)
        COMMON_THROW(BaseException, u"read size overflow");
    cl_event e;
    auto ret = clEnqueueReadBuffer(que->cmdque, MemID, shouldBlock ? CL_TRUE : CL_FALSE, offset, size, buf, 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot read clMemory");
    if (shouldBlock)
        return {};
    else
        return std::make_shared<detail::oclPromiseVoid>(e, que->cmdque);
}

PromiseResult<void> oclBuffer_::Write(const oclCmdQue& que, const void * const buf, const size_t size, const size_t offset, const bool shouldBlock) const
{
    if (offset >= Size)
        COMMON_THROW(BaseException, u"offset overflow");
    else if (offset + size > Size)
        COMMON_THROW(BaseException, u"write size overflow"); 
    cl_event e;
    const auto ret = clEnqueueWriteBuffer(que->cmdque, MemID, shouldBlock ? CL_TRUE : CL_FALSE, offset, size, buf, 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot write clMemory");
    if (shouldBlock)
        return {};
    else
        return std::make_shared<detail::oclPromiseVoid>(e, que->cmdque);
}

oclBuffer oclBuffer_::Create(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr)
{
    return MAKE_ENABLER_SHARED(oclBuffer_, ctx, AddMemHostCopyFlag(flag, ptr), size, ptr);
}


oclGLBuffer_::oclGLBuffer_(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf)
    : oclBuffer_(ctx, flag, SIZE_MAX, GLInterOP::CreateMemFromGLBuf(ctx, flag, buf)), GLBuf(buf) { }

oclGLBuffer_::~oclGLBuffer_() {}


oclGLInterBuf_::oclGLInterBuf_(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf)
    : oclGLObject_<oclGLBuffer_>(MAKE_ENABLER_UNIQUE(oclGLBuffer_, ctx, flag, buf)) {}

oclGLInterBuf oclGLInterBuf_::Create(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf)
{
    return MAKE_ENABLER_SHARED(oclGLInterBuf_, ctx, flag, buf);
}


}
