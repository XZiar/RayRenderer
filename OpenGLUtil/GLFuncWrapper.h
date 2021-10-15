#pragma once
#include "oglRely.h"
#include "common/ContainerEx.hpp"


#if defined(__gl_h_) || defined(__GL_H__) || defined(_GL_H) || defined(__X_GL_H)
#   error should not include gl.h
#endif


#if COMMON_OS_WIN
#   define APIENTRY __stdcall
#   define WINGDIAPI _declspec(dllimport)
#   include <GL/gl.h>
#   undef WINGDIAPI
#   include "GL/glext.h"
#elif COMMON_OS_UNIX
#   define APIENTRY
#   include <GL/gl.h>
#   include "GL/glext.h"
#else
#   error "unknown os"
#endif
#ifndef GLAPIENTRY
#   define GLAPIENTRY APIENTRY
#endif
#define GLAPIENTRYP GLAPIENTRY * 

namespace oglu
{

class oglIndirectBuffer_;
class oglContext_;


class PlatFuncs
{
#if COMMON_OS_WIN
private:
    const char* (GLAPIENTRYP wglGetExtensionsStringARB_) (void* hDC) = nullptr;
    const char* (GLAPIENTRYP wglGetExtensionsStringEXT_) () = nullptr;
    [[nodiscard]] common::container::FrozenDenseSet<std::string_view> GetExtensions(void* hdc) const;
public:
    void* (GLAPIENTRYP wglCreateContextAttribsARB_) (void* hDC, void* hShareContext, const int* attribList) = nullptr;
#else
private:
    void* (*glXGetCurrentDisplay_) () = nullptr;
    common::container::FrozenDenseSet<std::string_view> GetExtensions(void* dpy, int screen) const;
public:
    using GLXFBConfig = void*;
    void* (*glXCreateContextAttribsARB_) (void* dpy, GLXFBConfig config, void* share_context, bool direct, const int* attrib_list);
#endif
public:
    common::container::FrozenDenseSet<std::string_view> Extensions;
    bool SupportFlushControl;
    bool SupportSRGB;

    PlatFuncs(void* target);

    static void InitEnvironment();
    static void InJectRenderDoc(const common::fs::path& dllPath);

    [[nodiscard]] static void* GetCurrentDeviceContext();
    [[nodiscard]] static void* GetCurrentGLContext();
    static void  DeleteGLContext(void* hDC, void* hRC);
    [[nodiscard]] static std::vector<int32_t> GenerateContextAttrib(const uint32_t version, bool needFlushControl);
    [[nodiscard]] static void* CreateNewContext(const GLContextInfo& info, const uint32_t version);
    [[nodiscard]] static void* CreateNewContext(const oglContext_* prevCtx, const bool isShared, const int32_t* attribs);
    static void SwapBuffer(const oglContext_& ctx);
#if COMMON_OS_WIN
    [[nodiscard]] static bool MakeGLContextCurrent(void* hDC, void* hRC);
    [[nodiscard]] static uint32_t GetSystemError();
#else
    [[nodiscard]] static bool MakeGLContextCurrent(void* hDC, unsigned long DRW, void* hRC);
    [[nodiscard]] static unsigned long GetCurrentDrawable();
    [[nodiscard]] static int32_t GetSystemError();
#endif
    [[nodiscard]] static bool MakeGLContextCurrent(const GLContextInfo& info, void* hRC);
};
extern thread_local const PlatFuncs* PlatFunc;



/// xxx_ functions are internal and not recommanded to use directly

class CtxFuncs : public ContextCapability
{
    friend struct VAOBinder;
    friend struct FBOBinder;
private:
    mutable common::SpinLocker DataLock;
public:
    // buffer related
    GLint MaxUBOUnits = 0;
    void      (GLAPIENTRYP ogluGenBuffers) (GLsizei n, GLuint* buffers) = nullptr;
    void      (GLAPIENTRYP ogluDeleteBuffers) (GLsizei n, const GLuint* buffers) = nullptr;
    void      (GLAPIENTRYP ogluBindBuffer) (GLenum target, GLuint buffer) = nullptr;
    void      (GLAPIENTRYP ogluBindBufferBase) (GLenum target, GLuint index, GLuint buffer) = nullptr;
    void      (GLAPIENTRYP ogluBindBufferRange) (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) = nullptr;
    void      (GLAPIENTRYP ogluNamedBufferStorage_) (GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) = nullptr;
    void      (GLAPIENTRYP ogluBufferStorage_) (GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) = nullptr;
    void      (GLAPIENTRYP ogluNamedBufferData_) (GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) = nullptr;
    void      (GLAPIENTRYP ogluBufferData_) (GLenum target, GLsizeiptr size, const void* data, GLenum usage) = nullptr;
    void      (GLAPIENTRYP ogluNamedBufferSubData_) (GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data) = nullptr;
    void      (GLAPIENTRYP ogluBufferSubData_) (GLenum target, GLintptr offset, GLsizeiptr size, const void* data) = nullptr;
    void*     (GLAPIENTRYP ogluMapNamedBuffer_) (GLuint buffer, GLenum access) = nullptr;
    void*     (GLAPIENTRYP ogluMapBuffer_) (GLenum target, GLenum access) = nullptr;
    GLboolean (GLAPIENTRYP ogluUnmapNamedBuffer_) (GLuint buffer) = nullptr;
    GLboolean (GLAPIENTRYP ogluUnmapBuffer_) (GLenum target) = nullptr;

