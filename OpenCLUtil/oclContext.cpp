#include "oclRely.h"
#include "oclContext.h"
#include "oclException.h"
#include "oclPlatform.h"
#include "oclDevice.h"
#include "oclUtil.h"

namespace oclu
{


namespace detail
{



void CL_CALLBACK _oclContext::onNotify(const char * errinfo, const void * private_info, size_t cb, void *user_data)
{
	const _oclContext& ctx = *(_oclContext*)user_data;
	if (ctx.onMessage)
		ctx.onMessage();
	return;
}

cl_context _oclContext::createContext(const cl_context_properties props[], const vector<oclDevice>& devices) const
{
	cl_int ret;
	vector<cl_device_id> deviceIDs;
	for (const auto&dev : devices)
		deviceIDs.push_back(dev->deviceID);
	const auto ctx = clCreateContext(props, 1, deviceIDs.data(), &onNotify, const_cast<_oclContext*>(this), &ret);
	if (ret != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, L"cannot create opencl-context", to_wstring(oclUtil::getErrorString(ret)));
	return ctx;
}

_oclContext::_oclContext(const cl_context_properties props[], const vector<oclDevice>& devices) : context(createContext(props, devices))
{
}

_oclContext::~_oclContext()
{
	clReleaseContext(context);
}


oclCmdQue _oclContext::createCmdQue(const oclDevice& dev) const
{
	cl_int errcode;
	const auto que = clCreateCommandQueue(context, dev->deviceID, NULL, &errcode);
	if (errcode != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, L"cannot create command queue", to_wstring(oclUtil::getErrorString(errcode)));
	return oclCmdQue(new _oclCmdQue(que));
}

oclProgram _oclContext::loadProgram(const string& src) const
{
	cl_int errcode;
	auto *str = src.c_str();
	size_t size = src.length();
	const auto prog = clCreateProgramWithSource(context, 1, &str, &size, &errcode);
	if (errcode != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, L"cannot create program", to_wstring(oclUtil::getErrorString(errcode)));
	return oclProgram(new _oclProgram(prog, src));
}

}



}