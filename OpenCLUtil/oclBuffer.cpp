#include "oclPch.h"
#include "oclBuffer.h"
#include "oclException.h"
#include "oclUtil.h"
#include "oclPromise.h"


namespace oclu
{
using common::BaseException;
using common::PromiseResult;
MAKE_ENABLER_IMPL(oclSubBuffer_)
MAKE_ENABLER_IMPL(oclBuffer_)


static cl_mem CreateMem(const cl_context ctx, const MemFlag flag, const size_t size, const void* ptr)
{
    cl_int errcode;
    const auto id = clCreateBuffer(ctx, common::enum_cast(flag), size, const_cast<void*>(ptr), &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create memory");
    return id;
}

oclSubBuffer_::oclSubBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const cl_mem id)
    : oclMem_(ctx, id, flag), Size(size)
{
}

oclSubBuffer_::~oclSubBuffer_()
{ 
    if (Context->ShouldDebugResurce())
        oclLog().debug(u"oclBuffer {:p} with size {}, being destroyed.\n", (void*)MemID, Size);
}

common::span<std::byte> oclSubBuffer_::MapObject(const cl_command_queue& que, const MapFlag mapFlag)
{
    cl_int ret;
    const auto ptr = clEnqueueMapBuffer(que, MemID, CL_TRUE, common::enum_cast(mapFlag), 0, Size, 0, nullptr, nullptr, &ret);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot map clBuffer");
    return common::span<std::byte>(reinterpret_cast<std::byte*>(ptr), Size);
}

PromiseResult<void> oclSubBuffer_::ReadSpan(const common::PromiseStub& pmss, const oclCmdQue& que, common::span<std::byte> buf, const size_t offset) const
{
    Expects(offset < Size); // offset overflow
    Expects(offset + buf.size() <= Size); // read size overflow
    cl_event e;
    DependEvents evts(pmss);
    const auto [evtPtr, evtCnt] = evts.GetWaitList();
    auto ret = clEnqueueReadBuffer(que->CmdQue, MemID, CL_FALSE, offset, buf.size(), buf.data(), evtCnt, evtPtr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot read clBuffer");
    return oclPromise<void>::Create(std::move(evts), e, que);
}

common::PromiseResult<common::AlignedBuffer> oclSubBuffer_::Read(const common::PromiseStub& pmss, const oclCmdQue& que, const size_t offset) const
{
    Expects(offset < Size); // offset overflow
    const auto size = Size - offset;
    common::AlignedBuffer buf(size);
    cl_event e;
    DependEvents evts(pmss);
    const auto [evtPtr, evtCnt] = evts.GetWaitList();
    auto ret = clEnqueueReadBuffer(que->CmdQue, MemID, CL_FALSE, offset, size, buf.GetRawPtr(), evtCnt, evtPtr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot read clBuffer");
    return oclPromise<common::AlignedBuffer>::Create(std::move(evts), e, que, std::move(buf));
}

PromiseResult<void> oclSubBuffer_::WriteSpan(const common::PromiseStub& pmss, const oclCmdQue& que, common::span<const std::byte> buf, const size_t offset) const
{
    Expects(offset < Size); // offset overflow
    Expects(offset + buf.size() <= Size); // write size overflow
    cl_event e;
    DependEvents evts(pmss);
    const auto [evtPtr, evtCnt] = evts.GetWaitList();
    const auto ret = clEnqueueWriteBuffer(que->CmdQue, MemID, CL_FALSE, offset, buf.size(), buf.data(), evtCnt, evtPtr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot write clMemory");
    return oclPromise<void>::Create(std::move(evts), e, que);
}


oclBuffer_::oclBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const cl_mem id)
    : oclSubBuffer_(ctx, flag, size, id)
{
}

oclBuffer_::oclBuffer_(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr)
    : oclSubBuffer_(ctx, flag, size, CreateMem(ctx->Context, flag, size, ptr))
{
}

oclSubBuffer oclBuffer_::CreateSubBuffer(const size_t offset, const size_t size, MemFlag flag) const
{
    if (Context->Version < 11)
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Sub-buffer not supported on pre 1.1");

    if (size == 0)
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Cannot create sub-buffer of 0");
    if (offset >= Size || (offset + size) > Size)
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Sub-buffer region overflow");
    
    flag = oclMem_::ProcessMemFlag(*Context, flag, nullptr);

    if (HAS_FIELD(flag, MemFlag::HostInitMask))
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Sub-buffer does not support initMask");

    if (const auto devAccess = flag & MemFlag::DeviceAccessMask, parentFlag = Flag & MemFlag::DeviceAccessMask;
        devAccess == MemFlag::Empty)
        flag |= Flag & MemFlag::DeviceAccessMask;
    else if (!HAS_FIELD(Flag, MemFlag::ReadWrite) && devAccess != parentFlag)
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Sub-buffer conflicts parent's DeviceAccess");

    if (const auto hostAccess = flag & MemFlag::HostAccessMask, parentFlag = Flag & MemFlag::HostAccessMask;
        hostAccess == MemFlag::Empty)
        flag |= parentFlag;
    else if (parentFlag != MemFlag::Empty && hostAccess != parentFlag)
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"Sub-buffer conflicts parent's HostAccess");

    cl_buffer_region region{ offset, size };
    cl_int errcode;
    const auto id = clCreateSubBuffer(MemID, common::enum_cast(flag), CL_BUFFER_CREATE_TYPE_REGION, &region, &errcode);

    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create sub-buffer");

    return MAKE_ENABLER_SHARED(oclSubBuffer_, (Context, flag, size, id));
}

oclBuffer oclBuffer_::Create(const oclContext& ctx, const MemFlag flag, const size_t size, const void* ptr)
{
    return MAKE_ENABLER_SHARED(oclBuffer_, (ctx, oclMem_::ProcessMemFlag(*ctx, flag, ptr), size, ptr));
}



}
