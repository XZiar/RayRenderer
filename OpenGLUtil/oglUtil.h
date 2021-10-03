#pragma once
#include "oglRely.h"

#if COMMON_OS_WIN
#define OGLU_OPTIMUS_ENABLE_NV extern "C" { _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001; }
#else
#define OGLU_OPTIMUS_ENABLE_NV 
#endif

namespace oglu
{


class oglUtil
{
private:
public:
    OGLUAPI static void InJectRenderDoc(const common::fs::path& dllPath);
    OGLUAPI static void InitGLEnvironment();
    OGLUAPI static void SetFuncLoadingDebug(const bool printSuc, const bool printFail) noexcept;
    OGLUAPI static void InitLatestVersion();
    OGLUAPI static std::optional<std::string_view> GetError();
    OGLUAPI static common::PromiseResult<void> SyncGL();
    OGLUAPI static common::PromiseResult<void> ForceSyncGL();
};


}