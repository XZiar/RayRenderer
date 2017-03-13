#pragma once

#include "oclRely.h"

namespace oclu
{


namespace inner
{


OCLUAPI class _oclDevice : public NonCopyable
{
private:
	friend class _oclPlatfrom;
	friend class _oclProgram;
	friend class oclUtil;
	cl_device_id dID;
	_oclDevice(const _oclPlatfrom& _plat, const cl_device_id _dID);
public:
	string name, vendor, profile;
};


OCLUAPI class _oclPlatform : public NonCopyable
{
private:
	cl_platform_id pID;
	cl_device_id defDevID;
	cl_context context;
	vector<Wrapper<_oclDevice,false>> devs;
	Wrapper<_oclDevice, false> defDev;
};

}


OCLUAPI class oclUtil
{

};

}
