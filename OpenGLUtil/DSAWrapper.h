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

namespace oglu
{

class oglIndirectBuffer_;
class oglContext_;


class PlatFuncs
{
#if COMMON_OS_WIN
private:
    const char* (GLAPIENTRY *wglGetExtensionsStringARB_) (void* hDC) = nullptr;
    const char* (GLAPIENTRY *wglGetExtensionsStringEXT_) () = nullptr;
    [[nodiscard]] common::container::FrozenDenseSet<std::string_view> GetExtensions(void* hdc) const;
public:
    void* (GLAPIENTRY *wglCreateContextAttribsARB_) (void* hDC, void* hShareContext, const int* attribList) = nullptr;
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

    PlatFuncs();

    [[nodiscard]] static void* GetCurrentDeviceContext();
    [[nodiscard]] static void* GetCurrentGLContext();
    static void  DeleteGLContext(void* hDC, void* hRC);
    [[nodiscard]] static std::vector<int32_t> GenerateContextAttrib(const uint32_t version, bool needFlushControl);
    [[nodiscard]] static void* CreateNewContext(const oglContext_* prevCtx, const bool isShared, const int32_t* attribs);
#if COMMON_OS_WIN
    [[nodiscard]] static bool  MakeGLContextCurrent(void* hDC, void* hRC);
    [[nodiscard]] static uint32_t GetSystemError();
#else
    [[nodiscard]] static bool  MakeGLContextCurrent(void* hDC, unsigned long DRW, void* hRC);
    [[nodiscard]] static unsigned long GetCurrentDrawable();
    [[nodiscard]] static int32_t GetSystemError();
#endif
};
extern thread_local const PlatFuncs* PlatFunc;



struct DSAFuncs
{
    friend struct VAOBinder;
    friend struct FBOBinder;
private:
    mutable common::SpinLocker DataLock;
public:
    // buffer related

    void      (GLAPIENTRY *ogluGenBuffers) (GLsizei n, GLuint* buffers) = nullptr;
    void      (GLAPIENTRY *ogluDeleteBuffers) (GLsizei n, const GLuint* buffers) = nullptr;
    void      (GLAPIENTRY *ogluBindBuffer) (GLenum target, GLuint buffer) = nullptr;
    void      (GLAPIENTRY *ogluBindBufferBase) (GLenum target, GLuint index, GLuint buffer) = nullptr;
    void      (GLAPIENTRY *ogluBindBufferRange) (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) = nullptr;
    void      (GLAPIENTRY *ogluNamedBufferStorage_) (GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) = nullptr;
    void      (GLAPIENTRY *ogluBufferStorage_) (GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) = nullptr;
    void      (GLAPIENTRY *ogluNamedBufferData_) (GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) = nullptr;
    void      (GLAPIENTRY *ogluBufferData_) (GLenum target, GLsizeiptr size, const void* data, GLenum usage) = nullptr;
    void      (GLAPIENTRY *ogluNamedBufferSubData_) (GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data) = nullptr;
    void      (GLAPIENTRY *ogluBufferSubData_) (GLenum target, GLintptr offset, GLsizeiptr size, const void* data) = nullptr;
    void*     (GLAPIENTRY *ogluMapNamedBuffer_) (GLuint buffer, GLenum access) = nullptr;
    void*     (GLAPIENTRY *ogluMapBuffer_) (GLenum target, GLenum access) = nullptr;
    GLboolean (GLAPIENTRY *ogluUnmapNamedBuffer_) (GLuint buffer) = nullptr;
    GLboolean (GLAPIENTRY *ogluUnmapBuffer_) (GLenum target) = nullptr;

    void ogluNamedBufferStorage(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) const;
    void ogluNamedBufferData(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) const;
    void ogluNamedBufferSubData(GLenum target, GLuint buffer, GLintptr offset, GLsizeiptr size, const void* data) const;
    [[nodiscard]] void* ogluMapNamedBuffer(GLenum target, GLuint buffer, GLenum access) const;
    GLboolean ogluUnmapNamedBuffer(GLenum target, GLuint buffer) const;
    
