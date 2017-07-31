#include "FontRely.h"
#include "common/ResourceHelper.h"

namespace oglu
{

using namespace common::mlog;
logger& fntLog()
{
	static logger fntlog(L"FontHelper", nullptr, nullptr, LogOutput::Console, LogLevel::Debug);
	return fntlog;
}

string getShaderFromDLL(int32_t id)
{
	auto data = ResourceHelper::getData(L"SHADER", id);
	data.push_back('\0');
	return string((const char*)data.data());
}

}