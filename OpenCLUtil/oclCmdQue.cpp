#include "oclRely.h"
#include "oclCmdQue.h"
#include "oclException.h"
#include "oclDevice.h"

namespace oclu
{
MAKE_ENABLER_IMPL(oclCmdQue_)


static cl_command_queue CreateCmdQue(const cl_context ctx, const cl_device_id dev, const bool enableProfiling, const bool enableOutOfOrder)
{
    cl_int errcode;
    cl_command_queue_properties props = 0;
    if (enableProfiling)
        props |= CL_QUEUE_PROFILING_ENABLE;
    if (enableOutOfOrder)
        props |= CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;

    const auto que = clCreateCommandQueue(ctx, dev, props, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create command queue");
    return que;
}

oclCmdQue_::oclCmdQue_(const oclContext& ctx, const oclDevice& dev, const bool enableProfiling, const bool enableOutOfOrder) : Context(ctx), Device(dev),
    cmdque(CreateCmdQue(Context->context, Device->deviceID, enableProfiling && Device->SupportProfiling, enableOutOfOrder && Device->SupportOutOfOrder))
{ }


oclCmdQue_::~oclCmdQue_()
{
    Finish();
    clReleaseCommandQueue(cmdque);
}


void oclCmdQue_::Flush() const
{
    clFlush(cmdque);
}

void oclCmdQue_::Finish() const
{
    clFinish(cmdque);
}

std::shared_ptr<oclCmdQue_> oclCmdQue_::Create(const oclContext& ctx, const oclDevice& dev, const bool enableProfiling, const bool enableOutOfOrder)
{
    return MAKE_ENABLER_SHARED(oclCmdQue_, ctx, dev, enableProfiling, enableOutOfOrder);
}


}