    // vao related
    mutable GLuint VAO = 0;
    void (GLAPIENTRY *ogluGenVertexArrays) (GLsizei n, GLuint* arrays) = nullptr;
    void (GLAPIENTRY *ogluDeleteVertexArrays) (GLsizei n, const GLuint* arrays) = nullptr;
    void (GLAPIENTRY *ogluBindVertexArray_) (GLuint vaobj) = nullptr;
    void (GLAPIENTRY *ogluEnableVertexAttribArray_) (GLuint index) = nullptr;
    void (GLAPIENTRY *ogluEnableVertexArrayAttrib_) (GLuint vaobj, GLuint index) = nullptr;
    void (GLAPIENTRY *ogluVertexAttribIPointer_) (GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRY *ogluVertexAttribLPointer_) (GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRY *ogluVertexAttribPointer_) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRY *ogluVertexAttribDivisor) (GLuint index, GLuint divisor) = nullptr;

    void RefreshVAOState() const;
    void ogluBindVertexArray(GLuint vaobj) const;
    void ogluEnableVertexArrayAttrib(GLuint vaobj, GLuint index) const;
    void ogluVertexAttribPointer(GLuint index, GLint size, GLenum type, bool normalized, GLsizei stride, size_t offset) const;
    void ogluVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const;
    void ogluVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const;

    // draw related
    void (GLAPIENTRY *ogluDrawArrays) (GLenum mode, GLint first, GLsizei count) = nullptr;
    void (GLAPIENTRY *ogluDrawElements) (GLenum mode, GLsizei count, GLenum type, const void* indices) = nullptr;
    void (GLAPIENTRY *ogluMultiDrawArrays) (GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) = nullptr;
    void (GLAPIENTRY *ogluMultiDrawElements) (GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount) = nullptr;
    void (GLAPIENTRY *ogluMultiDrawArraysIndirect_) (GLenum mode, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;
    void (GLAPIENTRY *ogluDrawArraysInstancedBaseInstance_) (GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) = nullptr;
    void (GLAPIENTRY *ogluDrawArraysInstanced_) (GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
    void (GLAPIENTRY *ogluMultiDrawElementsIndirect_) (GLenum mode, GLenum type, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;
    void (GLAPIENTRY *ogluDrawElementsInstancedBaseVertexBaseInstance_) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) = nullptr;
    void (GLAPIENTRY *ogluDrawElementsInstancedBaseInstance_) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLuint baseinstance) = nullptr;
    void (GLAPIENTRY *ogluDrawElementsInstanced_) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) = nullptr;

