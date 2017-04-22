#pragma once
#include "oclRely.h"
#include "oclDevice.h"


namespace oclu
{

namespace detail
{


class OCLUAPI _oclKernel
{
private:
	const cl_kernel kernel;
	_oclKernel();
public:
	~_oclKernel();
};


class OCLUAPI _oclProgram
{
	friend class _oclContext;
private:
	const cl_program progID;
	const string src;
	//static void CL_CALLBACK onNotify(const cl_program pid, void *user_data);
	vector<oclDevice> getDevs() const;
	wstring getBuildLog(const oclDevice& dev) const;
	_oclProgram(const cl_program pid, const string& str);
public:
	~_oclProgram();
	void build() const;
};


}
using oclProgram = Wrapper<detail::_oclProgram>;


}

