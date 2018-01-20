#include "oclRely.h"
#include "common/PromiseTask.inl"


#pragma comment (lib, "opengl32.lib") /* link Microsoft OpenGL lib   */


namespace oclu
{

using namespace common::mlog;
logger& oclLog()
{
	static logger ocllog(L"OpenCLUtil", nullptr, nullptr, LogOutput::Console, LogLevel::Debug);
	return ocllog;
}


}