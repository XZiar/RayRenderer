#include "RenderCoreRely.h"
#include "common/ResourceHelper.h"
#include "common/ThreadEx.inl"

namespace rayr
{
using common::ResourceHelper;

using namespace common::mlog;
logger& basLog()
{
	static logger baslog(L"BasicTest", nullptr, nullptr, LogOutput::Console, LogLevel::Debug);
	return baslog;
}

string getShaderFromDLL(int32_t id)
{
	auto data = ResourceHelper::getData(L"SHADER", id);
	data.push_back('\0');
	return string((const char*)data.data());
}

}