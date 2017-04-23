#include "oclRely.h"
#include "oclCmdQue.h"
#include "oclException.h"
#include "oclDevice.h"
#include "oclContext.h"

namespace oclu
{


namespace detail
{


cl_command_queue _oclCmdQue::createCmdQue() const
{
	cl_int errcode;
	const auto que = clCreateCommandQueue(ctx->context, dev->deviceID, NULL, &errcode);
	if (errcode != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot create command queue", errcode));
	return que;
}

_oclCmdQue::_oclCmdQue(const std::shared_ptr<_oclContext>& ctx_, const oclDevice& dev_) : ctx(ctx_), dev(dev_), cmdque(createCmdQue())
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