#pragma once

#include "oclRely.h"
#include "oclDevice.h"


namespace oclu
{

namespace detail
{


class OCLUAPI _oclCmdQue : public NonCopyable
{
	friend class _oclContext;
	friend class _oclBuffer;
	friend class _oclGLBuffer;
	friend class _oclProgram;
	friend class _oclKernel;
private:
	const std::shared_ptr<const _oclContext> ctx;
	const oclDevice dev;
	const cl_command_queue cmdque;
	cl_command_queue createCmdQue() const;
public:
	_oclCmdQue(const std::shared_ptr<_oclContext>& ctx_, const oclDevice& dev_);
	~_oclCmdQue();
	void flush() const;
};


}
using oclCmdQue = Wrapper<detail::_oclCmdQue>;

}