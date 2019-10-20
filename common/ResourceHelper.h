#pragma once

#include "CommonRely.hpp"
#include <cstdint>

namespace common
{

class ResourceHelper
{
public:
	static void Init(void* dll);
	static common::span<const std::byte> GetData(const wchar_t *type, const int32_t id);
};

}