#include "oclRely.h"
#include "oclBuffer.h"
#include "oclException.h"
#include "oclUtil.h"
#include "oclPromise.hpp"


namespace oclu
{
MAKE_ENABLER_IMPL(oclBuffer_)


static cl_mem CreateMem(const cl_context ctx, const MemFlag flag, const size_t size, const void* ptr)
{
    cl_int errcode;
    const auto id = clCreateBuffer(ctx, common::enum_cast(flag), size, const_cast<void*>(ptr), &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create memory");
    return id;
}

oclBuffer_::oclBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const cl_mem id)
    : oclMem_(ctx, id, flag), Size(size)
{
}

oclBuffer_::oclBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr)
    : oclBuffer_(ctx, flag, size, CreateMem(ctx->Context, flag, size, ptr))
{
}

oclBuffer_::~oclBuffer_()
{ 
    oclLog().debug(u"oclBuffer {:p} with size {}, being destroyed.\n", (void*)MemID, Size);
}

void* oclBuffer_::MapObject(const cl_command_queue& que, const MapFlag mapFlag)
{
    cl_event e;
    cl_int ret;
    const auto ptr = clEnqueueMapBuffer(que, MemID, CL_TRUE, common::enum_cast(mapFlag), 0, Size, 0, nullptr, &e, &ret);
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
    auto ret = clEnqueueReadBuffer(que->CmdQue, MemID, shouldBlock ? CL_TRUE : CL_FALSE, offset, size, buf, 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot read clMemory");
    if (shouldBlock)
        return {};
    else
        return std::make_shared<oclPromise<void>>(e, que, 0);
}

PromiseResult<void> oclBuffer_::Write(const oclCmdQue& que, const void * const buf, const size_t size, const size_t offset, const bool shouldBlock) const
{
    if (offset >= Size)
        COMMON_THROW(BaseException, u"offset overflow");
    else if (offset + size > Size)
        COMMON_THROW(BaseException, u"write size overflow"); 
    cl_event e;
    const auto ret = clEnqueueWriteBuffer(que->CmdQue, MemID, shouldBlock ? CL_TRUE : CL_FALSE, offset, size, buf, 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot write clMemory");
    if (shouldBlock)
        return {};
    else
        return std::make_shared<oclPromise<void>>(e, que, 0);
}

oclBuffer oclBuffer_::Create(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr)
{
    return MAKE_ENABLER_SHARED(oclBuffer_, ctx, AddMemHostCopyFlag(flag, ptr), size, ptr);
}



}
