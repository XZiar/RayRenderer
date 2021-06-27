#pragma once
#include "FontRely.h"
#include "OpenCLUtil/oclException.h"
#include "MiniLogger/MiniLogger.h"
#include "3rdParty/Projects/FreeType/FreeType.h"
#include <cmath>
#include "resource.h"

namespace oglu
{
common::mlog::MiniLogger<false>& fntLog();
std::string LoadShaderFromDLL(int32_t id);
}