    void ogluNamedBufferStorage(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) const;
    void ogluNamedBufferData(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) const;
    void ogluNamedBufferSubData(GLenum target, GLuint buffer, GLintptr offset, GLsizeiptr size, const void* data) const;
    [[nodiscard]] void* ogluMapNamedBuffer(GLenum target, GLuint buffer, GLenum access) const;
    GLboolean ogluUnmapNamedBuffer(GLenum target, GLuint buffer) const;
    
    // vao related
    mutable GLuint VAO = 0;
    void (GLAPIENTRYP ogluGenVertexArrays) (GLsizei n, GLuint* arrays) = nullptr;
    void (GLAPIENTRYP ogluDeleteVertexArrays) (GLsizei n, const GLuint* arrays) = nullptr;
    void (GLAPIENTRYP ogluBindVertexArray_) (GLuint vaobj) = nullptr;
    void (GLAPIENTRYP ogluEnableVertexAttribArray_) (GLuint index) = nullptr;
    void (GLAPIENTRYP ogluEnableVertexArrayAttrib_) (GLuint vaobj, GLuint index) = nullptr;
    void (GLAPIENTRYP ogluVertexAttribIPointer_) (GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRYP ogluVertexAttribLPointer_) (GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRYP ogluVertexAttribPointer_) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRYP ogluVertexAttribDivisor) (GLuint index, GLuint divisor) = nullptr;

    void RefreshVAOState() const;
    void ogluBindVertexArray(GLuint vaobj) const;
    void ogluEnableVertexArrayAttrib(GLuint vaobj, GLuint index) const;
    void ogluVertexAttribPointer(GLuint index, GLint size, GLenum type, bool normalized, GLsizei stride, size_t offset) const;
    void ogluVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const;
    void ogluVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const;

    // draw related
    void (GLAPIENTRYP ogluDrawArrays) (GLenum mode, GLint first, GLsizei count) = nullptr;
    void (GLAPIENTRYP ogluDrawElements) (GLenum mode, GLsizei count, GLenum type, const void* indices) = nullptr;
    void (GLAPIENTRYP ogluMultiDrawArrays) (GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) = nullptr;
    void (GLAPIENTRYP ogluMultiDrawElements) (GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount) = nullptr;
    void (GLAPIENTRYP ogluMultiDrawArraysIndirect_) (GLenum mode, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;
    void (GLAPIENTRYP ogluDrawArraysInstancedBaseInstance_) (GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) = nullptr;
    void (GLAPIENTRYP ogluDrawArraysInstanced_) (GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
    void (GLAPIENTRYP ogluMultiDrawElementsIndirect_) (GLenum mode, GLenum type, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;
    void (GLAPIENTRYP ogluDrawElementsInstancedBaseVertexBaseInstance_) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) = nullptr;
    void (GLAPIENTRYP ogluDrawElementsInstancedBaseInstance_) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLuint baseinstance) = nullptr;
    void (GLAPIENTRYP ogluDrawElementsInstanced_) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) = nullptr;

