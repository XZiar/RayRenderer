#include "oglRely.h"
#include "oglInternal.h"
#include "../common/TimeUtil.inl"

#pragma comment (lib, "opengl32.lib")// link Microsoft OpenGL lib
#pragma comment (lib, "glu32.lib")// link OpenGL Utility lib


namespace oglu
{

using namespace common::mlog;
logger& oglLog()
{
	static logger ogllog(L"OpenGLUtil", nullptr, nullptr, LogOutput::Console, LogLevel::Debug);
	return ogllog;
}


}