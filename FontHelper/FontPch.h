#pragma once
#include "FontRely.h"
#include "FreeType.h"
#include "SystemCommon/MiniLogger.h"
#include <cmath>
#include "resource.h"

namespace oglu
{
common::mlog::MiniLogger<false>& fntLog();
std::string LoadShaderFromDLL(int32_t id);
}