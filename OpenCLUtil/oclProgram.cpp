#include "oclRely.h"
#include "oclProgram.h"
#include "oclException.h"
#include "oclUtil.h"


namespace oclu
{


namespace detail
{


cl_kernel _oclKernel::createKernel() const
{
	cl_int errorcode;
	auto kid = clCreateKernel(prog->progID, name.c_str(), &errorcode);
	if(errorcode!=CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"canot create kernel from program", errorcode));
	return kid;
}

_oclKernel::_oclKernel(const std::shared_ptr<_oclProgram>& prog_, const string& name_) : name(name_), prog(prog_), kernel(createKernel())
{
}

_oclKernel::~_oclKernel()
{
	clReleaseKernel(kernel);
}

optional<wstring> _oclKernel::setArg(const cl_uint idx, const oclBuffer& buf)
{
	auto ret = clSetKernelArg(kernel, idx, sizeof(cl_mem), &buf->memID);
	if (ret == CL_SUCCESS)
		return {};
	return oclUtil::getErrorString(ret);
}

optional<wstring> _oclKernel::setArg(const cl_uint idx, const void *dat, const size_t size)
{
	auto ret = clSetKernelArg(kernel, idx, size, dat);
	if (ret == CL_SUCCESS)
		return {};
	return oclUtil::getErrorString(ret);
}



_oclProgram::_oclProgram(std::shared_ptr<_oclContext>& ctx_, const string& str) : ctx(ctx_), src(str), progID(loadProgram())
{
}

_oclProgram::~_oclProgram()
{
	clReleaseProgram(progID);
}

vector<oclDevice> _oclProgram::getDevs() const
{
	cl_int ret;
	cl_uint devCount = 0;
	ret = clGetProgramInfo(progID, CL_PROGRAM_NUM_DEVICES, sizeof(devCount), &devCount, nullptr);
	vector<cl_device_id> devids(devCount);
	ret = clGetProgramInfo(progID, CL_PROGRAM_DEVICES, sizeof(cl_device_id)*devCount, devids.data(), nullptr);
	if (ret != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"ANY ERROR in get devices from program", ret));
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

cl_program _oclProgram::loadProgram() const
{
	cl_int errcode;
	auto *str = src.c_str();
	size_t size = src.length();
	const auto prog = clCreateProgramWithSource(ctx->context, 1, &str, &size, &errcode);
	if (errcode != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot create program", errcode));
	return prog;
}

string _oclProgram::loadFromFile(FILE *fp)
{
	fseek(fp, 0, SEEK_END);
	const size_t fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *dat = new char[fsize + 1];
	fread(dat, fsize, 1, fp);
	dat[fsize] = '\0';
	string txt(dat);
	delete[] dat;

	return txt;
}

void _oclProgram::initKers()
{
	cl_int ret;
	char buf[8192];
	size_t len = 0;
	ret = clGetProgramInfo(progID, CL_PROGRAM_KERNEL_NAMES, sizeof(buf) - 2, &buf, &len);
	if (ret != CL_SUCCESS)
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot find kernels", ret));
	buf[len] = '\0';
	auto names = str::split(string_view(buf), ';');
	kers.clear();
	kers.assign(names.begin(), names.end());
}

void _oclProgram::build()
{
	auto ret = clBuildProgram(progID, 0, nullptr, nullptr, nullptr, nullptr);
	if (ret != CL_SUCCESS)
	{
		wstring buildlog;
		if (ret == CL_BUILD_PROGRAM_FAILURE)
		{
			for (auto dev : getDevs())
				buildlog += dev->name + L":\n" + getBuildLog(dev) + L"\n";
		}
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"Build Program failed", ret), buildlog);
	}
	initKers();
}

void _oclProgram::build(const oclDevice& dev)
{
	auto ret = clBuildProgram(progID, 1, &dev->deviceID, nullptr, nullptr, nullptr);
	if (ret != CL_SUCCESS)
	{
		wstring buildlog;
		if (ret == CL_BUILD_PROGRAM_FAILURE)
		{
			buildlog = dev->name + L":\n" + getBuildLog(dev) + L"\n";
		}
		COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"Build Program failed", ret), buildlog);
	}
	initKers();
}

oclu::oclKernel _oclProgram::getKernel(const string& name)
{
	return oclKernel(new _oclKernel(shared_from_this(), name));
}



}


}
