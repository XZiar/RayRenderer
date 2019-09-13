#include "oclRely.h"
#include "oclBuffer.h"
#include "oclException.h"
#include "oclUtil.h"
#include "oclPromise.hpp"


namespace oclu::detail
{


static cl_mem CreateMem(const cl_context ctx, const MemFlag flag, const size_t size, const void* ptr)
{
    cl_int errcode;
    const auto id = clCreateBuffer(ctx, (cl_mem_flags)flag, size, const_cast<void*>(ptr), &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create memory");
    return id;
}

_oclBuffer::_oclBuffer(const oclContext& ctx, const MemFlag flag, const size_t size, const cl_mem id)
    :_oclMem(ctx, id, flag), Size(size)
{
}

_oclBuffer::_oclBuffer(const oclContext& ctx, const MemFlag flag, const size_t size)
    : _oclBuffer(ctx, flag, size, CreateMem(ctx->context, flag, size, nullptr))
{ }

_oclBuffer::_oclBuffer(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr)
    : _oclBuffer(ctx, flag, size, CreateMem(ctx->context, flag, size, ptr))
{
}

_oclBuffer::~_oclBuffer()
{ 
    oclLog().debug(u"oclBuffer {:p} with size {}, being destroyed.\n", (void*)MemID, Size);
}

void* _oclBuffer::MapObject(const oclCmdQue& que, const MapFlag mapFlag)
{
    cl_event e;
    cl_int ret;
    const auto ptr = clEnqueueMapBuffer(que->cmdque, MemID, CL_TRUE, (cl_map_flags)mapFlag, 0, Size, 0, nullptr, &e, &ret);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot map clBuffer");
    return ptr;
}

PromiseResult<void> _oclBuffer::Read(const oclCmdQue& que, void *buf, const size_t size, const size_t offset, const bool shouldBlock) const
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
        return std::make_shared<oclPromiseVoid>(e, que->cmdque);
}

PromiseResult<void> _oclBuffer::Write(const oclCmdQue& que, const void * const buf, const size_t size, const size_t offset, const bool shouldBlock) const
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
        return std::make_shared<oclPromiseVoid>(e, que->cmdque);
}


_oclGLBuffer::_oclGLBuffer(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf)
    : _oclBuffer(ctx, flag, SIZE_MAX, GLInterOP::CreateMemFromGLBuf(ctx, flag, buf)), GLBuf(buf) { }

_oclGLInterBuf::_oclGLInterBuf(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf)
    : _oclGLObject<_oclGLBuffer>(ctx, flag, buf) {}



}
