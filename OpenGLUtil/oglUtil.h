#pragma once
#include "oglRely.h"
#include "common/AsyncExecutor/AsyncAgent.h"

#define OGLU_OPTIMUS_ENABLE_NV extern "C" { _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001; }

namespace oglu
{

using common::asyexe::AsyncTaskFunc;

enum class GLMemBarrier : GLenum 
{
    VertAttrib = GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT, EBO = GL_ELEMENT_ARRAY_BARRIER_BIT, Unifrom = GL_UNIFORM_BARRIER_BIT, 
    TexFetch = GL_TEXTURE_FETCH_BARRIER_BIT, Image = GL_SHADER_IMAGE_ACCESS_BARRIER_BIT, Command = GL_COMMAND_BARRIER_BIT,
    PBO = GL_PIXEL_BUFFER_BARRIER_BIT, TexUpdate = GL_TEXTURE_UPDATE_BARRIER_BIT, Buffer = GL_BUFFER_UPDATE_BARRIER_BIT, 
    FBO = GL_FRAMEBUFFER_BARRIER_BIT, TransFB = GL_TRANSFORM_FEEDBACK_BARRIER_BIT, Query = GL_QUERY_BUFFER_BARRIER_BIT,
    Atomic = GL_ATOMIC_COUNTER_BARRIER_BIT, SSBO = GL_SHADER_STORAGE_BARRIER_BIT, BufMap = GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT,
    All = GL_ALL_BARRIER_BITS
};
MAKE_ENUM_BITFIELD(GLMemBarrier)

class OGLUAPI oglUtil
{
private:
public:
    static void init(const bool initLatestVer = false);
    static u16string getVersion();
    static optional<u16string> getError();
    static void applyTransform(Mat4x4& matModel, const TransformOP& op);
    static void applyTransform(Mat4x4& matModel, Mat3x3& matNormal, const TransformOP& op);
    static PromiseResult<void> SyncGL();
    static PromiseResult<void> ForceSyncGL();
    static void MemBarrier(const GLMemBarrier mbar);
    static void TryTask();
};

}