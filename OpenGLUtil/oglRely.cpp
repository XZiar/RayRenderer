#include "oglRely.h"

#pragma comment (lib, "opengl32.lib")// link Microsoft OpenGL lib
#pragma comment (lib, "glu32.lib")// link OpenGL Utility lib

#pragma message("Compile OpenGLUtil with boost[" STRINGIZE(BOOST_LIB_VERSION) "]")


namespace oglu
{

using namespace common::mlog;
logger& oglLog()
{
	static logger ogllog(L"OpenGLUtil", nullptr, nullptr, LogOutput::Console, LogLevel::Debug);
	return ogllog;
}


}