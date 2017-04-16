#include "oclRely.h"
#include "oclContext.h"
#include "oclDevice.h"

namespace oclu
{


namespace detail
{



void CL_CALLBACK _oclContext::onNotify(const char * errinfo, const void * private_info, size_t cb, void * user_data)
{
	const _oclContext& ctx = *(_oclContext*)user_data;
	if (ctx.onMessage == nullptr)
		return;
	ctx.onMessage();
}

cl_context _oclContext::createContext(const cl_context_properties props[], const vector<Wrapper<_oclDevice>>& devices) const
{
	cl_int ret;
	vector<cl_device_id> deviceIDs;
	for (const auto&dev : devices)
		deviceIDs.push_back(dev->deviceID);
	const auto ctx = clCreateContext(props, 1, deviceIDs.data(), &onNotify, const_cast<_oclContext*>(this), &ret);
	if (ret != CL_SUCCESS)
		throw std::exception("cannot create opencl-context");
	return ctx;
}

_oclContext::_oclContext(const cl_context_properties props[], const vector<Wrapper<_oclDevice>>& devices)
	: context(createContext(props, devices))
{

}

_oclContext::~_oclContext()
{
	clReleaseContext(context);
}


}


}