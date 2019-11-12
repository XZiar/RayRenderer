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
enum class TransformType : uint8_t { RotateXYZ, Rotate, Translate, Scale };
struct OGLUAPI TransformOP
{
    Vec4 vec;
    TransformType type;
    TransformOP(const Vec4& vec_, const TransformType type_) :vec(vec_), type(type_) { }
};


class OGLUAPI oglUtil
{
private:
public:
    static void SetFuncLoadingDebug(const bool printSuc, const bool printFail) noexcept;
    static void InitLatestVersion();
    static std::u16string GetVersionStr();
    static std::optional<std::string_view> GetError();
    static void applyTransform(Mat4x4& matModel, const TransformOP& op);
    static void applyTransform(Mat4x4& matModel, Mat3x3& matNormal, const TransformOP& op);
    static common::PromiseResult<void> SyncGL();
    static common::PromiseResult<void> ForceSyncGL();
};

}