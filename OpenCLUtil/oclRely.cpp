#include "oclRely.h"

#ifdef USING_INTEL
#    pragma comment(lib, R"(\intel\opencl.lib)")
#    ifdef _WIN64
#    else
#    endif
#elif defined(USING_NVIDIA)
#    pragma comment(lib, R"(\nvidia\opencl.lib)")
#    ifdef _WIN64
#    else
#    endif
#endif

