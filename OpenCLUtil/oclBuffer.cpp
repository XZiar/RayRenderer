#include "oclRely.h"
#include "oclBuffer.h"
#include "oclException.h"
#include "oclUtil.h"
#include "oclPromise.hpp"


namespace oclu::detail
{


static void CL_CALLBACK OnMemDestroyed(cl_mem memobj, void *user_data)
{
    const auto& buf = *reinterpret_cast<_oclBuffer*>(user_data);
    oclLog().debug(u"oclBuffer {:p} with size {}, being destroyed.\n", (void*)memobj, buf.Size);
    //async callback, should not access cl-func since buffer may be released at any time.
    //size_t size = 0;
    //clGetMemObjectInfo(memobj, CL_MEM_SIZE, sizeof(size), &size, nullptr);
}

cl_mem CreateMem(const cl_context ctx, const cl_mem_flags flag, const size_t size)
{
    cl_int errcode;
    const auto id = clCreateBuffer(ctx, flag, size, nullptr, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot create memory", errcode));
    return id;
}

_oclBuffer::_oclBuffer(const oclContext& ctx, const MemFlag flag, const size_t size, const cl_mem id)
    :Context(ctx), Flags(flag), Size(size), memID(id)
{
    clSetMemObjectDestructorCallback(memID, &OnMemDestroyed, this);
}

_oclBuffer::_oclBuffer(const oclContext& ctx, const MemFlag flag, const size_t size)
    : _oclBuffer(ctx, flag, size, CreateMem(ctx->context, (cl_mem_flags)flag, size))
{ }

_oclBuffer::~_oclBuffer()
{
#ifdef _DEBUG
    uint32_t refCount = 0;
    clGetMemObjectInfo(memID, CL_MEM_REFERENCE_COUNT, sizeof(uint32_t), &refCount, nullptr);
    if (refCount == 1)
    {
        oclLog().debug(u"oclBuffer {:p} with size {}, has {} reference being release.\n", (void*)memID, Size, refCount);
        clReleaseMemObject(memID);
    }
    else
        oclLog().warning(u"oclBuffer {:p} with size {}, has {} reference and not able to release.\n", (void*)memID, Size, refCount);
#else
    clReleaseMemObject(memID);
#endif
}

PromiseResult<void> _oclBuffer::Read(const oclCmdQue& que, void *buf, const size_t size, const size_t offset, const bool shouldBlock) const
{
    if (offset >= Size)
        COMMON_THROW(BaseException, u"offset overflow");
    else if (offset + size > Size)
        COMMON_THROW(BaseException, u"read size overflow");
    cl_event e;
    auto ret = clEnqueueReadBuffer(que->cmdque, memID, shouldBlock ? CL_TRUE : CL_FALSE, offset, min(Size - offset, size), buf, 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot read clMemory", ret));
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
    const auto ret = clEnqueueWriteBuffer(que->cmdque, memID, shouldBlock ? CL_TRUE : CL_FALSE, offset, min(Size - offset, size), buf, 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot write clMemory", ret));
    if (shouldBlock)
        return {};
    else
        return std::make_shared<oclPromiseVoid>(e, que->cmdque);
}


_oclGLBuffer::_oclGLBuffer(const oclContext& ctx, const MemFlag flag, const oglu::oglBuffer& buf)
    : _oclBuffer(ctx, flag, SIZE_MAX, CreateMemFromGLBuf(ctx, flag, buf)), GlBuf(buf)
{
}

_oclGLBuffer::~_oclGLBuffer()
{
}


}
