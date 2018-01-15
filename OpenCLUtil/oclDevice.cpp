#include "oclRely.h"
#include "oclDevice.h"


namespace oclu
{


namespace detail
{

static wstring getStr(const cl_device_id deviceID, const cl_device_info type)
{
	char str[128] = { 0 };
	clGetDeviceInfo(deviceID, type, 127, str, nullptr);
	return str::to_wstring(str);
}

template<typename T>
static T getNum(const cl_device_id deviceID, const cl_device_info type)
{
	static_assert(std::is_integral_v<T>, "T should be numeric type");
	T num = 0;
	clGetDeviceInfo(deviceID, type, sizeof(T), &num, nullptr);
	return num;
}


DeviceType _oclDevice::getDevType() const
{
	cl_device_type dtype;
	clGetDeviceInfo(deviceID, CL_DEVICE_TYPE, sizeof(dtype), &dtype, nullptr);
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
	: deviceID(dID), name(getStr(dID, CL_DEVICE_NAME)), vendor(getStr(dID, CL_DEVICE_VENDOR)), version(getStr(dID, CL_DEVICE_VERSION)), type(getDevType()),
	GlobalMemSize(getNum<uint64_t>(dID, CL_DEVICE_GLOBAL_MEM_SIZE)), LocalMemSize(getNum<uint64_t>(dID, CL_DEVICE_LOCAL_MEM_SIZE)),
	ConstantBufSize(getNum<uint64_t>(dID, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE)), MaxMemSize(getNum<uint64_t>(dID, CL_DEVICE_MAX_MEM_ALLOC_SIZE)),
	GlobalCacheSize(getNum<uint64_t>(dID, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE)), GlobalCacheLine(getNum<uint32_t>(dID, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE))
{
}





}


}