#include "oclRely.h"
#include "oclContext.h"
#include "oclException.h"
#include "oclDevice.h"
#include "oclUtil.h"

namespace oclu
{


namespace detail
{


static void CL_CALLBACK onNotify(const char * errinfo, const void * private_info, size_t cb, void *user_data)
{
	const _oclContext& ctx = *(_oclContext*)user_data;
	if (ctx.onMessage)
		ctx.onMessage(str::to_u16string(errinfo));
	return;
}

cl_context _oclContext::createContext(const cl_context_properties props[]) const
{
	cl_int ret;
	vector<cl_device_id> deviceIDs;
	for (const auto&dev : devs)
		deviceIDs.push_back(dev->deviceID);
	const auto ctx = clCreateContext(props, 1, deviceIDs.data(), &onNotify, const_cast<_oclContext*>(this), &ret);
	if (ret != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot create opencl-context", ret));
	return ctx;
}

_oclContext::_oclContext(const cl_context_properties props[], const vector<oclDevice>& devices, const u16string name, const Vendor thevendor) :
	devs(devices), context(createContext(props)), PlatformName(name), vendor(thevendor)
{
}

_oclContext::~_oclContext()
{
#ifdef _DEBUG
	uint32_t refCount = 0;
	clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, sizeof(uint32_t), &refCount, nullptr);
	if (refCount == 1)
	{
		oclLog().debug(u"oclContext {:p} named {}, has {} reference being release.\n", (void*)context, PlatformName, refCount);
		clReleaseContext(context);
	}
	else
		oclLog().warning(u"oclContext {:p} named {}, has {} reference and not able to release.\n", (void*)context, PlatformName, refCount);
#else
	clReleaseContext(context);
#endif
}


}



}