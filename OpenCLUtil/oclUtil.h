#pragma once

#include "oclRely.h"
#include "oclPlatform.h"

namespace oclu
{

class oclUtil;
namespace detail
{



}


class OCLUAPI oclUtil
{
private:
	static vector<oclPlatform> platforms;
public:
	static void __cdecl init();
	static const vector<oclPlatform>& __cdecl getPlatforms() { return platforms; }
};

}
