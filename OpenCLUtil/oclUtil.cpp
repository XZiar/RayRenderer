#include "oclUtil.h"
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

namespace oclu
{


namespace inner
{





}



vector<oclPlatform> oclUtil::platforms;

void oclUtil::init()
{
	platforms.clear();
	cl_uint numPlatforms = 0;
	clGetPlatformIDs(0, NULL, &numPlatforms);
	//Get all Platforms
	vector<cl_platform_id> platformIDs(numPlatforms);
	clGetPlatformIDs(numPlatforms, platformIDs.data(), NULL);
	for (const auto& pID : platformIDs)
	{
		platforms.push_back(oclPlatform(new inner::_oclPlatform(pID)));
	}
}



}