#pragma once
#include "oclRely.h"


namespace oclu
{

namespace inner
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
private:
	const cl_program progID;
public:
	_oclProgram();
	~_oclProgram();
};


}
using oclProgram = Wrapper<inner::_oclProgram>;


}

