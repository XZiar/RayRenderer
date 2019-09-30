#pragma once

#include "oclRely.h"
#include "oclPlatform.h"
#include "oclContext.h"

namespace oclu
{


class OCLUAPI oclUtil
{
    friend class GLInterop;
private:
    static common::mlog::MiniLogger<false>& GetOCLLog();
public:
    static void LogCLInfo();
    static const vector<oclPlatform>& GetPlatforms();
    static u16string_view GetErrorString(const cl_int err);
};

}
