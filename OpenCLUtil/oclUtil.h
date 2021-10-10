#pragma once

#include "oclRely.h"


namespace oclu
{


class OCLUAPI oclUtil
{
    friend class GLInterop;
private:
    [[nodiscard]] static common::mlog::MiniLogger<false>& GetOCLLog();
public:
    static void LogCLInfo();
    [[nodiscard]] static std::u16string_view GetErrorString(const int32_t err);
    static void WaitMany(common::PromiseStub promises);
};

}
