#include "oclRely.h"
#include "oclCmdQue.h"
#include "oclException.h"
#include "oclDevice.h"
#include "oclContext.h"

namespace oclu
{


namespace detail
{


cl_command_queue _oclCmdQue::createCmdQue(const bool enableProfiling, const bool enableOutOfOrder) const
{
    cl_int errcode;
    cl_command_queue_properties props = 0;
    if (enableProfiling)
        props |= CL_QUEUE_PROFILING_ENABLE;
    if (enableOutOfOrder)
        props |= CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;

    const auto que = clCreateCommandQueue(ctx->context, dev->deviceID, props, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot create command queue", errcode));
    return que;
}

_oclCmdQue::_oclCmdQue(const std::shared_ptr<_oclContext>& ctx_, const oclDevice& dev_, const bool enableProfiling, const bool enableOutOfOrder) : ctx(ctx_), dev(dev_),
    cmdque(createCmdQue(enableProfiling, enableOutOfOrder))
{
}


_oclCmdQue::~_oclCmdQue()
{
    flush();
    clReleaseCommandQueue(cmdque);
}


void _oclCmdQue::flush() const
{
    clFlush(cmdque);
    clFinish(cmdque);
}


}


}