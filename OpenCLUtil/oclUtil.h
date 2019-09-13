#pragma once

#include "oclRely.h"
#include "oclPlatform.h"
#include "oclContext.h"

namespace oclu
{


class OCLUAPI oclUtil
{
public:
    static void LogCLInfo(const bool checkGL = true);
    static const vector<oclPlatform>& GetPlatforms();
    static oclContext CreateGLSharedContext(const oglu::oglContext& ctx);
    static u16string_view GetErrorString(const cl_int err);
};

}
