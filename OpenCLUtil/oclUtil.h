#pragma once

#include "oclRely.h"
#include "oclPlatform.h"
#include "oclContext.h"

namespace oclu
{


class OCLUAPI oclUtil
{
public:
    static void Init(const bool checkGL = true);
    static const vector<oclPlatform>& GetPlatforms();
    static oclContext CreateGLSharedContext(const oglu::oglContext& ctx);
    static u16string_view getErrorString(const cl_int err);
};
inline u16string errString(const u16string_view& prefix, cl_int errcode)
{
    return u16string(prefix).append(u" --ERROR: ").append(oclUtil::getErrorString(errcode));

}

}
