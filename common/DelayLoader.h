#pragma  once

#include <cstdint>
#include <string>
#include <functional>

namespace common
{

class DelayLoader
{
public:
	using LoadFunc = std::function<void*(const std::wstring& name)>;
	static LoadFunc onLoadDLL;
	static LoadFunc onGetFuncAddr;
};

}