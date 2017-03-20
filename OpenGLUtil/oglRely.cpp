#include "oglRely.h"
#include "oglInternal.h"
#include "../common/BasicUtil.hpp"

#pragma comment (lib, "opengl32.lib") /* link Microsoft OpenGL lib   */
//#pragma comment (lib, "gdi32.lib")    /* link Windows GDI lib        */
//#pragma comment (lib, "user32.lib")   /* link Windows user lib       */

#ifndef _DEBUG
#    pragma comment(lib, "glew32.lib")
#else
#    pragma comment(lib, "glew32d.lib")
#endif


namespace oglu
{

using namespace common::mlog;
logger& oglLog()
{
	static logger ogllog(L"OpenGLUtil", nullptr, nullptr, LogOutput::Console, LogLevel::Debug);
	return ogllog;
}


}