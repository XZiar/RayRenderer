#pragma once

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <vector>

namespace common
{

class ResourceHelper
{
public:
	enum class Result :int { Success, FindFail = -1, ZeroSize = -2, LoadFail = -3, LockFail = -4 };
private:
	static HMODULE thisdll;
public:
	static void init(HINSTANCE dll);
	static Result getData(std::vector<uint8_t>& data, const wchar_t *type, const int32_t id);
};

}