#include "oclRely.h"
#include "oclInternal.h"
#include "../common/PromiseTask.inl"

#ifdef USING_INTEL
#   pragma comment(lib, R"(\intel\OpenCL.lib)")
#elif defined(USING_NVIDIA)
#   pragma comment(lib, R"(\nvidia\opencl.lib)")
#endif
#pragma comment (lib, "opengl32.lib") /* link Microsoft OpenGL lib   */


namespace oclu
{

using namespace common::mlog;
logger& oglLog()
{
	static logger ocllog(L"OpenGLUtil", nullptr, nullptr, LogOutput::Console, LogLevel::Debug);
	return ocllog;
}


}