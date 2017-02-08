#pragma once

#ifdef RAYCORE_EXPORT
#   define RAYCOREAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define RAYCOREAPI _declspec(dllimport)
#endif



#include <cstdint>
#include <cstdio>
#include <string>
#include "../3DBasic/3dElement.hpp"
#include "../OpenGLUtil/oglUtil.h"


namespace rayr
{
using namespace common;
using std::string;

class RAYCOREAPI SimpleTest
{
private:
public:
	SimpleTest();
	string get2dvert();
};

}