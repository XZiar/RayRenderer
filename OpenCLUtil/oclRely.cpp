#include "oclRely.h"

#ifdef USING_INTEL
#   pragma comment(lib, R"(\intel\OpenCL.lib)")
#elif defined(USING_NVIDIA)
#   pragma comment(lib, R"(\nvidia\opencl.lib)")
#endif
#pragma comment (lib, "opengl32.lib") /* link Microsoft OpenGL lib   */

