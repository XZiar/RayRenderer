#pragma once

#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"


namespace oclu
{

namespace detail
{


class OCLUAPI _oclComQue : public NonCopyable
{
private:
	friend class _oclContext;
	static cl_command_queue createQueue(const cl_context context, const cl_device_id dev);
	const Wrapper<_oclContext> context;
	const Wrapper<_oclDevice> device;
	const cl_command_queue comque;
	_oclComQue(const Wrapper<_oclContext>& ctx, const Wrapper<_oclDevice>& dev);
public:
	~_oclComQue();
	void flush() const;
	Wrapper<_oclProgram> loadProgram() const;
};


}
using oclComQue = Wrapper<detail::_oclComQue>;

}