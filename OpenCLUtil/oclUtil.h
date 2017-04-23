#pragma once

#include "oclRely.h"
#include "oclPlatform.h"

namespace oclu
{


class OCLUAPI oclUtil
{
private:
	static vector<oclPlatform> platforms;
public:
	static void __cdecl init();
	static const vector<oclPlatform>& __cdecl getPlatforms() { return platforms; }
	static const wchar_t* __cdecl getErrorString(const cl_int err);
};

}
