#include "oclRely.h"
#include "oclProgram.h"
#include "oclException.h"
#include "oclUtil.h"


namespace oclu
{


namespace detail
{


vector<oclDevice> _oclProgram::getDevs() const
{
	cl_int ret;
	cl_uint devCount = 0;
	ret = clGetProgramInfo(progID, CL_PROGRAM_NUM_DEVICES, sizeof(devCount), &devCount, nullptr);
	vector<cl_device_id> devids(devCount);
	ret = clGetProgramInfo(progID, CL_PROGRAM_DEVICES, sizeof(cl_device_id)*devCount, devids.data(), nullptr);
	if (ret != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, L"ANY ERROR in get devices from program", oclUtil::getErrorString(ret));
	vector<oclDevice> devs;
	while (devCount--)
		devs.push_back(oclDevice(new _oclDevice(devids[devCount])));
	return devs;
}

wstring _oclProgram::getBuildLog(const oclDevice & dev) const
{
	cl_build_status status;
	clGetProgramBuildInfo(progID, dev->deviceID, CL_PROGRAM_BUILD_STATUS, sizeof(status), &status, nullptr);
	switch (status)
	{
	case CL_BUILD_NONE:
		return L"Not been built yet";
	case CL_BUILD_SUCCESS:
		return L"Build successfully";
	case CL_BUILD_ERROR:
		{
			char logstr[8192] = { 0 };
			clGetProgramBuildInfo(progID, dev->deviceID, CL_PROGRAM_BUILD_LOG, sizeof(logstr), logstr, nullptr);
			return to_wstring(logstr);
		}
	default:
		return L"";
	}
}

_oclProgram::_oclProgram(const cl_program pid, const string& str) : progID(pid), src(str)
{
}


_oclProgram::~_oclProgram()
{
	clReleaseProgram(progID);
}


void _oclProgram::build() const
{
	auto ret = clBuildProgram(progID, 0, nullptr, nullptr, nullptr, nullptr);
	if (ret != CL_SUCCESS)
	{
		wstring buildlog;
		if (ret == CL_BUILD_PROGRAM_FAILURE)
		{
			for (auto dev : getDevs())
				buildlog = str::concat<wchar_t>(buildlog, dev->name, L":\n", getBuildLog(dev), L"\n");
		}
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, L"Build Program ERROR : " + to_wstring(oclUtil::getErrorString(ret)), buildlog);
	}
}






}


}
