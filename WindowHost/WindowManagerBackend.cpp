#include "common/CommonRely.hpp"

#if COMMON_OS_WIN
#   include "WindowManagerWin32.inl"
#elif COMMON_OS_DARWIN
#   include "WindowManagerCocoa.inl"
#elif COMMON_OS_LINUX
#   include "WindowManagerXCB.inl"
#endif

