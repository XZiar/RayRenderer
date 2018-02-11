#include "oclRely.h"
#include "oclPlatform.h"

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

namespace oclu
{


namespace detail
{


bool _oclPlatform::checkGL() const
{
	if (name.find(L"Experimental") != wstring::npos)
		return false;
	if (!common::findvec(devs, [](auto& dev) { return dev->type == DeviceType::GPU; }))// no GPU
		return false;
	//Additional attributes to OpenCL context creation
	//which associate an OpenGL context with the OpenCL context 
	cl_context_properties props[] =
	{
		//OpenCL platform
		CL_CONTEXT_PLATFORM, (cl_context_properties)platformID,
		//OpenGL context
		CL_GL_CONTEXT_KHR,   (cl_context_properties)wglGetCurrentContext(),
		//HDC used to create the OpenGL context
		CL_WGL_HDC_KHR,      (cl_context_properties)wglGetCurrentDC(),
		0
	};
	clGetGLContextInfoKHR_fn clGetGLContext = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(platformID, "clGetGLContextInfoKHR");
	cl_device_id dID;
	cl_int ret = clGetGLContext(props, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &dID, NULL);
	return ret == CL_SUCCESS;
}

Vendor judgeBrand(const wstring& name)
{
	if (str::ifind_first(name, L"nvidia").has_value())
		return Vendor::NVIDIA;
	else if (str::ifind_first(name, L"amd").has_value())
		return Vendor::AMD;
	else if (str::ifind_first(name, L"intel").has_value())
		return Vendor::Intel;
	else
		return Vendor::Other;
}

wstring _oclPlatform::getStr(const cl_platform_info type) const
{
	char str[128] = { 0 };
	clGetPlatformInfo(platformID, type, 127, str, NULL);
	return str::to_wstring(str);
}

_oclPlatform::_oclPlatform(const cl_platform_id pID)
	: platformID(pID), name(getStr(CL_PLATFORM_NAME)), ver(getStr(CL_PLATFORM_VERSION)), vendor(judgeBrand(name))
{
	cl_device_id defDevID;
	clGetDeviceIDs(platformID, CL_DEVICE_TYPE_DEFAULT, 1, &defDevID, NULL);
	cl_uint numDevices;
	clGetDeviceIDs(platformID, CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
	// Get all Device Info
	vector<cl_device_id> deviceIDs(numDevices);
	clGetDeviceIDs(platformID, CL_DEVICE_TYPE_ALL, numDevices, deviceIDs.data(), NULL);

	for (const auto & dID : deviceIDs)
	{
		devs.push_back(oclDevice(new _oclDevice(dID)));
		if (dID == defDevID)
			defDev = devs.back();
	}
	isCurGL = checkGL();
}

oclContext _oclPlatform::createContext(const bool needGLOp) const
{
	vector<cl_context_properties> props;
	//OpenCL platform
	props.assign({ CL_CONTEXT_PLATFORM, (cl_context_properties)platformID });
	if (isCurGL && needGLOp)
	{
		props.insert(props.cend(), 
		{
			//OpenGL context
			CL_GL_CONTEXT_KHR,   (cl_context_properties)wglGetCurrentContext(),
			//HDC used to create the OpenGL context
			CL_WGL_HDC_KHR,      (cl_context_properties)wglGetCurrentDC()
		});
	}
	props.push_back(0);
	return oclContext(new _oclContext(props.data(), devs, name, vendor));
}



}


}