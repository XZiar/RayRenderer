#pragma once
#include "oglRely.h"
#include "GLFuncWrapper.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/ThreadEx.h"
#include "SystemCommon/StackTrace.h"
#include "SystemCommon/CopyEx.h"
#include "SystemCommon/MiniLogger.h"
#include "ImageUtil/ImageUtil.h"
#include "ImageUtil/TexFormat.h"
#include "common/Linq2.hpp"
#include "common/StringEx.hpp"
#include "common/StringLinq.hpp"
#include "common/ContainerEx.hpp"
#include "common/math/VecSIMD.hpp"
#include "common/math/MatSIMD.hpp"

#include <algorithm>


namespace oglu
{
namespace msimd = common::math::simd;
common::mlog::MiniLogger<false>& oglLog();
}
