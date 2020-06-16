#include "oclPch.h"
#include "oclCmdQue.h"
#include "oclException.h"
#include "oclDevice.h"

namespace oclu
{
MAKE_ENABLER_IMPL(oclCmdQue_)



oclCmdQue_::oclCmdQue_(const oclContext& ctx, const oclDevice& dev, const bool enableProfiling, const bool enableOutOfOrder) : 
    Context(ctx), Device(dev), CmdQue(nullptr), 
    IsProfiling(enableProfiling&& Device->SupportProfiling), IsOutOfOrder(enableOutOfOrder&& Device->SupportOutOfOrder)
{
    cl_int errcode;
    cl_command_queue_properties props = 0;
    if (IsProfiling)
        props |= CL_QUEUE_PROFILING_ENABLE;
    if (IsOutOfOrder)
        props |= CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;

    CmdQue = clCreateCommandQueue(Context->Context, *Device, props, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create command queue");
}


oclCmdQue_::~oclCmdQue_()
{
    Finish();
    clReleaseCommandQueue(CmdQue);
}

void oclCmdQue_::AddBarrier(const bool force) const
{
    if (IsOutOfOrder || force)
    {
        const auto errcode = clEnqueueBarrier(CmdQue);
        if (errcode != CL_SUCCESS)
            COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"error when adding barrier to command queue");
    }
}

void oclCmdQue_::Flush() const
{
    clFlush(CmdQue);
}

void oclCmdQue_::Finish() const
{
    clFinish(CmdQue);
}

oclCmdQue oclCmdQue_::Create(const oclContext& ctx, const oclDevice& dev, const bool enableProfiling, const bool enableOutOfOrder)
{
    return MAKE_ENABLER_SHARED(oclCmdQue_, (ctx, dev, enableProfiling, enableOutOfOrder));
}


}
