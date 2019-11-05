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
    [[nodiscard]] static common::mlog::MiniLogger<false>& GetOCLLog();
public:
    static void LogCLInfo();
    [[nodiscard]] static const std::vector<oclPlatform>& GetPlatforms();
    [[nodiscard]] static std::u16string_view GetErrorString(const cl_int err);
    static void WaitMany(common::PromiseStub promises);
};

}
