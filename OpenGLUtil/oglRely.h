#pragma once

#ifdef OGLU_EXPORT
#   define OGLUAPI _declspec(dllexport)
#   define OGLUTPL _declspec(dllexport) 
#   define COMMON_EXPORT
#else
#   define OGLUAPI _declspec(dllimport)
#   define OGLUTPL
#endif

#include "../3dBasic/3dElement.hpp"
#include "../3DBasic/CommonBase.hpp"

#include <cstdio>
#include <string>
#include <cstring>

namespace oclu
{
class _oclMem;
}
