#pragma  once

#include <cstdint>
#include <string>
#include <functional>

namespace common
{

class DelayLoader
{
public:
	using LoadFunc = std::function<void*(const char *name)>;
	static LoadFunc onLoadDLL;
	static LoadFunc onGetFuncAddr;
	static bool unload(const std::string& name);
};


}