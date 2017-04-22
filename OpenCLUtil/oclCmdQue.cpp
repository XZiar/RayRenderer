#include "oclRely.h"
#include "oclCmdQue.h"
#include "oclDevice.h"
#include "oclContext.h"

namespace oclu
{


namespace detail
{



_oclCmdQue::_oclCmdQue(const cl_command_queue que) : cmdque(que)
{
}


_oclCmdQue::~_oclCmdQue()
{
	flush();
	clReleaseCommandQueue(cmdque);
}


void _oclCmdQue::flush() const
{
	clFlush(cmdque);
	clFinish(cmdque);
}



}


}