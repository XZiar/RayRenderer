#include "oclRely.h"

#pragma comment (lib, "opengl32.lib") /* link Microsoft OpenGL lib   */


namespace oclu
{

using namespace common::mlog;
MiniLogger<false>& oclLog()
{
    static MiniLogger<false> ocllog(u"OpenCLUtil", { GetConsoleBackend() });
    return ocllog;
}


}