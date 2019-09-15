#pragma once

#include "common/FileEx.hpp"
#include "MiniLogger/MiniLogger.h"
#include "common/TimeUtil.hpp"
#include "common/ResourceHelper.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <cinttypes>
#include <iostream>
#include "resource.h"


common::fs::path FindPath();
uint32_t RegistTest(const char *name, void(*func)());
std::string LoadShaderFallback(const std::u16string& filename, int32_t id);

inline void ClearReturn()
{
    std::cin.ignore(1024, '\n');
}