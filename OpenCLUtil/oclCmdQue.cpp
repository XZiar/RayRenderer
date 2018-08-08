#include "oclRely.h"
#include "oclCmdQue.h"
#include "oclException.h"
#include "oclDevice.h"

namespace oclu
{


namespace detail
{


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
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot create command queue", errcode));
    return que;
}

_oclCmdQue::_oclCmdQue(const oclContext& ctx, const oclDevice& dev, const bool enableProfiling, const bool enableOutOfOrder) : Context(ctx), Device(dev),
    cmdque(CreateCmdQue(Context->context, Device->deviceID, enableProfiling && Device->SupportProfiling, enableOutOfOrder && Device->SupportOutOfOrder))
{ }


_oclCmdQue::~_oclCmdQue()
{
    Finish();
    clReleaseCommandQueue(cmdque);
}


void _oclCmdQue::Flush() const
{
    clFlush(cmdque);
}

void _oclCmdQue::Finish() const
{
    clFinish(cmdque);
}


}


}