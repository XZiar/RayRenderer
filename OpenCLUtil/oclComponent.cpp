#include "oclComponent.h"

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

namespace oclu
{


namespace inner
{

string _oclDevice::getStr(const cl_device_info type) const
{
	char str[128] = { 0 };
	clGetDeviceInfo(deviceID, type, 127, str, NULL);
	return string(str);
}

DeviceType _oclDevice::getDevType() const
{
	cl_device_type dtype;
	clGetDeviceInfo(deviceID, CL_DEVICE_TYPE, sizeof(dtype), &dtype, NULL);
	switch (dtype)
	{
	case CL_DEVICE_TYPE_CPU:
		return DeviceType::CPU;
	case CL_DEVICE_TYPE_GPU:
		return DeviceType::GPU;
	case CL_DEVICE_TYPE_ACCELERATOR:
		return DeviceType::Accelerator;
	case CL_DEVICE_TYPE_CUSTOM:
		return DeviceType::Custom;
	default:
		return DeviceType::Default;
	}
}

_oclDevice::_oclDevice(const cl_device_id dID)
	: deviceID(dID), name(getStr(CL_DEVICE_NAME)), vendor(getStr(CL_DEVICE_VENDOR)), version(getStr(CL_DEVICE_VERSION)), type(getDevType())
{
}


void CL_CALLBACK _oclContext::onNotify(const char * errinfo, const void * private_info, size_t cb, void * user_data)
{
	const _oclContext& ctx = *(_oclContext*)user_data;
	if(ctx.onMessage == nullptr)
		return;
	ctx.onMessage();
}

cl_context _oclContext::createContext(const cl_context_properties props[], const vector<Wrapper<_oclDevice, false>>& devices) const
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

_oclContext::_oclContext(const cl_context_properties props[], const vector<Wrapper<_oclDevice, false>>& devices) 
	: context(createContext(props, devices))
{

}

_oclContext::~_oclContext()
{
	clReleaseContext(context);
}




bool _oclPlatform::checkGL() const
{
	if (name.find("Experimental") != string::npos)
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

string _oclPlatform::getStr(const cl_platform_info type) const
{
	char str[128] = { 0 };
	clGetPlatformInfo(platformID, type, 127, str, NULL);
	return string(str);
}

_oclPlatform::_oclPlatform(const cl_platform_id pID) 
	: platformID(pID), name(getStr(CL_PLATFORM_NAME)), ver(getStr(CL_PLATFORM_VERSION)), isCurrentGL(checkGL())
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
		devs.push_back(oclDevice(new inner::_oclDevice(dID)));
		if (dID == defDevID)
			defDev = devs.back();
	}
}

oclContext _oclPlatform::createContext() const
{
	vector<cl_context_properties> props;
	if (isCurrentGL)
	{
		props.assign(
		{
			//OpenCL platform
			CL_CONTEXT_PLATFORM, (cl_context_properties)platformID,
			//OpenGL context
			CL_GL_CONTEXT_KHR,   (cl_context_properties)wglGetCurrentContext(),
			//HDC used to create the OpenGL context
			CL_WGL_HDC_KHR,      (cl_context_properties)wglGetCurrentDC()
		});
	}
	else
	{
		props.assign({ CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platformID) });
	}
	props.push_back(0);
	return oclContext(new _oclContext(props.data(), devs));
}







}


}