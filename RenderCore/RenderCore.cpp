#include "resource.h"
#include "RenderCore.h"
#include "ResourceHelper.h"

namespace rayr
{

static string getShaderFromDLL(int32_t id)
{
	std::vector<uint8_t> data;
	if (ResourceHelper::getData(data, L"SHADER", IDR_SHADER_2DVERT) != ResourceHelper::Result::Success)
		return "";
	data.push_back('\0');
	return std::string((const char*)data.data());
}

SimpleTest::SimpleTest()
{

}

string SimpleTest::get2dvert()
{
	return getShaderFromDLL(IDR_SHADER_2DVERT);
}

}