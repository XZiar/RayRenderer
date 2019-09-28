#pragma once
#include "oglRely.h"
#include "AsyncExecutor/AsyncAgent.h"
#include "3DElement.hpp"

#define OGLU_OPTIMUS_ENABLE_NV extern "C" { _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001; }

namespace oglu
{
using b3d::Vec3;
using b3d::Vec4;
using b3d::Mat3x3;
using b3d::Mat4x4;
enum class TransformType : uint8_t { RotateXYZ, Rotate, Translate, Scale };
struct OGLUAPI TransformOP : public common::AlignBase<alignof(Vec4)>
{
    Vec4 vec;
    TransformType type;
    TransformOP(const Vec4& vec_, const TransformType type_) :vec(vec_), type(type_) { }
};


enum class GLMemBarrier : GLenum 
{
    VertAttrib = 0x00000001     /*GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT*/,
    EBO        = 0x00000002     /*GL_ELEMENT_ARRAY_BARRIER_BIT*/,
    Unifrom    = 0x00000004     /*GL_UNIFORM_BARRIER_BIT*/,
    TexFetch   = 0x00000008     /*GL_TEXTURE_FETCH_BARRIER_BIT*/,
    Image      = 0x00000020     /*GL_SHADER_IMAGE_ACCESS_BARRIER_BIT*/,
    Command    = 0x00000040     /*GL_COMMAND_BARRIER_BIT*/,
    PBO        = 0x00000080     /*GL_PIXEL_BUFFER_BARRIER_BIT*/,
    TexUpdate  = 0x00000100     /*GL_TEXTURE_UPDATE_BARRIER_BIT*/,
    Buffer     = 0x00000200     /*GL_BUFFER_UPDATE_BARRIER_BIT*/,
    FBO        = 0x00000400     /*GL_FRAMEBUFFER_BARRIER_BIT*/,
    TransFB    = 0x00000800     /*GL_TRANSFORM_FEEDBACK_BARRIER_BIT*/,
    Atomic     = 0x00001000     /*GL_ATOMIC_COUNTER_BARRIER_BIT*/,
    SSBO       = 0x00002000     /*GL_SHADER_STORAGE_BARRIER_BIT*/,
    BufMap     = 0x00004000     /*GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT*/,
    Query      = 0x00008000     /*GL_QUERY_BUFFER_BARRIER_BIT*/,
    All        = 0xFFFFFFFF     /*GL_ALL_BARRIER_BITS*/,
};
MAKE_ENUM_BITFIELD(GLMemBarrier)

class OGLUAPI oglUtil
{
private:
public:
    //static void Init(const bool initLatestVer = false);
    static void InitLatestVersion();
    static u16string GetVersionStr();
    static optional<string_view> GetError();
    static set<string_view, std::less<>> GetExtensions();
    static void applyTransform(Mat4x4& matModel, const TransformOP& op);
    static void applyTransform(Mat4x4& matModel, Mat3x3& matNormal, const TransformOP& op);
    static PromiseResult<void> SyncGL();
    static PromiseResult<void> ForceSyncGL();
    static void MemBarrier(const GLMemBarrier mbar);
};

}