    void ogluDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, uint32_t instancecount, uint32_t baseinstance) const;
    void ogluDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, uint32_t instancecount, uint32_t baseinstance) const;
    void ogluMultiDrawArraysIndirect(GLenum mode, const oglIndirectBuffer_& indirect, GLint offset, GLsizei drawcount) const;
    void ogluMultiDrawElementsIndirect(GLenum mode, GLenum type, const oglIndirectBuffer_& indirect, GLint offset, GLsizei drawcount) const;

    // texture related
    GLint MaxTextureUnits = 0;
    GLint MaxImageUnits = 0;
    void     (GLAPIENTRYP ogluGenTextures) (GLsizei n, GLuint* textures) = nullptr;
    void     (GLAPIENTRYP ogluCreateTextures_) (GLenum target, GLsizei n, GLuint* textures) = nullptr;
    void     (GLAPIENTRYP ogluDeleteTextures) (GLsizei n, const GLuint* textures) = nullptr;
    void     (GLAPIENTRYP ogluActiveTexture) (GLenum texture) = nullptr;
    void     (GLAPIENTRYP ogluBindTexture) (GLenum target, GLuint texture) = nullptr;
    void     (GLAPIENTRYP ogluBindTextureUnit_) (GLuint unit, GLuint texture) = nullptr;
    void     (GLAPIENTRYP ogluBindMultiTextureEXT_) (GLuint unit, GLenum target, GLuint texture) = nullptr;
    void     (GLAPIENTRYP ogluBindImageTexture) (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) = nullptr;
    void     (GLAPIENTRYP ogluTextureView) (GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) = nullptr;
    void     (GLAPIENTRYP ogluTextureBuffer_) (GLuint texture, GLenum internalformat, GLuint buffer) = nullptr;
    void     (GLAPIENTRYP ogluTextureBufferEXT_) (GLuint texture, GLenum target, GLenum internalformat, GLuint buffer) = nullptr;
    void     (GLAPIENTRYP ogluTexBuffer_) (GLenum target, GLenum internalformat, GLuint buffer) = nullptr;
    void     (GLAPIENTRYP ogluGenerateTextureMipmap_) (GLuint texture) = nullptr;
    void     (GLAPIENTRYP ogluGenerateTextureMipmapEXT_) (GLuint texture, GLenum target) = nullptr;
    void     (GLAPIENTRYP ogluGenerateMipmap_) (GLenum target) = nullptr;
    GLuint64 (GLAPIENTRYP ogluGetTextureHandle) (GLuint texture) = nullptr;
    void     (GLAPIENTRYP ogluMakeTextureHandleResident) (GLuint64 handle) = nullptr;
    void     (GLAPIENTRYP ogluMakeTextureHandleNonResident) (GLuint64 handle) = nullptr;
    GLuint64 (GLAPIENTRYP ogluGetImageHandle) (GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format) = nullptr;
    void     (GLAPIENTRYP ogluMakeImageHandleResident) (GLuint64 handle, GLenum access) = nullptr;
    void     (GLAPIENTRYP ogluMakeImageHandleNonResident) (GLuint64 handle) = nullptr;
    void     (GLAPIENTRYP ogluTextureParameteri_) (GLuint texture, GLenum pname, GLint param) = nullptr;
    void     (GLAPIENTRYP ogluTextureParameteriEXT_) (GLuint texture, GLenum target, GLenum pname, GLint param) = nullptr;
    void     (GLAPIENTRYP ogluTextureSubImage1D_) (GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP ogluTextureSubImage2D_) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP ogluTextureSubImage3D_) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP ogluTextureSubImage1DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP ogluTextureSubImage2DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP ogluTextureSubImage3DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP ogluTexSubImage3D_) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP ogluTextureImage1DEXT_) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP ogluTextureImage2DEXT_) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP ogluTextureImage3DEXT_) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP ogluTexImage3D_) (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTextureSubImage1D_) (GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTextureSubImage2D_) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTextureSubImage3D_) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTextureSubImage1DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTextureSubImage2DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTextureSubImage3DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTexSubImage1D_) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTexSubImage2D_) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTexSubImage3D_) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTextureImage1DEXT_) (GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTextureImage2DEXT_) (GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTextureImage3DEXT_) (GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTexImage1D_) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTexImage2D_) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCompressedTexImage3D_) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluCopyImageSubData) (GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) = nullptr;
    void     (GLAPIENTRYP ogluTextureStorage1D_) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) = nullptr;
    void     (GLAPIENTRYP ogluTextureStorage2D_) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void     (GLAPIENTRYP ogluTextureStorage3D_) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = nullptr;
    void     (GLAPIENTRYP ogluTextureStorage1DEXT_) (GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) = nullptr;
    void     (GLAPIENTRYP ogluTextureStorage2DEXT_) (GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void     (GLAPIENTRYP ogluTextureStorage3DEXT_) (GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = nullptr;
    void     (GLAPIENTRYP ogluTexStorage1D_) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) = nullptr;
    void     (GLAPIENTRYP ogluTexStorage2D_) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void     (GLAPIENTRYP ogluTexStorage3D_) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = nullptr;
    void     (GLAPIENTRYP ogluClearTexImage) (GLuint texture, GLint level, GLenum format, GLenum type, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluClearTexSubImage) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* data) = nullptr;
    void     (GLAPIENTRYP ogluGetTextureLevelParameteriv_) (GLuint texture, GLint level, GLenum pname, GLint* params) = nullptr;
    void     (GLAPIENTRYP ogluGetTextureLevelParameterivEXT_) (GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params) = nullptr;
    void     (GLAPIENTRYP ogluGetTextureImage_) (GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) = nullptr;
    void     (GLAPIENTRYP ogluGetTextureImageEXT_) (GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void *pixels) = nullptr;
    void     (GLAPIENTRYP ogluGetCompressedTextureImage_) (GLuint texture, GLint level, GLsizei bufSize, void *img) = nullptr;
    void     (GLAPIENTRYP ogluGetCompressedTextureImageEXT_) (GLuint texture, GLenum target, GLint level, void *img) = nullptr;
    void     (GLAPIENTRYP ogluGetCompressedTexImage_) (GLenum target, GLint lod, void *img) = nullptr;

    void ogluCreateTextures(GLenum target, GLsizei n, GLuint* textures) const;
    void ogluBindTextureUnit(GLuint unit, GLuint texture, GLenum target) const;
    void ogluTextureBuffer(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer) const;
    void ogluGenerateTextureMipmap(GLuint texture, GLenum target) const;
    void ogluTextureParameteri(GLuint texture, GLenum target, GLenum pname, GLint param) const;
    void ogluTextureSubImage1D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) const;
    void ogluTextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) const;
    void ogluTextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) const;
    void ogluTextureImage1D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels) const;
    void ogluTextureImage2D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) const;
    void ogluTextureImage3D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) const;
    void ogluCompressedTextureSubImage1D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) const;
    void ogluCompressedTextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) const;
    void ogluCompressedTextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) const;
    void ogluCompressedTextureImage1D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void* data) const;
    void ogluCompressedTextureImage2D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) const;
    void ogluCompressedTextureImage3D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) const;
    void ogluTextureStorage1D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) const;
    void ogluTextureStorage2D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) const;
    void ogluTextureStorage3D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) const;
    void ogluGetTextureLevelParameteriv(GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params) const;
    void ogluGetTextureImage(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, size_t bufSize, void* pixels) const;
    void ogluGetCompressedTextureImage(GLuint texture, GLenum target, GLint level, size_t bufSize, void* img) const;

    // rbo related
    void (GLAPIENTRYP ogluGenRenderbuffers_) (GLsizei n, GLuint* renderbuffers) = nullptr;
    void (GLAPIENTRYP ogluCreateRenderbuffers_) (GLsizei n, GLuint* renderbuffers) = nullptr;
    void (GLAPIENTRYP ogluDeleteRenderbuffers) (GLsizei n, const GLuint* renderbuffers) = nullptr;
    void (GLAPIENTRYP ogluBindRenderbuffer_) (GLenum target, GLuint renderbuffer) = nullptr;
    void (GLAPIENTRYP ogluNamedRenderbufferStorage_) (GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRYP ogluRenderbufferStorage_) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRYP ogluNamedRenderbufferStorageMultisample_) (GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRYP ogluRenderbufferStorageMultisample_) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRYP ogluNamedRenderbufferStorageMultisampleCoverageEXT_) (GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRYP ogluRenderbufferStorageMultisampleCoverageNV_) (GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRYP ogluGetRenderbufferParameteriv_) (GLenum target, GLenum pname, GLint* params) = nullptr;

    void ogluCreateRenderbuffers(GLsizei n, GLuint* renderbuffers) const;
    void ogluNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) const;
    void ogluNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) const;
    void ogluNamedRenderbufferStorageMultisampleCoverage(GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) const;

    // fbo related
    mutable GLuint ReadFBO = 0, DrawFBO = 0;
    GLint MaxColorAttachment = 0;
    GLint MaxDrawBuffers = 0;
    void   (GLAPIENTRYP ogluGenFramebuffers_) (GLsizei n, GLuint* framebuffers) = nullptr;
    void   (GLAPIENTRYP ogluCreateFramebuffers_) (GLsizei n, GLuint *framebuffers) = nullptr;
    void   (GLAPIENTRYP ogluDeleteFramebuffers) (GLsizei n, const GLuint* framebuffers) = nullptr;
    void   (GLAPIENTRYP ogluBindFramebuffer_) (GLenum target, GLuint framebuffer) = nullptr;
    void   (GLAPIENTRYP ogluBlitNamedFramebuffer_) (GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) = nullptr;
    void   (GLAPIENTRYP ogluBlitFramebuffer_) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) = nullptr;
    void   (GLAPIENTRYP ogluDrawBuffers) (GLsizei n, const GLenum* bufs) = nullptr;
    void   (GLAPIENTRYP ogluInvalidateNamedFramebufferData_) (GLuint framebuffer, GLsizei numAttachments, const GLenum* attachments) = nullptr;
    void   (GLAPIENTRYP ogluInvalidateFramebuffer_) (GLenum target, GLsizei numAttachments, const GLenum* attachments) = nullptr;
    void   (GLAPIENTRYP ogluDiscardFramebufferEXT_) (GLenum target, GLsizei numAttachments, const GLenum* attachments) = nullptr;
    void   (GLAPIENTRYP ogluNamedFramebufferRenderbuffer_) (GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = nullptr;
    void   (GLAPIENTRYP ogluFramebufferRenderbuffer_) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = nullptr;
    void   (GLAPIENTRYP ogluNamedFramebufferTexture1DEXT_) (GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void   (GLAPIENTRYP ogluFramebufferTexture1D_) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void   (GLAPIENTRYP ogluNamedFramebufferTexture2DEXT_) (GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void   (GLAPIENTRYP ogluFramebufferTexture2D_) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void   (GLAPIENTRYP ogluNamedFramebufferTexture3DEXT_) (GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) = nullptr;
    void   (GLAPIENTRYP ogluFramebufferTexture3D_) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer) = nullptr;
    void   (GLAPIENTRYP ogluNamedFramebufferTexture_) (GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
    void   (GLAPIENTRYP ogluFramebufferTexture_) (GLenum target, GLenum attachment, GLuint texture, GLint level);
    void   (GLAPIENTRYP ogluNamedFramebufferTextureLayer_) (GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) = nullptr;
    void   (GLAPIENTRYP ogluFramebufferTextureLayer_) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) = nullptr;
    GLenum (GLAPIENTRYP ogluCheckNamedFramebufferStatus_) (GLuint framebuffer, GLenum target) = nullptr;
    GLenum (GLAPIENTRYP ogluCheckFramebufferStatus_) (GLenum target) = nullptr;
    void   (GLAPIENTRYP ogluGetNamedFramebufferAttachmentParameteriv_) (GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRYP ogluGetFramebufferAttachmentParameteriv_) (GLenum target, GLenum attachment, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRYP ogluClear) (GLbitfield mask) = nullptr;
    void   (GLAPIENTRYP ogluClearColor) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = nullptr;
    void   (GLAPIENTRYP ogluClearDepth_) (GLclampd d) = nullptr;
    void   (GLAPIENTRYP ogluClearDepthf_) (GLclampf f) = nullptr;
    void   (GLAPIENTRYP ogluClearStencil) (GLint s) = nullptr;
    void   (GLAPIENTRYP ogluClearNamedFramebufferiv_) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ogluClearBufferiv_) (GLenum buffer, GLint drawbuffer, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ogluClearNamedFramebufferuiv_) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value) = nullptr;
    void   (GLAPIENTRYP ogluClearBufferuiv_) (GLenum buffer, GLint drawbuffer, const GLuint* value) = nullptr;
    void   (GLAPIENTRYP ogluClearNamedFramebufferfv_) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluClearBufferfv_) (GLenum buffer, GLint drawbuffer, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluClearNamedFramebufferfi_) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) = nullptr;
    void   (GLAPIENTRYP ogluClearBufferfi_) (GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) = nullptr;

    void RefreshFBOState() const;
    void ogluCreateFramebuffers(GLsizei n, GLuint* framebuffers) const;
    void ogluBindFramebuffer(GLenum target, GLuint framebuffer) const;
    void ogluBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) const;
    void ogluInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum* attachments) const;
    void ogluNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) const;
    void ogluNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) const;
    void ogluNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer) const;
    [[nodiscard]] GLenum ogluCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) const;
    void ogluGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params) const;
    void ogluClearDepth(GLclampd d) const;
    void ogluClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value) const;
    void ogluClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value) const;
    void ogluClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value) const;
    void ogluClearNamedFramebufferDepthStencil(GLuint framebuffer, GLfloat depth, GLint stencil) const;

    // shader related
    GLuint (GLAPIENTRYP ogluCreateShader) (GLenum type) = nullptr;
    void   (GLAPIENTRYP ogluDeleteShader) (GLuint shader) = nullptr;
    void   (GLAPIENTRYP ogluShaderSource) (GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) = nullptr;
    void   (GLAPIENTRYP ogluCompileShader) (GLuint shader) = nullptr;
    void   (GLAPIENTRYP ogluGetShaderInfoLog) (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) = nullptr;
    void   (GLAPIENTRYP ogluGetShaderSource) (GLuint obj, GLsizei maxLength, GLsizei* length, GLchar* source) = nullptr;
    void   (GLAPIENTRYP ogluGetShaderiv) (GLuint shader, GLenum pname, GLint* param) = nullptr;
    
    //program related
    GLuint (GLAPIENTRYP ogluCreateProgram) () = nullptr;
    void   (GLAPIENTRYP ogluDeleteProgram) (GLuint program) = nullptr;
    void   (GLAPIENTRYP ogluAttachShader) (GLuint program, GLuint shader) = nullptr;
    void   (GLAPIENTRYP ogluDetachShader) (GLuint program, GLuint shader) = nullptr;
    void   (GLAPIENTRYP ogluLinkProgram) (GLuint program) = nullptr;
    void   (GLAPIENTRYP ogluUseProgram) (GLuint program) = nullptr;
    void   (GLAPIENTRYP ogluDispatchCompute) (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) = nullptr;
    void   (GLAPIENTRYP ogluDispatchComputeIndirect) (GLintptr indirect) = nullptr;
    
    void   (GLAPIENTRYP ogluUniform1f_) (GLint location, GLfloat v0) = nullptr;
    void   (GLAPIENTRYP ogluUniform1fv_) (GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluUniform1i_) (GLint location, GLint v0) = nullptr;
    void   (GLAPIENTRYP ogluUniform1iv_) (GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ogluUniform2f_) (GLint location, GLfloat v0, GLfloat v1) = nullptr;
    void   (GLAPIENTRYP ogluUniform2fv_) (GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluUniform2i_) (GLint location, GLint v0, GLint v1) = nullptr;
    void   (GLAPIENTRYP ogluUniform2iv_) (GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ogluUniform3f_) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2) = nullptr;
    void   (GLAPIENTRYP ogluUniform3fv_) (GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluUniform3i_) (GLint location, GLint v0, GLint v1, GLint v2) = nullptr;
    void   (GLAPIENTRYP ogluUniform3iv_) (GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ogluUniform4f_) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) = nullptr;
    void   (GLAPIENTRYP ogluUniform4fv_) (GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluUniform4i_) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3) = nullptr;
    void   (GLAPIENTRYP ogluUniform4iv_) (GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ogluUniformMatrix2fv_) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluUniformMatrix3fv_) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluUniformMatrix4fv_) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform1d) (GLuint program, GLint location, GLdouble x) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform1dv) (GLuint program, GLint location, GLsizei count, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform1f) (GLuint program, GLint location, GLfloat x) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform1fv) (GLuint program, GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform1i) (GLuint program, GLint location, GLint x) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform1iv) (GLuint program, GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform1ui) (GLuint program, GLint location, GLuint x) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform1uiv) (GLuint program, GLint location, GLsizei count, const GLuint* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform2d) (GLuint program, GLint location, GLdouble x, GLdouble y) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform2dv) (GLuint program, GLint location, GLsizei count, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform2f) (GLuint program, GLint location, GLfloat x, GLfloat y) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform2fv) (GLuint program, GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform2i) (GLuint program, GLint location, GLint x, GLint y) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform2iv) (GLuint program, GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform2ui) (GLuint program, GLint location, GLuint x, GLuint y) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform2uiv) (GLuint program, GLint location, GLsizei count, const GLuint* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform3d) (GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform3dv) (GLuint program, GLint location, GLsizei count, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform3f) (GLuint program, GLint location, GLfloat x, GLfloat y, GLfloat z) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform3fv) (GLuint program, GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform3i) (GLuint program, GLint location, GLint x, GLint y, GLint z) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform3iv) (GLuint program, GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform3ui) (GLuint program, GLint location, GLuint x, GLuint y, GLuint z) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform3uiv) (GLuint program, GLint location, GLsizei count, const GLuint* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform4d) (GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform4dv) (GLuint program, GLint location, GLsizei count, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform4f) (GLuint program, GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform4fv) (GLuint program, GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform4i) (GLuint program, GLint location, GLint x, GLint y, GLint z, GLint w) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform4iv) (GLuint program, GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform4ui) (GLuint program, GLint location, GLuint x, GLuint y, GLuint z, GLuint w) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniform4uiv) (GLuint program, GLint location, GLsizei count, const GLuint* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix2dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix2fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix2x3dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix2x3fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix2x4dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix2x4fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix3dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix3fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix3x2dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix3x2fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix3x4dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix3x4fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix4dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix4fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix4x2dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix4x2fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix4x3dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformMatrix4x3fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ogluProgramUniformHandleui64) (GLuint program, GLint location, GLuint64 value) = nullptr;

    void   (GLAPIENTRYP ogluGetUniformfv) (GLuint program, GLint location, GLfloat* params) = nullptr;
    void   (GLAPIENTRYP ogluGetUniformiv) (GLuint program, GLint location, GLint* params) = nullptr;
    void   (GLAPIENTRYP ogluGetUniformuiv) (GLuint program, GLint location, GLuint* params) = nullptr;
    void   (GLAPIENTRYP ogluGetProgramInfoLog) (GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) = nullptr;
    void   (GLAPIENTRYP ogluGetProgramiv) (GLuint program, GLenum pname, GLint* param) = nullptr;
    void   (GLAPIENTRYP ogluGetProgramInterfaceiv) (GLuint program, GLenum programInterface, GLenum pname, GLint* params) = nullptr;
    GLuint (GLAPIENTRYP ogluGetProgramResourceIndex) (GLuint program, GLenum programInterface, const GLchar* name) = nullptr;
    GLint  (GLAPIENTRYP ogluGetProgramResourceLocation) (GLuint program, GLenum programInterface, const GLchar* name) = nullptr;
    GLint  (GLAPIENTRYP ogluGetProgramResourceLocationIndex) (GLuint program, GLenum programInterface, const GLchar* name) = nullptr;
    void   (GLAPIENTRYP ogluGetProgramResourceName) (GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar* name) = nullptr;
    void   (GLAPIENTRYP ogluGetProgramResourceiv) (GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei bufSize, GLsizei* length, GLint* params) = nullptr;
    void   (GLAPIENTRYP ogluGetActiveSubroutineName) (GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei* length, GLchar* name) = nullptr;
    void   (GLAPIENTRYP ogluGetActiveSubroutineUniformName) (GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei* length, GLchar* name) = nullptr;
    void   (GLAPIENTRYP ogluGetActiveSubroutineUniformiv) (GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint* values) = nullptr;
    void   (GLAPIENTRYP ogluGetProgramStageiv) (GLuint program, GLenum shadertype, GLenum pname, GLint* values) = nullptr;
    GLuint (GLAPIENTRYP ogluGetSubroutineIndex) (GLuint program, GLenum shadertype, const GLchar* name) = nullptr;
    GLint  (GLAPIENTRYP ogluGetSubroutineUniformLocation) (GLuint program, GLenum shadertype, const GLchar* name) = nullptr;
    void   (GLAPIENTRYP ogluGetUniformSubroutineuiv) (GLenum shadertype, GLint location, GLuint* params) = nullptr;
    void   (GLAPIENTRYP ogluUniformSubroutinesuiv) (GLenum shadertype, GLsizei count, const GLuint* indices) = nullptr;
    void   (GLAPIENTRYP ogluGetActiveUniformBlockName) (GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName) = nullptr;
    void   (GLAPIENTRYP ogluGetActiveUniformBlockiv) (GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRYP ogluGetActiveUniformName) (GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformName) = nullptr;
    void   (GLAPIENTRYP ogluGetActiveUniformsiv) (GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRYP ogluGetIntegeri_v) (GLenum target, GLuint index, GLint* data) = nullptr;
    GLuint (GLAPIENTRYP ogluGetUniformBlockIndex) (GLuint program, const GLchar* uniformBlockName) = nullptr;
    void   (GLAPIENTRYP ogluGetUniformIndices) (GLuint program, GLsizei uniformCount, const GLchar* const* uniformNames, GLuint* uniformIndices) = nullptr;
    void   (GLAPIENTRYP ogluUniformBlockBinding) (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) = nullptr;

    // query related
    void   (GLAPIENTRYP ogluGenQueries) (GLsizei n, GLuint* ids) = nullptr;
    void   (GLAPIENTRYP ogluDeleteQueries) (GLsizei n, const GLuint* ids) = nullptr;
    void   (GLAPIENTRYP ogluBeginQuery) (GLenum target, GLuint id) = nullptr;
    void   (GLAPIENTRYP ogluQueryCounter) (GLuint id, GLenum target) = nullptr;
    void   (GLAPIENTRYP ogluGetQueryObjectiv) (GLuint id, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRYP ogluGetQueryObjectuiv) (GLuint id, GLenum pname, GLuint* params) = nullptr;
    void   (GLAPIENTRYP ogluGetQueryObjecti64v) (GLuint id, GLenum pname, GLint64* params) = nullptr;
    void   (GLAPIENTRYP ogluGetQueryObjectui64v) (GLuint id, GLenum pname, GLuint64* params) = nullptr;
    void   (GLAPIENTRYP ogluGetQueryiv) (GLenum target, GLenum pname, GLint* params) = nullptr;
    GLsync (GLAPIENTRYP ogluFenceSync) (GLenum condition, GLbitfield flags) = nullptr;
    void   (GLAPIENTRYP ogluDeleteSync) (GLsync GLsync) = nullptr;
    GLenum (GLAPIENTRYP ogluClientWaitSync) (GLsync GLsync, GLbitfield flags, GLuint64 timeout) = nullptr;
    void   (GLAPIENTRYP ogluWaitSync) (GLsync GLsync, GLbitfield flags, GLuint64 timeout) = nullptr;
    void   (GLAPIENTRYP ogluGetInteger64v) (GLenum pname, GLint64* params) = nullptr;
    void   (GLAPIENTRYP ogluGetSynciv) (GLsync GLsync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values) = nullptr;

    // debug
    GLint MaxLabelLen = 0;
    GLsizei MaxMessageLen = 0;
    using DebugCallback    = void (GLAPIENTRYP)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
    using DebugCallbackAMD = void (GLAPIENTRYP)(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
    void (GLAPIENTRYP ogluDebugMessageCallback) (DebugCallback callback, const void* userParam) = nullptr;
    void (GLAPIENTRYP ogluDebugMessageCallbackAMD) (DebugCallbackAMD callback, const void* userParam) = nullptr;
    void (GLAPIENTRYP ogluObjectLabel_) (GLenum identifier, GLuint name, GLsizei length, const GLchar* label) = nullptr;
    void (GLAPIENTRYP ogluLabelObjectEXT_) (GLenum type, GLuint object, GLsizei length, const GLchar* label) = nullptr;
    void (GLAPIENTRYP ogluObjectPtrLabel_) (void* ptr, GLsizei length, const GLchar* label) = nullptr;
    void (GLAPIENTRYP ogluPushDebugGroup_) (GLenum source, GLuint id, GLsizei length, const GLchar* message) = nullptr;
    void (GLAPIENTRYP ogluPopDebugGroup_) () = nullptr;
    void (GLAPIENTRYP ogluPushGroupMarkerEXT_) (GLsizei length, const GLchar* marker) = nullptr;
    void (GLAPIENTRYP ogluPopGroupMarkerEXT_) () = nullptr;
    void (GLAPIENTRYP ogluDebugMessageInsert_) (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* buf) = nullptr;
    void (GLAPIENTRYP ogluDebugMessageInsertAMD) (GLenum category, GLenum severity, GLuint id, GLsizei length, const GLchar* buf) = nullptr;

    void ogluSetObjectLabel(GLenum identifier, GLuint id, std::u16string_view name) const;
    void ogluSetObjectLabel(GLsync sync, std::u16string_view name) const;
    void ogluPushDebugGroup(GLenum source, GLuint id, std::u16string_view message) const;
    void ogluPopDebugGroup() const;
    void ogluInsertDebugMarker(GLuint id, std::u16string_view name) const;

    // others
    GLenum         (GLAPIENTRYP ogluGetError) () = nullptr;
    void           (GLAPIENTRYP ogluGetFloatv) (GLenum pname, GLfloat* params) = nullptr;
    void           (GLAPIENTRYP ogluGetIntegerv) (GLenum pname, GLint* params) = nullptr;
    const GLubyte* (GLAPIENTRYP ogluGetString) (GLenum name) = nullptr;
    const char*    (GLAPIENTRYP ogluGetStringi) (GLenum name, GLuint index) = nullptr;
    GLboolean      (GLAPIENTRYP ogluIsEnabled) (GLenum cap) = nullptr;
    void           (GLAPIENTRYP ogluEnable) (GLenum cap) = nullptr;
    void           (GLAPIENTRYP ogluDisable) (GLenum cap) = nullptr;
    void           (GLAPIENTRYP ogluFinish) () = nullptr;
    void           (GLAPIENTRYP ogluFlush) () = nullptr;
    void           (GLAPIENTRYP ogluDepthFunc) (GLenum func) = nullptr;
    void           (GLAPIENTRYP ogluCullFace) (GLenum mode) = nullptr;
    void           (GLAPIENTRYP ogluFrontFace) (GLenum mode) = nullptr;
    void           (GLAPIENTRYP ogluViewport) (GLint x, GLint y, GLsizei width, GLsizei height) = nullptr;
    void           (GLAPIENTRYP ogluViewportArrayv) (GLuint first, GLsizei count, const GLfloat* v) = nullptr;
    void           (GLAPIENTRYP ogluViewportIndexedf) (GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h) = nullptr;
    void           (GLAPIENTRYP ogluViewportIndexedfv) (GLuint index, const GLfloat* v) = nullptr;
    void           (GLAPIENTRYP ogluClipControl) (GLenum origin, GLenum depth) = nullptr;
    void           (GLAPIENTRYP ogluMemoryBarrier) (GLbitfield barriers) = nullptr;

    CtxFuncs(void*);
private:
    [[nodiscard]] common::container::FrozenDenseSet<std::string_view> GetExtensions() const;
public:
    [[nodiscard]] std::optional<std::string_view> GetError() const;
};
extern thread_local const CtxFuncs* CtxFunc;

extern std::atomic_uint32_t LatestVersion;


}


