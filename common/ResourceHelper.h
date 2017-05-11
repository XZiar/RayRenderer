#pragma once

#include <cstdint>
#include <vector>

namespace common
{

class ResourceHelper
{
private:
	static void* thisdll;
public:
	static void init(void* dll);
	static std::vector<uint8_t> getData(const wchar_t *type, const int32_t id);
};

}