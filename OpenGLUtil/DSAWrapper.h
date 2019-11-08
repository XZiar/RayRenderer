#pragma once
#include "oglRely.h"

namespace oglu
{

class oglIndirectBuffer_;

struct DSAFuncs
{
    template<typename T>
    static void Bind(const T& obj) { obj->bind(); }
    
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
    
    void (GLAPIENTRY *ogluGenVertexArrays) (GLsizei n, const GLuint* arrays) = nullptr;
    void (GLAPIENTRY *ogluDeleteVertexArrays) (GLsizei n, const GLuint* arrays) = nullptr;
    void (GLAPIENTRY *ogluBindVertexArray) (GLuint vaobj) = nullptr;
    void (GLAPIENTRY *ogluEnableVertexAttribArray) (GLuint index) = nullptr;
    void (GLAPIENTRY *ogluEnableVertexArrayAttrib_) (GLuint vaobj, GLuint index) = nullptr;
    void (GLAPIENTRY *ogluVertexAttribIPointer_) (GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRY *ogluVertexAttribLPointer_) (GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRY *ogluVertexAttribPointer_) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRY *ogluVertexAttribDivisor) (GLuint index, GLuint divisor) = nullptr;

    void ogluEnableVertexArrayAttrib(GLuint vaobj, GLuint index) const;
    void ogluVertexAttribPointer(GLuint index, GLint size, GLenum type, bool normalized, GLsizei stride, size_t offset) const;
    void ogluVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const;
    void ogluVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const;

    // draw related
    
    void (GLAPIENTRY *ogluMultiDrawArrays) (GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) = nullptr;
    void (GLAPIENTRY *ogluMultiDrawElements) (GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount) = nullptr;
    void (GLAPIENTRY *ogluMultiDrawArraysIndirect_) (GLenum mode, const void* indirect, GLsizei primcount, GLsizei stride) = nullptr;
    void (GLAPIENTRY *ogluDrawArraysInstancedBaseInstance_) (GLenum mode, GLint first, GLsizei count, GLsizei primcount, GLuint baseinstance) = nullptr;
    void (GLAPIENTRY *ogluDrawArraysInstanced_) (GLenum mode, GLint first, GLsizei count, GLsizei primcount);
    void (GLAPIENTRY *ogluMultiDrawElementsIndirect_) (GLenum mode, GLenum type, const void* indirect, GLsizei primcount, GLsizei stride) = nullptr;
    void (GLAPIENTRY *ogluDrawElementsInstancedBaseVertexBaseInstance_) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount, GLint basevertex, GLuint baseinstance) = nullptr;
    void (GLAPIENTRY *ogluDrawElementsInstancedBaseInstance_) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount, GLuint baseinstance) = nullptr;
    void (GLAPIENTRY *ogluDrawElementsInstanced_) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount) = nullptr;

    void ogluMultiDrawArraysIndirect(GLenum mode, const oglIndirectBuffer_& indirect, GLint offset, GLsizei primcount) const;
    void ogluMultiDrawElementsIndirect(GLenum mode, GLenum type, const oglIndirectBuffer_& indirect, GLint offset, GLsizei primcount) const;

    // texture related

    void (GLAPIENTRY *ogluActiveTexture) (GLenum texture) = nullptr;
    void (GLAPIENTRY *ogluCreateTextures_) (GLenum target, GLsizei n, GLuint* textures) = nullptr;
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



    void (GLAPIENTRY *ogluCreateFramebuffers) (GLsizei n, GLuint *framebuffers) = nullptr;
    void (GLAPIENTRY *ogluFramebufferTexture) (GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void (GLAPIENTRY *ogluFramebufferTextureLayer) (GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) = nullptr;
    void (GLAPIENTRY *ogluFramebufferRenderbuffer) (GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = nullptr;

};
extern thread_local const DSAFuncs* DSA;
void InitDSAFuncs(DSAFuncs& dsa);

}