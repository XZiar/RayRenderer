#pragma once
#include "oglRely.h"
#include "3DElement.hpp"

#if COMMON_OS_WIN
#define OGLU_OPTIMUS_ENABLE_NV extern "C" { _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001; }
#else
#define OGLU_OPTIMUS_ENABLE_NV 
#endif

namespace oglu
{
using b3d::Vec3;
using b3d::Vec4;
using b3d::Mat3x3;
using b3d::Mat4x4;


class OGLUAPI oglUtil
{
private:
public:
    static void InJectRenderDoc(const common::fs::path& dllPath);
    static void InitGLEnvironment();
    static void SetFuncLoadingDebug(const bool printSuc, const bool printFail) noexcept;
    static void InitLatestVersion();
    static std::u16string GetVersionStr();
    static std::optional<std::string_view> GetError();
    static common::PromiseResult<void> SyncGL();
    static common::PromiseResult<void> ForceSyncGL();
};

}