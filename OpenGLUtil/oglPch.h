#pragma once
#include "GLFuncWrapper.h"
#include "oglRely.h"
#include "StringUtil/Convert.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/ThreadEx.h"
#include "SystemCommon/StackTrace.h"
#include "SystemCommon/CopyEx.h"
#include "MiniLogger/MiniLogger.h"
#include "ImageUtil/ImageUtil.h"
#include "ImageUtil/TexFormat.h"
#include "common/Linq2.hpp"
#include "common/StringEx.hpp"
#include "common/StringLinq.hpp"
#include "common/ContainerEx.hpp"

#include <algorithm>


namespace oglu
{
common::mlog::MiniLogger<false>& oglLog();
}
