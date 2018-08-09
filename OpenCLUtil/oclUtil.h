#pragma once

#include "oclRely.h"
#include "oclPlatform.h"
#include "oclContext.h"

namespace oclu
{


class OCLUAPI oclUtil
{
private:
    static vector<oclPlatform> platforms;
public:
    static void init();
    static const vector<oclPlatform>& getPlatforms() { return platforms; }
    static oclContext CreateGLSharedContext(const oglu::oglContext& ctx);
    static u16string_view getErrorString(const cl_int err);
};
inline u16string errString(const u16string_view& prefix, cl_int errcode)
{
    return u16string(prefix).append(u" --ERROR: ").append(oclUtil::getErrorString(errcode));

}

}
