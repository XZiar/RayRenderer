#pragma once
#include "TexUtilRely.h"
#include "TexUtilWorker.h"
#include "OpenCLInterop/GLInterop.h"
#include "OpenGLUtil/oglException.h"
#include "ImageUtil/ImageUtil.h"
#include "MiniLogger/MiniLogger.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/ThreadEx.h"
#include "common/ContainerEx.hpp"
#include "common/TimeUtil.hpp"
#include "common/Linq2.hpp"


namespace oglu::texutil
{
common::mlog::MiniLogger<false>& texLog();
std::string LoadShaderFromDLL(int32_t id);
}
