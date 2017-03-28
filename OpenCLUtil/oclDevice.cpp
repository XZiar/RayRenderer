#include "oclRely.h"
#include "oclDevice.h"


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





}


}