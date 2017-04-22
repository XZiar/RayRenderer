#pragma once

#include "oclRely.h"

namespace oclu
{



enum class DeviceType : uint8_t { Default, CPU, GPU, Accelerator, Custom };

namespace detail
{


class OCLUAPI _oclDevice : public NonCopyable
{
	friend class _oclContext;
	friend class _oclPlatform;
	friend class _oclProgram;
	friend class _oclCmdQue;
	friend class ::oclu::oclUtil;
private:
	const cl_device_id deviceID;
	wstring getStr(const cl_device_info type) const;
	DeviceType getDevType() const;
	_oclDevice(const cl_device_id dID);
public:
	const DeviceType type;
	const wstring name, vendor, version;
};


}
using oclDevice = Wrapper<detail::_oclDevice>;


}

