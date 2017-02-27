#pragma once

#include <cstdint>
#include <vector>

namespace common
{

class ResourceHelper
{
public:
	enum class Result :int8_t { Success, FindFail = -1, ZeroSize = -2, LoadFail = -3, LockFail = -4 };
private:
	static void* thisdll;
public:
	static void init(void* dll);
	static Result getData(std::vector<uint8_t>& data, const wchar_t *type, const int32_t id);
};

}