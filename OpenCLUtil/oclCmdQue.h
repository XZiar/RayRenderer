#pragma once

#include "oclRely.h"
#include "oclDevice.h"


namespace oclu
{

namespace detail
{


class OCLUAPI _oclCmdQue : public NonCopyable
{
private:
	friend class _oclContext;
	const cl_command_queue cmdque;
	_oclCmdQue(const cl_command_queue que);
public:
	~_oclCmdQue();
	void flush() const;
};


}
using oclCmdQue = Wrapper<detail::_oclCmdQue>;

}