#include "oclPch.h"
#include "oclBuffer.h"
#include "oclException.h"
#include "oclUtil.h"
#include "oclPromise.hpp"


namespace oclu
{
using common::BaseException;
using common::PromiseResult;
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
    if (Context->ShouldDebugResurce())
        oclLog().debug(u"oclBuffer {:p} with size {}, being destroyed.\n", (void*)MemID, Size);
}

common::span<std::byte> oclBuffer_::MapObject(const cl_command_queue& que, const MapFlag mapFlag)
{
    cl_event e;
    cl_int ret;
    const auto ptr = clEnqueueMapBuffer(que, MemID, CL_TRUE, common::enum_cast(mapFlag), 0, Size, 0, nullptr, &e, &ret);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot map clBuffer");
    return common::span<std::byte>(reinterpret_cast<std::byte*>(ptr), Size);
}

PromiseResult<void> oclBuffer_::ReadSpan(const oclCmdQue& que, common::span<std::byte> buf, const size_t offset, const bool shouldBlock) const
{
    Expects(offset < Size); // offset overflow
    Expects(offset + buf.size() <= Size); // read size overflow
    cl_event e;
    auto ret = clEnqueueReadBuffer(que->CmdQue, MemID, shouldBlock ? CL_TRUE : CL_FALSE, offset, buf.size(), buf.data(), 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot read clBuffer");
    if (shouldBlock)
        return {};
    else
        return oclPromise<void>::Create(e, que);
}

common::PromiseResult<common::AlignedBuffer> oclBuffer_::Read(const oclCmdQue& que, const size_t offset) const
{
    Expects(offset < Size); // offset overflow
    const auto size = Size - offset;
    common::AlignedBuffer buf(size);
    cl_event e;
    auto ret = clEnqueueReadBuffer(que->CmdQue, MemID, CL_FALSE, offset, size, buf.GetRawPtr(), 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot read clBuffer");
    return oclPromise<common::AlignedBuffer>::Create(e, que, std::move(buf));
}

PromiseResult<void> oclBuffer_::WriteSpan(const oclCmdQue& que, common::span<const std::byte> buf, const size_t offset, const bool shouldBlock) const
{
    Expects(offset < Size); // offset overflow
    Expects(offset + buf.size() <= Size); // write size overflow
    cl_event e;
    const auto ret = clEnqueueWriteBuffer(que->CmdQue, MemID, shouldBlock ? CL_TRUE : CL_FALSE, offset, buf.size(), buf.data(), 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot write clMemory");
    if (shouldBlock)
        return {};
    else
        return oclPromise<void>::Create(e, que);
}

oclBuffer oclBuffer_::Create(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr)
{
    return MAKE_ENABLER_SHARED(oclBuffer_, (ctx, AddMemHostCopyFlag(flag, ptr), size, ptr));
}



}
