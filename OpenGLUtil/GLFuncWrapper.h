#pragma once
#include "oglRely.h"
#include "common/ContainerEx.hpp"


#if defined(__gl_h_) || defined(__GL_H__) || defined(_GL_H) || defined(__X_GL_H)
#   error should not include gl.h
#endif


#if COMMON_OS_WIN
#   define APIENTRY __stdcall
#   define WINGDIAPI _declspec(dllimport)
#elif COMMON_OS_UNIX
#   define APIENTRY
#else
#   error "unknown os"
#endif
#include "GL/glcorearb.h"
#include "GL/glext.h"
#if COMMON_OS_WIN
#   undef WINGDIAPI
#endif
#ifndef GLAPIENTRY
#   define GLAPIENTRY APIENTRY
#endif
#define GLAPIENTRYP GLAPIENTRY * 


namespace oglu
{

class oglIndirectBuffer_;
class oglContext_;


/// xxx_ functions are internal and not recommanded to use directly

class CtxFuncs : public ContextCapability
{
    friend struct VAOBinder;
    friend struct FBOBinder;
private:
    mutable common::SpinLocker DataLock;
public:
    void* Target;
    const xcomp::CommonDeviceInfo* XCompDevice = nullptr;
    // buffer related
    GLint MaxUBOUnits = 0;
    void      (GLAPIENTRYP GenBuffers) (GLsizei n, GLuint* buffers) = nullptr;
    void      (GLAPIENTRYP DeleteBuffers) (GLsizei n, const GLuint* buffers) = nullptr;
    void      (GLAPIENTRYP BindBuffer) (GLenum target, GLuint buffer) = nullptr;
    void      (GLAPIENTRYP BindBufferBase) (GLenum target, GLuint index, GLuint buffer) = nullptr;
    void      (GLAPIENTRYP BindBufferRange) (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) = nullptr;
    void      (GLAPIENTRYP NamedBufferStorage_) (GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) = nullptr;
    void      (GLAPIENTRYP BufferStorage_) (GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) = nullptr;
    void      (GLAPIENTRYP NamedBufferData_) (GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) = nullptr;
    void      (GLAPIENTRYP BufferData_) (GLenum target, GLsizeiptr size, const void* data, GLenum usage) = nullptr;
    void      (GLAPIENTRYP NamedBufferSubData_) (GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data) = nullptr;
    void      (GLAPIENTRYP BufferSubData_) (GLenum target, GLintptr offset, GLsizeiptr size, const void* data) = nullptr;
    void*     (GLAPIENTRYP MapNamedBuffer_) (GLuint buffer, GLenum access) = nullptr;
    void*     (GLAPIENTRYP MapBuffer_) (GLenum target, GLenum access) = nullptr;
    GLboolean (GLAPIENTRYP UnmapNamedBuffer_) (GLuint buffer) = nullptr;
    GLboolean (GLAPIENTRYP UnmapBuffer_) (GLenum target) = nullptr;
    
    void NamedBufferStorage(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) const;
    void NamedBufferData(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) const;
    void NamedBufferSubData(GLenum target, GLuint buffer, GLintptr offset, GLsizeiptr size, const void* data) const;
    [[nodiscard]] void* MapNamedBuffer(GLenum target, GLuint buffer, GLenum access) const;
    GLboolean UnmapNamedBuffer(GLenum target, GLuint buffer) const;
    
    // vao related
    mutable GLuint VAO = 0;
    void (GLAPIENTRYP GenVertexArrays) (GLsizei n, GLuint* arrays) = nullptr;
    void (GLAPIENTRYP DeleteVertexArrays) (GLsizei n, const GLuint* arrays) = nullptr;
    void (GLAPIENTRYP BindVertexArray_) (GLuint vaobj) = nullptr;
    void (GLAPIENTRYP EnableVertexAttribArray_) (GLuint index) = nullptr;
    void (GLAPIENTRYP EnableVertexArrayAttrib_) (GLuint vaobj, GLuint index) = nullptr;
    void (GLAPIENTRYP VertexAttribIPointer_) (GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRYP VertexAttribLPointer_) (GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRYP VertexAttribPointer_) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRYP VertexAttribDivisor) (GLuint index, GLuint divisor) = nullptr;

    void RefreshVAOState() const;
    void BindVertexArray(GLuint vaobj) const;
    void EnableVertexArrayAttrib(GLuint vaobj, GLuint index) const;
    void VertexAttribPointer(GLuint index, GLint size, GLenum type, bool normalized, GLsizei stride, size_t offset) const;
    void VertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const;
    void VertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const;

