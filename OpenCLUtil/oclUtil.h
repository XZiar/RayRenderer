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
	static void init();
	static const vector<oclPlatform>& getPlatforms() { return platforms; }
	static u16string_view getErrorString(const cl_int err);
};
wstring errString(const wchar_t *prefix, cl_int errcode);

}
