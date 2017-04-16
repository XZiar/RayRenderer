#include "oclRely.h"
#include "oclComQue.h"
#include "oclDevice.h"
#include "oclContext.h"

namespace oclu
{


namespace detail
{


cl_command_queue _oclComQue::createQueue(const cl_context context, const cl_device_id dev)
{
	cl_int errcode;
	const auto que = clCreateCommandQueue(context, dev, NULL, &errcode);
	if (errcode != CL_SUCCESS)
		throw std::runtime_error("cannot create command queue");
	return que;
}


_oclComQue::_oclComQue(const Wrapper<_oclContext>& ctx, const Wrapper<_oclDevice>& dev)
	: context(ctx), device(dev), comque(createQueue(context->context, dev->deviceID))
{
}


_oclComQue::~_oclComQue()
{
	flush();
	clReleaseCommandQueue(comque);
}


void _oclComQue::flush() const
{
	clFlush(comque);
	clFinish(comque);
}



}


}