    // draw related
    void (GLAPIENTRYP DrawArrays) (GLenum mode, GLint first, GLsizei count) = nullptr;
    void (GLAPIENTRYP DrawElements) (GLenum mode, GLsizei count, GLenum type, const void* indices) = nullptr;
    void (GLAPIENTRYP MultiDrawArrays) (GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) = nullptr;
    void (GLAPIENTRYP MultiDrawElements) (GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount) = nullptr;
    void (GLAPIENTRYP MultiDrawArraysIndirect_) (GLenum mode, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;
    void (GLAPIENTRYP DrawArraysInstancedBaseInstance_) (GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) = nullptr;
    void (GLAPIENTRYP DrawArraysInstanced_) (GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
    void (GLAPIENTRYP MultiDrawElementsIndirect_) (GLenum mode, GLenum type, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;
    void (GLAPIENTRYP DrawElementsInstancedBaseVertexBaseInstance_) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) = nullptr;
    void (GLAPIENTRYP DrawElementsInstancedBaseInstance_) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLuint baseinstance) = nullptr;
    void (GLAPIENTRYP DrawElementsInstanced_) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) = nullptr;

    void DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, uint32_t instancecount, uint32_t baseinstance) const;
    void DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, uint32_t instancecount, uint32_t baseinstance) const;
    void MultiDrawArraysIndirect(GLenum mode, const oglIndirectBuffer_& indirect, GLint offset, GLsizei drawcount) const;
    void MultiDrawElementsIndirect(GLenum mode, GLenum type, const oglIndirectBuffer_& indirect, GLint offset, GLsizei drawcount) const;

    // texture related
    GLint MaxTextureUnits = 0;
    GLint MaxImageUnits = 0;
    
    void     (GLAPIENTRYP GenTextures) (GLsizei n, GLuint* textures) = nullptr;
    void     (GLAPIENTRYP CreateTextures_) (GLenum target, GLsizei n, GLuint* textures) = nullptr;
    void     (GLAPIENTRYP DeleteTextures) (GLsizei n, const GLuint* textures) = nullptr;
    void     (GLAPIENTRYP ActiveTexture) (GLenum texture) = nullptr;
    void     (GLAPIENTRYP BindTexture) (GLenum target, GLuint texture) = nullptr;
    void     (GLAPIENTRYP BindTextureUnit_) (GLuint unit, GLuint texture) = nullptr;
    void     (GLAPIENTRYP BindMultiTextureEXT_) (GLuint unit, GLenum target, GLuint texture) = nullptr;
    void     (GLAPIENTRYP BindImageTexture) (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) = nullptr;
    void     (GLAPIENTRYP TextureView) (GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) = nullptr;
    void     (GLAPIENTRYP TextureBuffer_) (GLuint texture, GLenum internalformat, GLuint buffer) = nullptr;
    void     (GLAPIENTRYP TextureBufferEXT_) (GLuint texture, GLenum target, GLenum internalformat, GLuint buffer) = nullptr;
    void     (GLAPIENTRYP TexBuffer_) (GLenum target, GLenum internalformat, GLuint buffer) = nullptr;
    void     (GLAPIENTRYP GetTexImage) (GLenum target, GLint level, GLenum format, GLenum type, void* pixels) = nullptr;
    void     (GLAPIENTRYP GetTexLevelParameteriv) (GLenum target, GLint level, GLenum pname, GLint* params) = nullptr;
    void     (GLAPIENTRYP GenerateTextureMipmap_) (GLuint texture) = nullptr;
    void     (GLAPIENTRYP GenerateTextureMipmapEXT_) (GLuint texture, GLenum target) = nullptr;
    void     (GLAPIENTRYP GenerateMipmap_) (GLenum target) = nullptr;
    GLuint64 (GLAPIENTRYP GetTextureHandle) (GLuint texture) = nullptr;
    void     (GLAPIENTRYP MakeTextureHandleResident) (GLuint64 handle) = nullptr;
    void     (GLAPIENTRYP MakeTextureHandleNonResident) (GLuint64 handle) = nullptr;
    GLuint64 (GLAPIENTRYP GetImageHandle) (GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format) = nullptr;
    void     (GLAPIENTRYP MakeImageHandleResident) (GLuint64 handle, GLenum access) = nullptr;
    void     (GLAPIENTRYP MakeImageHandleNonResident) (GLuint64 handle) = nullptr;
    void     (GLAPIENTRYP TexParameteri) (GLenum target, GLenum pname, GLint param) = nullptr;
    void     (GLAPIENTRYP TextureParameteri_) (GLuint texture, GLenum pname, GLint param) = nullptr;
    void     (GLAPIENTRYP TextureParameteriEXT_) (GLuint texture, GLenum target, GLenum pname, GLint param) = nullptr; 
    void     (GLAPIENTRYP TexImage1D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TexImage3D_) (GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TexSubImage1D_) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TexSubImage2D_) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TexSubImage3D_) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TextureSubImage1D_) (GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TextureSubImage2D_) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TextureSubImage3D_) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TextureSubImage1DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TextureSubImage2DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TextureSubImage3DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TextureImage1DEXT_) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TextureImage2DEXT_) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP TextureImage3DEXT_) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void     (GLAPIENTRYP CompressedTextureSubImage1D_) (GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTextureSubImage2D_) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTextureSubImage3D_) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTextureSubImage1DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTextureSubImage2DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTextureSubImage3DEXT_) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTexSubImage1D_) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTexSubImage2D_) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTexSubImage3D_) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTextureImage1DEXT_) (GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTextureImage2DEXT_) (GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTextureImage3DEXT_) (GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTexImage1D_) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTexImage2D_) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CompressedTexImage3D_) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void     (GLAPIENTRYP CopyImageSubData) (GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) = nullptr;
    void     (GLAPIENTRYP TextureStorage1D_) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) = nullptr;
    void     (GLAPIENTRYP TextureStorage2D_) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void     (GLAPIENTRYP TextureStorage3D_) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = nullptr;
    void     (GLAPIENTRYP TextureStorage1DEXT_) (GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) = nullptr;
    void     (GLAPIENTRYP TextureStorage2DEXT_) (GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void     (GLAPIENTRYP TextureStorage3DEXT_) (GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = nullptr;
    void     (GLAPIENTRYP TexStorage1D_) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) = nullptr;
    void     (GLAPIENTRYP TexStorage2D_) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void     (GLAPIENTRYP TexStorage3D_) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = nullptr;
    void     (GLAPIENTRYP ClearTexImage) (GLuint texture, GLint level, GLenum format, GLenum type, const void* data) = nullptr;
    void     (GLAPIENTRYP ClearTexSubImage) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* data) = nullptr;
    void     (GLAPIENTRYP GetTextureLevelParameteriv_) (GLuint texture, GLint level, GLenum pname, GLint* params) = nullptr;
    void     (GLAPIENTRYP GetTextureLevelParameterivEXT_) (GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params) = nullptr;
    void     (GLAPIENTRYP GetTextureImage_) (GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) = nullptr;
    void     (GLAPIENTRYP GetTextureImageEXT_) (GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void *pixels) = nullptr;
    void     (GLAPIENTRYP GetCompressedTextureImage_) (GLuint texture, GLint level, GLsizei bufSize, void *img) = nullptr;
    void     (GLAPIENTRYP GetCompressedTextureImageEXT_) (GLuint texture, GLenum target, GLint level, void *img) = nullptr;
    void     (GLAPIENTRYP GetCompressedTexImage_) (GLenum target, GLint lod, void *img) = nullptr;

    void CreateTextures(GLenum target, GLsizei n, GLuint* textures) const;
    void BindTextureUnit(GLuint unit, GLuint texture, GLenum target) const;
    void TextureBuffer(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer) const;
    void GenerateTextureMipmap(GLuint texture, GLenum target) const;
    void TextureParameteri(GLuint texture, GLenum target, GLenum pname, GLint param) const;
    void TextureSubImage1D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) const;
    void TextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) const;
    void TextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) const;
    void TextureImage1D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels) const;
    void TextureImage2D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) const;
    void TextureImage3D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) const;
    void CompressedTextureSubImage1D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) const;
    void CompressedTextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) const;
    void CompressedTextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) const;
    void CompressedTextureImage1D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void* data) const;
    void CompressedTextureImage2D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) const;
    void CompressedTextureImage3D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) const;
    void TextureStorage1D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) const;
    void TextureStorage2D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) const;
    void TextureStorage3D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) const;
    void GetTextureLevelParameteriv(GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params) const;
    void GetTextureImage(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, size_t bufSize, void* pixels) const;
    void GetCompressedTextureImage(GLuint texture, GLenum target, GLint level, size_t bufSize, void* img) const;

    // rbo related
    void (GLAPIENTRYP GenRenderbuffers_) (GLsizei n, GLuint* renderbuffers) = nullptr;
    void (GLAPIENTRYP CreateRenderbuffers_) (GLsizei n, GLuint* renderbuffers) = nullptr;
    void (GLAPIENTRYP DeleteRenderbuffers) (GLsizei n, const GLuint* renderbuffers) = nullptr;
    void (GLAPIENTRYP BindRenderbuffer_) (GLenum target, GLuint renderbuffer) = nullptr;
    void (GLAPIENTRYP NamedRenderbufferStorage_) (GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRYP RenderbufferStorage_) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRYP NamedRenderbufferStorageMultisample_) (GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRYP RenderbufferStorageMultisample_) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRYP NamedRenderbufferStorageMultisampleCoverageEXT_) (GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRYP RenderbufferStorageMultisampleCoverageNV_) (GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRYP GetRenderbufferParameteriv_) (GLenum target, GLenum pname, GLint* params) = nullptr;

    void CreateRenderbuffers(GLsizei n, GLuint* renderbuffers) const;
    void NamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) const;
    void NamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) const;
    void NamedRenderbufferStorageMultisampleCoverage(GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) const;

    // fbo related
    mutable GLuint ReadFBO = 0, DrawFBO = 0;
    GLint MaxColorAttachment = 0;
    GLint MaxDrawBuffers = 0;
    void   (GLAPIENTRYP GenFramebuffers_) (GLsizei n, GLuint* framebuffers) = nullptr;
    void   (GLAPIENTRYP CreateFramebuffers_) (GLsizei n, GLuint *framebuffers) = nullptr;
    void   (GLAPIENTRYP DeleteFramebuffers) (GLsizei n, const GLuint* framebuffers) = nullptr;
    void   (GLAPIENTRYP BindFramebuffer_) (GLenum target, GLuint framebuffer) = nullptr;
    void   (GLAPIENTRYP BlitNamedFramebuffer_) (GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) = nullptr;
    void   (GLAPIENTRYP BlitFramebuffer_) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) = nullptr;
    void   (GLAPIENTRYP DrawBuffers) (GLsizei n, const GLenum* bufs) = nullptr;
    void   (GLAPIENTRYP InvalidateNamedFramebufferData_) (GLuint framebuffer, GLsizei numAttachments, const GLenum* attachments) = nullptr;
    void   (GLAPIENTRYP InvalidateFramebuffer_) (GLenum target, GLsizei numAttachments, const GLenum* attachments) = nullptr;
    void   (GLAPIENTRYP DiscardFramebufferEXT_) (GLenum target, GLsizei numAttachments, const GLenum* attachments) = nullptr;
    void   (GLAPIENTRYP NamedFramebufferRenderbuffer_) (GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = nullptr;
    void   (GLAPIENTRYP FramebufferRenderbuffer_) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = nullptr;
    void   (GLAPIENTRYP NamedFramebufferTexture1DEXT_) (GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void   (GLAPIENTRYP FramebufferTexture1D_) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void   (GLAPIENTRYP NamedFramebufferTexture2DEXT_) (GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void   (GLAPIENTRYP FramebufferTexture2D_) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void   (GLAPIENTRYP NamedFramebufferTexture3DEXT_) (GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) = nullptr;
    void   (GLAPIENTRYP FramebufferTexture3D_) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer) = nullptr;
    void   (GLAPIENTRYP NamedFramebufferTexture_) (GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
    void   (GLAPIENTRYP FramebufferTexture_) (GLenum target, GLenum attachment, GLuint texture, GLint level);
    void   (GLAPIENTRYP NamedFramebufferTextureLayer_) (GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) = nullptr;
    void   (GLAPIENTRYP FramebufferTextureLayer_) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) = nullptr;
    GLenum (GLAPIENTRYP CheckNamedFramebufferStatus_) (GLuint framebuffer, GLenum target) = nullptr;
    GLenum (GLAPIENTRYP CheckFramebufferStatus_) (GLenum target) = nullptr;
    void   (GLAPIENTRYP GetNamedFramebufferAttachmentParameteriv_) (GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRYP GetFramebufferAttachmentParameteriv_) (GLenum target, GLenum attachment, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRYP Clear) (GLbitfield mask) = nullptr;
    void   (GLAPIENTRYP ClearColor) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = nullptr;
    void   (GLAPIENTRYP ClearDepth_) (GLclampd d) = nullptr;
    void   (GLAPIENTRYP ClearDepthf_) (GLclampf f) = nullptr;
    void   (GLAPIENTRYP ClearStencil) (GLint s) = nullptr;
    void   (GLAPIENTRYP ClearNamedFramebufferiv_) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ClearBufferiv_) (GLenum buffer, GLint drawbuffer, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ClearNamedFramebufferuiv_) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value) = nullptr;
    void   (GLAPIENTRYP ClearBufferuiv_) (GLenum buffer, GLint drawbuffer, const GLuint* value) = nullptr;
    void   (GLAPIENTRYP ClearNamedFramebufferfv_) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ClearBufferfv_) (GLenum buffer, GLint drawbuffer, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ClearNamedFramebufferfi_) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) = nullptr;
    void   (GLAPIENTRYP ClearBufferfi_) (GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) = nullptr;

    void RefreshFBOState() const;
    void CreateFramebuffers(GLsizei n, GLuint* framebuffers) const;
    void BindFramebuffer(GLenum target, GLuint framebuffer) const;
    void BlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) const;
    void InvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum* attachments) const;
    void NamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) const;
    void NamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) const;
    void NamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer) const;
    [[nodiscard]] GLenum CheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) const;
    void GetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params) const;
    void ClearDepth(GLclampd d) const;
    void ClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value) const;
    void ClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value) const;
    void ClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value) const;
    void ClearNamedFramebufferDepthStencil(GLuint framebuffer, GLfloat depth, GLint stencil) const;

    // shader related
    GLuint (GLAPIENTRYP CreateShader) (GLenum type) = nullptr;
    void   (GLAPIENTRYP DeleteShader) (GLuint shader) = nullptr;
    void   (GLAPIENTRYP ShaderSource) (GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) = nullptr;
    void   (GLAPIENTRYP CompileShader) (GLuint shader) = nullptr;
    void   (GLAPIENTRYP GetShaderInfoLog) (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) = nullptr;
    void   (GLAPIENTRYP GetShaderSource) (GLuint obj, GLsizei maxLength, GLsizei* length, GLchar* source) = nullptr;
    void   (GLAPIENTRYP GetShaderiv) (GLuint shader, GLenum pname, GLint* param) = nullptr;
    
    //program related
    GLuint (GLAPIENTRYP CreateProgram) () = nullptr;
    void   (GLAPIENTRYP DeleteProgram) (GLuint program) = nullptr;
    void   (GLAPIENTRYP AttachShader) (GLuint program, GLuint shader) = nullptr;
    void   (GLAPIENTRYP DetachShader) (GLuint program, GLuint shader) = nullptr;
    void   (GLAPIENTRYP LinkProgram) (GLuint program) = nullptr;
    void   (GLAPIENTRYP UseProgram) (GLuint program) = nullptr;
    void   (GLAPIENTRYP DispatchCompute) (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) = nullptr;
    void   (GLAPIENTRYP DispatchComputeIndirect) (GLintptr indirect) = nullptr;
    
    void   (GLAPIENTRYP Uniform1f_) (GLint location, GLfloat v0) = nullptr;
    void   (GLAPIENTRYP Uniform1fv_) (GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP Uniform1i_) (GLint location, GLint v0) = nullptr;
    void   (GLAPIENTRYP Uniform1iv_) (GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP Uniform2f_) (GLint location, GLfloat v0, GLfloat v1) = nullptr;
    void   (GLAPIENTRYP Uniform2fv_) (GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP Uniform2i_) (GLint location, GLint v0, GLint v1) = nullptr;
    void   (GLAPIENTRYP Uniform2iv_) (GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP Uniform3f_) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2) = nullptr;
    void   (GLAPIENTRYP Uniform3fv_) (GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP Uniform3i_) (GLint location, GLint v0, GLint v1, GLint v2) = nullptr;
    void   (GLAPIENTRYP Uniform3iv_) (GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP Uniform4f_) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) = nullptr;
    void   (GLAPIENTRYP Uniform4fv_) (GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP Uniform4i_) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3) = nullptr;
    void   (GLAPIENTRYP Uniform4iv_) (GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP UniformMatrix2fv_) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP UniformMatrix3fv_) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP UniformMatrix4fv_) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform1d) (GLuint program, GLint location, GLdouble x) = nullptr;
    void   (GLAPIENTRYP ProgramUniform1dv) (GLuint program, GLint location, GLsizei count, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform1f) (GLuint program, GLint location, GLfloat x) = nullptr;
    void   (GLAPIENTRYP ProgramUniform1fv) (GLuint program, GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform1i) (GLuint program, GLint location, GLint x) = nullptr;
    void   (GLAPIENTRYP ProgramUniform1iv) (GLuint program, GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform1ui) (GLuint program, GLint location, GLuint x) = nullptr;
    void   (GLAPIENTRYP ProgramUniform1uiv) (GLuint program, GLint location, GLsizei count, const GLuint* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform2d) (GLuint program, GLint location, GLdouble x, GLdouble y) = nullptr;
    void   (GLAPIENTRYP ProgramUniform2dv) (GLuint program, GLint location, GLsizei count, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform2f) (GLuint program, GLint location, GLfloat x, GLfloat y) = nullptr;
    void   (GLAPIENTRYP ProgramUniform2fv) (GLuint program, GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform2i) (GLuint program, GLint location, GLint x, GLint y) = nullptr;
    void   (GLAPIENTRYP ProgramUniform2iv) (GLuint program, GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform2ui) (GLuint program, GLint location, GLuint x, GLuint y) = nullptr;
    void   (GLAPIENTRYP ProgramUniform2uiv) (GLuint program, GLint location, GLsizei count, const GLuint* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform3d) (GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z) = nullptr;
    void   (GLAPIENTRYP ProgramUniform3dv) (GLuint program, GLint location, GLsizei count, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform3f) (GLuint program, GLint location, GLfloat x, GLfloat y, GLfloat z) = nullptr;
    void   (GLAPIENTRYP ProgramUniform3fv) (GLuint program, GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform3i) (GLuint program, GLint location, GLint x, GLint y, GLint z) = nullptr;
    void   (GLAPIENTRYP ProgramUniform3iv) (GLuint program, GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform3ui) (GLuint program, GLint location, GLuint x, GLuint y, GLuint z) = nullptr;
    void   (GLAPIENTRYP ProgramUniform3uiv) (GLuint program, GLint location, GLsizei count, const GLuint* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform4d) (GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) = nullptr;
    void   (GLAPIENTRYP ProgramUniform4dv) (GLuint program, GLint location, GLsizei count, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform4f) (GLuint program, GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) = nullptr;
    void   (GLAPIENTRYP ProgramUniform4fv) (GLuint program, GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform4i) (GLuint program, GLint location, GLint x, GLint y, GLint z, GLint w) = nullptr;
    void   (GLAPIENTRYP ProgramUniform4iv) (GLuint program, GLint location, GLsizei count, const GLint* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniform4ui) (GLuint program, GLint location, GLuint x, GLuint y, GLuint z, GLuint w) = nullptr;
    void   (GLAPIENTRYP ProgramUniform4uiv) (GLuint program, GLint location, GLsizei count, const GLuint* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix2dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix2fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix2x3dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix2x3fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix2x4dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix2x4fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix3dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix3fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix3x2dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix3x2fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix3x4dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix3x4fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix4dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix4fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix4x2dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix4x2fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix4x3dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformMatrix4x3fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void   (GLAPIENTRYP ProgramUniformHandleui64) (GLuint program, GLint location, GLuint64 value) = nullptr;

    void   (GLAPIENTRYP GetUniformfv) (GLuint program, GLint location, GLfloat* params) = nullptr;
    void   (GLAPIENTRYP GetUniformiv) (GLuint program, GLint location, GLint* params) = nullptr;
    void   (GLAPIENTRYP GetUniformuiv) (GLuint program, GLint location, GLuint* params) = nullptr;
    void   (GLAPIENTRYP GetProgramInfoLog) (GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) = nullptr;
    void   (GLAPIENTRYP GetProgramiv) (GLuint program, GLenum pname, GLint* param) = nullptr;
    void   (GLAPIENTRYP GetProgramInterfaceiv) (GLuint program, GLenum programInterface, GLenum pname, GLint* params) = nullptr;
    GLuint (GLAPIENTRYP GetProgramResourceIndex) (GLuint program, GLenum programInterface, const GLchar* name) = nullptr;
    GLint  (GLAPIENTRYP GetProgramResourceLocation) (GLuint program, GLenum programInterface, const GLchar* name) = nullptr;
    GLint  (GLAPIENTRYP GetProgramResourceLocationIndex) (GLuint program, GLenum programInterface, const GLchar* name) = nullptr;
    void   (GLAPIENTRYP GetProgramResourceName) (GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar* name) = nullptr;
    void   (GLAPIENTRYP GetProgramResourceiv) (GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei bufSize, GLsizei* length, GLint* params) = nullptr;
    void   (GLAPIENTRYP GetActiveSubroutineName) (GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei* length, GLchar* name) = nullptr;
    void   (GLAPIENTRYP GetActiveSubroutineUniformName) (GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei* length, GLchar* name) = nullptr;
    void   (GLAPIENTRYP GetActiveSubroutineUniformiv) (GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint* values) = nullptr;
    void   (GLAPIENTRYP GetProgramStageiv) (GLuint program, GLenum shadertype, GLenum pname, GLint* values) = nullptr;
    GLuint (GLAPIENTRYP GetSubroutineIndex) (GLuint program, GLenum shadertype, const GLchar* name) = nullptr;
    GLint  (GLAPIENTRYP GetSubroutineUniformLocation) (GLuint program, GLenum shadertype, const GLchar* name) = nullptr;
    void   (GLAPIENTRYP GetUniformSubroutineuiv) (GLenum shadertype, GLint location, GLuint* params) = nullptr;
    void   (GLAPIENTRYP UniformSubroutinesuiv) (GLenum shadertype, GLsizei count, const GLuint* indices) = nullptr;
    void   (GLAPIENTRYP GetActiveUniformBlockName) (GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName) = nullptr;
    void   (GLAPIENTRYP GetActiveUniformBlockiv) (GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRYP GetActiveUniformName) (GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformName) = nullptr;
    void   (GLAPIENTRYP GetActiveUniformsiv) (GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRYP GetIntegeri_v) (GLenum target, GLuint index, GLint* data) = nullptr;
    GLuint (GLAPIENTRYP GetUniformBlockIndex) (GLuint program, const GLchar* uniformBlockName) = nullptr;
    void   (GLAPIENTRYP GetUniformIndices) (GLuint program, GLsizei uniformCount, const GLchar* const* uniformNames, GLuint* uniformIndices) = nullptr;
    void   (GLAPIENTRYP UniformBlockBinding) (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) = nullptr;

    // query related
    void   (GLAPIENTRYP GenQueries) (GLsizei n, GLuint* ids) = nullptr;
    void   (GLAPIENTRYP DeleteQueries) (GLsizei n, const GLuint* ids) = nullptr;
    void   (GLAPIENTRYP BeginQuery) (GLenum target, GLuint id) = nullptr;
    void   (GLAPIENTRYP QueryCounter) (GLuint id, GLenum target) = nullptr;
    void   (GLAPIENTRYP GetQueryObjectiv) (GLuint id, GLenum pname, GLint* params) = nullptr;
    void   (GLAPIENTRYP GetQueryObjectuiv) (GLuint id, GLenum pname, GLuint* params) = nullptr;
    void   (GLAPIENTRYP GetQueryObjecti64v) (GLuint id, GLenum pname, GLint64* params) = nullptr;
    void   (GLAPIENTRYP GetQueryObjectui64v) (GLuint id, GLenum pname, GLuint64* params) = nullptr;
    void   (GLAPIENTRYP GetQueryiv) (GLenum target, GLenum pname, GLint* params) = nullptr;
    GLsync (GLAPIENTRYP FenceSync) (GLenum condition, GLbitfield flags) = nullptr;
    void   (GLAPIENTRYP DeleteSync) (GLsync GLsync) = nullptr;
    GLenum (GLAPIENTRYP ClientWaitSync) (GLsync GLsync, GLbitfield flags, GLuint64 timeout) = nullptr;
    void   (GLAPIENTRYP WaitSync) (GLsync GLsync, GLbitfield flags, GLuint64 timeout) = nullptr;
    void   (GLAPIENTRYP GetInteger64v) (GLenum pname, GLint64* params) = nullptr;
    void   (GLAPIENTRYP GetSynciv) (GLsync GLsync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values) = nullptr;

    // debug
    GLint MaxLabelLen = 0;
    GLsizei MaxMessageLen = 0;
    using DebugCallback    = void (GLAPIENTRYP)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
    using DebugCallbackAMD = void (GLAPIENTRYP)(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, void* userParam);
    void (GLAPIENTRYP DebugMessageCallback) (DebugCallback callback, const void* userParam) = nullptr;
    void (GLAPIENTRYP DebugMessageCallbackAMD) (DebugCallbackAMD callback, void* userParam) = nullptr;
    void (GLAPIENTRYP ObjectLabel_) (GLenum identifier, GLuint name, GLsizei length, const GLchar* label) = nullptr;
    void (GLAPIENTRYP LabelObjectEXT_) (GLenum type, GLuint object, GLsizei length, const GLchar* label) = nullptr;
    void (GLAPIENTRYP ObjectPtrLabel_) (const void* ptr, GLsizei length, const GLchar* label) = nullptr;
    void (GLAPIENTRYP PushDebugGroup_) (GLenum source, GLuint id, GLsizei length, const GLchar* message) = nullptr;
    void (GLAPIENTRYP PopDebugGroup_) () = nullptr;
    void (GLAPIENTRYP PushGroupMarkerEXT_) (GLsizei length, const GLchar* marker) = nullptr;
    void (GLAPIENTRYP PopGroupMarkerEXT_) () = nullptr;
    void (GLAPIENTRYP DebugMessageInsert_) (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* buf) = nullptr;
    void (GLAPIENTRYP DebugMessageInsertAMD) (GLenum category, GLenum severity, GLuint id, GLsizei length, const GLchar* buf) = nullptr;
    void (GLAPIENTRYP InsertEventMarkerEXT) (GLsizei length, const GLchar* marker) = nullptr;

    void SetObjectLabel(GLenum identifier, GLuint id, std::u16string_view name) const;
    void SetObjectLabel(GLsync sync, std::u16string_view name) const;
    void PushDebugGroup(GLenum source, GLuint id, std::u16string_view message) const;
    void PopDebugGroup() const;
    void InsertDebugMarker(GLuint id, std::u16string_view name) const;

    // others
    GLenum         (GLAPIENTRYP GetError_) () = nullptr;
    void           (GLAPIENTRYP GetFloatv) (GLenum pname, GLfloat* params) = nullptr;
    void           (GLAPIENTRYP GetIntegerv) (GLenum pname, GLint* params) = nullptr;
    void           (GLAPIENTRYP GetUnsignedBytevEXT) (GLenum pname, GLubyte* params) = nullptr;
    void           (GLAPIENTRYP GetUnsignedBytei_vEXT) (GLenum target, GLuint index, GLubyte* data) = nullptr;
    const GLubyte* (GLAPIENTRYP GetString) (GLenum name) = nullptr;
    const GLubyte* (GLAPIENTRYP GetStringi) (GLenum name, GLuint index) = nullptr;
    GLboolean      (GLAPIENTRYP IsEnabled) (GLenum cap) = nullptr;
    void           (GLAPIENTRYP Enable) (GLenum cap) = nullptr;
    void           (GLAPIENTRYP Disable) (GLenum cap) = nullptr;
    void           (GLAPIENTRYP Finish) () = nullptr;
    void           (GLAPIENTRYP Flush) () = nullptr;
    void           (GLAPIENTRYP DepthFunc) (GLenum func) = nullptr;
    void           (GLAPIENTRYP CullFace) (GLenum mode) = nullptr;
    void           (GLAPIENTRYP FrontFace) (GLenum mode) = nullptr;
    void           (GLAPIENTRYP Viewport) (GLint x, GLint y, GLsizei width, GLsizei height) = nullptr;
    void           (GLAPIENTRYP ViewportArrayv) (GLuint first, GLsizei count, const GLfloat* v) = nullptr;
    void           (GLAPIENTRYP ViewportIndexedf) (GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h) = nullptr;
    void           (GLAPIENTRYP ViewportIndexedfv) (GLuint index, const GLfloat* v) = nullptr;
    void           (GLAPIENTRYP ClipControl) (GLenum origin, GLenum depth) = nullptr;
    void           (GLAPIENTRYP MemoryBarrier) (GLbitfield barriers) = nullptr;

    CtxFuncs(void*, const GLHost&, std::pair<bool, bool>);
private:
    [[nodiscard]] common::container::FrozenDenseSet<std::string_view> GetExtensions() const;
public:
    [[nodiscard]] std::optional<std::string_view> GetError() const;
    [[nodiscard]] std::optional<std::array<std::byte, 8>> GetLUID() const noexcept;
    [[nodiscard]] std::optional<std::array<std::byte, 16>> GetUUID() const noexcept;
    [[nodiscard]] static const CtxFuncs* PrepareCurrent(const GLHost& host, void* hRC, std::pair<bool, bool> shouldPrint);
};
extern thread_local const CtxFuncs* CtxFunc;

//extern std::atomic_uint32_t LatestVersion;


}


