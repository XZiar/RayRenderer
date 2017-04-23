#pragma once

#include "oclRely.h"


namespace oclu
{

class OCLUAPI oclPromise : public ::common::detail::PromiseResult_<void>
{
	friend class detail::_oclBuffer;
	friend class detail::_oclKernel;
protected:
	const cl_event eventPoint;
	oclPromise(const cl_event e) : eventPoint(e)
	{ }
public:
	void wait() override
	{
		clWaitForEvents(1, &eventPoint);
	}
	~oclPromise() override
	{
		clReleaseEvent(eventPoint);
	}
	oclPromise(oclPromise&&) = default;
};


}