    void ogluDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, uint32_t instancecount, uint32_t baseinstance) const;
    void ogluDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, uint32_t instancecount, uint32_t baseinstance) const;
    void ogluMultiDrawArraysIndirect(GLenum mode, const oglIndirectBuffer_& indirect, GLint offset, GLsizei drawcount) const;
    void ogluMultiDrawElementsIndirect(GLenum mode, GLenum type, const oglIndirectBuffer_& indirect, GLint offset, GLsizei drawcount) const;

    // texture related
    void (GLAPIENTRY *ogluGenTextures) (GLsizei n, GLuint* textures) = nullptr;
    void (GLAPIENTRY *ogluCreateTextures_) (GLenum target, GLsizei n, GLuint* textures) = nullptr;
    void (GLAPIENTRY *ogluDeleteTextures) (GLsizei n, const GLuint* textures) = nullptr;
    void (GLAPIENTRY *ogluActiveTexture) (GLenum texture) = nullptr;
    void (GLAPIENTRY *ogluBindTexture) (GLenum target, GLuint texture) = nullptr;
    void (GLAPIENTRY *ogluBindTextureUnit_) (GLuint unit, GLuint texture) = nullptr;
    void (GLAPIENTRY *ogluBindMultiTextureEXT_) (GLuint unit, GLenum target, GLuint texture) = nullptr;
    void (GLAPIENTRY *ogluBindImageTexture) (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) = nullptr;
    void (GLAPIENTRY *ogluTextureView) (GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) = nullptr;
    void (GLAPIENTRY *ogluTextureBuffer_) (GLuint texture, GLenum internalformat, GLuint buffer) = nullptr;
    void (GLAPIENTRY *ogluTextureBufferEXT_) (GLuint texture, GLenum target, GLenum internalformat, GLuint buffer) = nullptr;
    void (GLAPIENTRY *ogluTexBuffer_) (GLenum target, GLenum internalformat, GLuint buffer) = nullptr;
    void (GLAPIENTRY *ogluGenerateTextureMipmap_) (GLuint texture) = nullptr;
    void (GLAPIENTRY *ogluGenerateTextureMipmapEXT_) (GLuint texture, GLenum target) = nullptr;
    void (GLAPIENTRY *ogluGenerateMipmap_) (GLenum target) = nullptr;
    void (GLAPIENTRY *ogluTextureParameteri_) (GLuint texture, GLenum pname, GLint param) = nullptr;
    void (GLAPIENTRY *ogluTextureParameteriEXT_) (GLuint texture, GLenum target, GLenum pname, GLint param) = nullptr;
    void (GLAPIENTRY *ogluTextureSubImage1D_) (GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureSubImage2D_) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureSubImage3D_) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureSubImage1DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureSubImage2DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureSubImage3DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (GLAPIENTRY *ogluTexSubImage3D_) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureImage1DEXT_) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureImage2DEXT_) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureImage3DEXT_) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (GLAPIENTRY *ogluTexImage3D_) (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureSubImage1D_) (GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureSubImage2D_) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureSubImage3D_) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureSubImage1DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureSubImage2DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureSubImage3DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTexSubImage1D_) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTexSubImage2D_) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTexSubImage3D_) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureImage1DEXT_) (GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureImage2DEXT_) (GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureImage3DEXT_) (GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTexImage1D_) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTexImage2D_) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTexImage3D_) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void (GLAPIENTRY *ogluCopyImageSubData) (GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) = nullptr;
    void (GLAPIENTRY *ogluTextureStorage1D_) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) = nullptr;
    void (GLAPIENTRY *ogluTextureStorage2D_) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRY *ogluTextureStorage3D_) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = nullptr;
    void (GLAPIENTRY *ogluTextureStorage1DEXT_) (GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) = nullptr;
    void (GLAPIENTRY *ogluTextureStorage2DEXT_) (GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRY *ogluTextureStorage3DEXT_) (GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = nullptr;
    void (GLAPIENTRY *ogluTexStorage1D_) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) = nullptr;
    void (GLAPIENTRY *ogluTexStorage2D_) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRY *ogluTexStorage3D_) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = nullptr;
    void (GLAPIENTRY *ogluClearTexImage) (GLuint texture, GLint level, GLenum format, GLenum type, const void* data) = nullptr;
    void (GLAPIENTRY *ogluClearTexSubImage) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* data) = nullptr;
    void (GLAPIENTRY *ogluGetTextureLevelParameteriv_) (GLuint texture, GLint level, GLenum pname, GLint* params) = nullptr;
    void (GLAPIENTRY *ogluGetTextureLevelParameterivEXT_) (GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params) = nullptr;
    void (GLAPIENTRY *ogluGetTextureImage_) (GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) = nullptr;
    void (GLAPIENTRY *ogluGetTextureImageEXT_) (GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void *pixels) = nullptr;
    void (GLAPIENTRY *ogluGetCompressedTextureImage_) (GLuint texture, GLint level, GLsizei bufSize, void *img) = nullptr;
    void (GLAPIENTRY *ogluGetCompressedTextureImageEXT_) (GLuint texture, GLenum target, GLint level, void *img) = nullptr;
    void (GLAPIENTRY *ogluGetCompressedTexImage_) (GLenum target, GLint lod, void *img) = nullptr;

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
    void (GLAPIENTRY *ogluGenRenderbuffers_) (GLsizei n, GLuint* renderbuffers) = nullptr;
    void (GLAPIENTRY *ogluCreateRenderbuffers_) (GLsizei n, GLuint* renderbuffers) = nullptr;
    void (GLAPIENTRY *ogluDeleteRenderbuffers) (GLsizei n, const GLuint* renderbuffers) = nullptr;
    void (GLAPIENTRY *ogluBindRenderbuffer_) (GLenum target, GLuint renderbuffer) = nullptr;
    void (GLAPIENTRY *ogluNamedRenderbufferStorage_) (GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRY *ogluRenderbufferStorage_) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRY *ogluNamedRenderbufferStorageMultisample_) (GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRY *ogluRenderbufferStorageMultisample_) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRY *ogluNamedRenderbufferStorageMultisampleCoverageEXT_) (GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRY *ogluRenderbufferStorageMultisampleCoverageNV_) (GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRY *ogluGetRenderbufferParameteriv_) (GLenum target, GLenum pname, GLint* params) = nullptr;

    void ogluCreateRenderbuffers(GLsizei n, GLuint* renderbuffers) const;
    void ogluNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) const;
    void ogluNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) const;
    void ogluNamedRenderbufferStorageMultisampleCoverage(GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) const;

    // fbo related
    mutable GLuint ReadFBO = 0, DrawFBO = 0;
    GLint MaxColorAttachment = 0;
    void   (GLAPIENTRY *ogluGenFramebuffers_) (GLsizei n, GLuint* framebuffers) = nullptr;
    void   (GLAPIENTRY *ogluCreateFramebuffers_) (GLsizei n, GLuint *framebuffers) = nullptr;
    void   (GLAPIENTRY *ogluDeleteFramebuffers) (GLsizei n, const GLuint* framebuffers) = nullptr;
    void   (GLAPIENTRY *ogluBindFramebuffer_) (GLenum target, GLuint framebuffer) = nullptr;
    void   (GLAPIENTRY *ogluBlitNamedFramebuffer_) (GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) = nullptr;
    void   (GLAPIENTRY *ogluBlitFramebuffer_) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) = nullptr;
    void   (GLAPIENTRY *ogluNamedFramebufferRenderbuffer_) (GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = nullptr;
    void   (GLAPIENTRY *ogluFramebufferRenderbuffer_) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = nullptr;
    void   (GLAPIENTRY *ogluNamedFramebufferTexture1DEXT_) (GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void   (GLAPIENTRY *ogluFramebufferTexture1D_) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void   (GLAPIENTRY *ogluNamedFramebufferTexture2DEXT_) (GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void   (GLAPIENTRY *ogluFramebufferTexture2D_) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void   (GLAPIENTRY *ogluNamedFramebufferTexture3DEXT_) (GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) = nullptr;
    void   (GLAPIENTRY *ogluFramebufferTexture3D_) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer) = nullptr;
    void   (GLAPIENTRY *ogluNamedFramebufferTexture_) (GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
    void   (GLAPIENTRY *ogluFramebufferTexture_) (GLenum target, GLenum attachment, GLuint texture, GLint level);
    void   (GLAPIENTRY *ogluNamedFramebufferTextureLayer_) (GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) = nullptr;
    void   (GLAPIENTRY *ogluFramebufferTextureLayer_) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) = nullptr;
    GLenum (GLAPIENTRY *ogluCheckNamedFramebufferStatus_) (GLuint framebuffer, GLenum target) = nullptr;
    GLenum (GLAPIENTRY *ogluCheckFramebufferStatus_) (GLenum target) = nullptr;
    void   (GLAPIENTRY *ogluGetNamedFramebufferAttachmentParameteriv_) (GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRY *ogluGetFramebufferAttachmentParameteriv_) (GLenum target, GLenum attachment, GLenum pname, GLint* params) = nullptr;
    
    void RefreshFBOState() const;
    void ogluCreateFramebuffers(GLsizei n, GLuint* framebuffers) const;
    void ogluBindFramebuffer(GLenum target, GLuint framebuffer) const;
    void ogluBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) const;
    void ogluNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) const;
    void ogluNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) const;
    void ogluNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer) const;
    [[nodiscard]] GLenum ogluCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) const;
    void ogluGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params) const;

    // shader related
    GLuint (GLAPIENTRY *ogluCreateShader) (GLenum type) = nullptr;
    void   (GLAPIENTRY *ogluDeleteShader) (GLuint shader) = nullptr;
    void   (GLAPIENTRY *ogluShaderSource) (GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) = nullptr;
    void   (GLAPIENTRY *ogluCompileShader) (GLuint shader) = nullptr;
    void   (GLAPIENTRY *ogluGetShaderInfoLog) (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) = nullptr;
    void   (GLAPIENTRY *ogluGetShaderSource) (GLuint obj, GLsizei maxLength, GLsizei* length, GLchar* source) = nullptr;
    void   (GLAPIENTRY *ogluGetShaderiv) (GLuint shader, GLenum pname, GLint* param) = nullptr;
    
    //program related
    GLuint (GLAPIENTRY *ogluCreateProgram) () = nullptr;
    void   (GLAPIENTRY *ogluDeleteProgram) (GLuint program) = nullptr;
    void   (GLAPIENTRY *ogluAttachShader) (GLuint program, GLuint shader) = nullptr;
    void   (GLAPIENTRY *ogluDetachShader) (GLuint program, GLuint shader) = nullptr;
    void   (GLAPIENTRY *ogluLinkProgram) (GLuint program) = nullptr;
    void   (GLAPIENTRY *ogluUseProgram) (GLuint program) = nullptr;
    void   (GLAPIENTRY *ogluDispatchCompute) (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) = nullptr;
    void   (GLAPIENTRY *ogluDispatchComputeIndirect) (GLintptr indirect) = nullptr;
    
    void   (GLAPIENTRY *ogluUniform1f_) (GLint location, GLfloat v0) = nullptr;
    void   (GLAPIENTRY *ogluUniform1fv_) (GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluUniform1i_) (GLint location, GLint v0) = nullptr;
    void   (GLAPIENTRY *ogluUniform1iv_) (GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRY *ogluUniform2f_) (GLint location, GLfloat v0, GLfloat v1) = nullptr;
    void   (GLAPIENTRY *ogluUniform2fv_) (GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluUniform2i_) (GLint location, GLint v0, GLint v1) = nullptr;
    void   (GLAPIENTRY *ogluUniform2iv_) (GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRY *ogluUniform3f_) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2) = nullptr;
    void   (GLAPIENTRY *ogluUniform3fv_) (GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluUniform3i_) (GLint location, GLint v0, GLint v1, GLint v2) = nullptr;
    void   (GLAPIENTRY *ogluUniform3iv_) (GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRY *ogluUniform4f_) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) = nullptr;
    void   (GLAPIENTRY *ogluUniform4fv_) (GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluUniform4i_) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3) = nullptr;
    void   (GLAPIENTRY *ogluUniform4iv_) (GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRY *ogluUniformMatrix2fv_) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluUniformMatrix3fv_) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluUniformMatrix4fv_) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform1d) (GLuint program, GLint location, GLdouble x) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform1dv) (GLuint program, GLint location, GLsizei count, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform1f) (GLuint program, GLint location, GLfloat x) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform1fv) (GLuint program, GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform1i) (GLuint program, GLint location, GLint x) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform1iv) (GLuint program, GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform1ui) (GLuint program, GLint location, GLuint x) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform1uiv) (GLuint program, GLint location, GLsizei count, const GLuint* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform2d) (GLuint program, GLint location, GLdouble x, GLdouble y) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform2dv) (GLuint program, GLint location, GLsizei count, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform2f) (GLuint program, GLint location, GLfloat x, GLfloat y) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform2fv) (GLuint program, GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform2i) (GLuint program, GLint location, GLint x, GLint y) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform2iv) (GLuint program, GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform2ui) (GLuint program, GLint location, GLuint x, GLuint y) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform2uiv) (GLuint program, GLint location, GLsizei count, const GLuint* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform3d) (GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform3dv) (GLuint program, GLint location, GLsizei count, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform3f) (GLuint program, GLint location, GLfloat x, GLfloat y, GLfloat z) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform3fv) (GLuint program, GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform3i) (GLuint program, GLint location, GLint x, GLint y, GLint z) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform3iv) (GLuint program, GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform3ui) (GLuint program, GLint location, GLuint x, GLuint y, GLuint z) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform3uiv) (GLuint program, GLint location, GLsizei count, const GLuint* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform4d) (GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform4dv) (GLuint program, GLint location, GLsizei count, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform4f) (GLuint program, GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform4fv) (GLuint program, GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform4i) (GLuint program, GLint location, GLint x, GLint y, GLint z, GLint w) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform4iv) (GLuint program, GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform4ui) (GLuint program, GLint location, GLuint x, GLuint y, GLuint z, GLuint w) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniform4uiv) (GLuint program, GLint location, GLsizei count, const GLuint* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix2dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix2fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix2x3dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix2x3fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix2x4dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix2x4fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix3dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix3fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix3x2dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix3x2fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix3x4dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix3x4fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix4dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix4fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix4x2dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix4x2fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix4x3dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRY *ogluProgramUniformMatrix4x3fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    
    void   (GLAPIENTRY *ogluGetUniformfv) (GLuint program, GLint location, GLfloat* params) = nullptr;
    void   (GLAPIENTRY *ogluGetUniformiv) (GLuint program, GLint location, GLint* params) = nullptr;
    void   (GLAPIENTRY *ogluGetUniformuiv) (GLuint program, GLint location, GLuint* params) = nullptr;
    void   (GLAPIENTRY *ogluGetProgramInfoLog) (GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) = nullptr;
    void   (GLAPIENTRY *ogluGetProgramiv) (GLuint program, GLenum pname, GLint* param) = nullptr;
    void   (GLAPIENTRY *ogluGetProgramInterfaceiv) (GLuint program, GLenum programInterface, GLenum pname, GLint* params) = nullptr;
    GLuint (GLAPIENTRY *ogluGetProgramResourceIndex) (GLuint program, GLenum programInterface, const GLchar* name) = nullptr;
    GLint  (GLAPIENTRY *ogluGetProgramResourceLocation) (GLuint program, GLenum programInterface, const GLchar* name) = nullptr;
    GLint  (GLAPIENTRY *ogluGetProgramResourceLocationIndex) (GLuint program, GLenum programInterface, const GLchar* name) = nullptr;
    void   (GLAPIENTRY *ogluGetProgramResourceName) (GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar* name) = nullptr;
    void   (GLAPIENTRY *ogluGetProgramResourceiv) (GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei bufSize, GLsizei* length, GLint* params) = nullptr;
    void   (GLAPIENTRY *ogluGetActiveSubroutineName) (GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei* length, GLchar* name) = nullptr;
    void   (GLAPIENTRY *ogluGetActiveSubroutineUniformName) (GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei* length, GLchar* name) = nullptr;
    void   (GLAPIENTRY *ogluGetActiveSubroutineUniformiv) (GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint* values) = nullptr;
    void   (GLAPIENTRY *ogluGetProgramStageiv) (GLuint program, GLenum shadertype, GLenum pname, GLint* values) = nullptr;
    GLuint (GLAPIENTRY *ogluGetSubroutineIndex) (GLuint program, GLenum shadertype, const GLchar* name) = nullptr;
    GLint  (GLAPIENTRY *ogluGetSubroutineUniformLocation) (GLuint program, GLenum shadertype, const GLchar* name) = nullptr;
    void   (GLAPIENTRY *ogluGetUniformSubroutineuiv) (GLenum shadertype, GLint location, GLuint* params) = nullptr;
    void   (GLAPIENTRY *ogluUniformSubroutinesuiv) (GLenum shadertype, GLsizei count, const GLuint* indices) = nullptr;
    void   (GLAPIENTRY *ogluGetActiveUniformBlockName) (GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName) = nullptr;
    void   (GLAPIENTRY *ogluGetActiveUniformBlockiv) (GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRY *ogluGetActiveUniformName) (GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformName) = nullptr;
    void   (GLAPIENTRY *ogluGetActiveUniformsiv) (GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRY *ogluGetIntegeri_v) (GLenum target, GLuint index, GLint* data) = nullptr;
    GLuint (GLAPIENTRY *ogluGetUniformBlockIndex) (GLuint program, const GLchar* uniformBlockName) = nullptr;
    void   (GLAPIENTRY *ogluGetUniformIndices) (GLuint program, GLsizei uniformCount, const GLchar* const* uniformNames, GLuint* uniformIndices) = nullptr;
    void   (GLAPIENTRY *ogluUniformBlockBinding) (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) = nullptr;

    // query related
    void   (GLAPIENTRY *ogluGenQueries) (GLsizei n, GLuint* ids) = nullptr;
    void   (GLAPIENTRY *ogluDeleteQueries) (GLsizei n, const GLuint* ids) = nullptr;
    void   (GLAPIENTRY *ogluBeginQuery) (GLenum target, GLuint id) = nullptr;
    void   (GLAPIENTRY *ogluQueryCounter) (GLuint id, GLenum target) = nullptr;
    void   (GLAPIENTRY *ogluGetQueryObjectiv) (GLuint id, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRY *ogluGetQueryObjectuiv) (GLuint id, GLenum pname, GLuint* params) = nullptr;
    void   (GLAPIENTRY *ogluGetQueryObjecti64v) (GLuint id, GLenum pname, GLint64* params) = nullptr;
    void   (GLAPIENTRY *ogluGetQueryObjectui64v) (GLuint id, GLenum pname, GLuint64* params) = nullptr;
    void   (GLAPIENTRY *ogluGetQueryiv) (GLenum target, GLenum pname, GLint* params) = nullptr;
    GLsync (GLAPIENTRY *ogluFenceSync) (GLenum condition, GLbitfield flags) = nullptr;
    void   (GLAPIENTRY *ogluDeleteSync) (GLsync GLsync) = nullptr;
    GLenum (GLAPIENTRY *ogluClientWaitSync) (GLsync GLsync, GLbitfield flags, GLuint64 timeout) = nullptr;
    void   (GLAPIENTRY *ogluWaitSync) (GLsync GLsync, GLbitfield flags, GLuint64 timeout) = nullptr;
    void   (GLAPIENTRY *ogluGetInteger64v) (GLenum pname, GLint64* params) = nullptr;
    void   (GLAPIENTRY *ogluGetSynciv) (GLsync GLsync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values) = nullptr;

    // debug
    GLint MaxLabelLen = 0;
    using DebugCallback = void (GLAPIENTRY*)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * message, const void* userParam);
    void (GLAPIENTRY *ogluDebugMessageCallback) (DebugCallback callback, const void* userParam) = nullptr;
    void (GLAPIENTRY *ogluObjectLabel_) (GLenum identifier, GLuint name, GLsizei length, const GLchar* label) = nullptr;
    void (GLAPIENTRY *ogluObjectPtrLabel_) (void* ptr, GLsizei length, const GLchar* label) = nullptr;

    void ogluSetObjectLabel(GLenum type, GLuint id, std::u16string_view name) const;
    void ogluSetObjectLabel(GLsync sync, std::u16string_view name) const;

    // others
    GLenum         (GLAPIENTRY *ogluGetError) () = nullptr;
    void           (GLAPIENTRY *ogluGetFloatv) (GLenum pname, GLfloat* params) = nullptr;
    void           (GLAPIENTRY *ogluGetIntegerv) (GLenum pname, GLint* params) = nullptr;
    const GLubyte* (GLAPIENTRY *ogluGetString) (GLenum name) = nullptr;
    const GLubyte* (GLAPIENTRY *ogluGetStringi) (GLenum name, GLuint index) = nullptr;
    GLboolean      (GLAPIENTRY *ogluIsEnabled) (GLenum cap) = nullptr;
    void           (GLAPIENTRY *ogluEnable) (GLenum cap) = nullptr;
    void           (GLAPIENTRY *ogluDisable) (GLenum cap) = nullptr;
    void           (GLAPIENTRY *ogluFinish) () = nullptr;
    void           (GLAPIENTRY *ogluFlush) () = nullptr;
    void           (GLAPIENTRY *ogluDepthFunc) (GLenum func) = nullptr;
    void           (GLAPIENTRY *ogluCullFace) (GLenum mode) = nullptr;
    void           (GLAPIENTRY *ogluFrontFace) (GLenum mode) = nullptr;
    void           (GLAPIENTRY *ogluClear) (GLbitfield mask) = nullptr;
    void           (GLAPIENTRY *ogluViewport) (GLint x, GLint y, GLsizei width, GLsizei height) = nullptr;
    void           (GLAPIENTRY *ogluViewportArrayv) (GLuint first, GLsizei count, const GLfloat* v) = nullptr;
    void           (GLAPIENTRY *ogluViewportIndexedf) (GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h) = nullptr;
    void           (GLAPIENTRY *ogluViewportIndexedfv) (GLuint index, const GLfloat* v) = nullptr;
    void           (GLAPIENTRY *ogluClearDepth_) (GLclampd d) = nullptr;
    void           (GLAPIENTRY *ogluClearDepthf_) (GLclampf f) = nullptr;
    void           (GLAPIENTRY *ogluClipControl) (GLenum origin, GLenum depth) = nullptr;
    void           (GLAPIENTRY *ogluMemoryBarrier) (GLbitfield barriers) = nullptr;

    void ogluClearDepth(GLclampd d) const;
    
    bool SupportDebug           = false;
    bool SupportSRGB            = false;
    bool SupportClipControl     = false;
    bool SupportImageLoadStore  = false;
    bool SupportComputeShader   = false;
    bool SupportTessShader      = false;
    bool SupportBaseInstance    = false;

    DSAFuncs();
private:
    [[nodiscard]] common::container::FrozenDenseSet<std::string_view> GetExtensions() const;
public:
    common::container::FrozenDenseSet<std::string_view> Extensions;
    [[nodiscard]] std::optional<std::string_view> GetError() const;
};
extern thread_local const DSAFuncs* DSA;

}


