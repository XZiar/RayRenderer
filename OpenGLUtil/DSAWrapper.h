#pragma once
#include "oglRely.h"

namespace oglu
{

class oglIndirectBuffer_;

struct DSAFuncs
{
    template<typename T>
    static void Bind(const T& obj) { obj->bind(); }
    
    void (GLAPIENTRY *ogluGenBuffers) (GLsizei n, GLuint* buffers) = nullptr;
    void (GLAPIENTRY *ogluDeleteBuffers) (GLsizei n, const GLuint* buffers) = nullptr;
    void (GLAPIENTRY *ogluBufferStorage) (GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) = nullptr;
    void (GLAPIENTRY *ogluBindBuffer) (GLenum target, GLuint buffer) = nullptr;
    void (GLAPIENTRY *ogluBufferData) (GLenum target, GLsizeiptr size, const void* data, GLenum usage) = nullptr;
    void (GLAPIENTRY *ogluBufferSubData) (GLenum target, GLintptr offset, GLsizeiptr size, const void* data) = nullptr;
    void* (GLAPIENTRY *ogluMapBuffer) (GLenum target, GLenum access) = nullptr;
    GLboolean (GLAPIENTRY *ogluUnmapBuffer) (GLenum target) = nullptr;
    void (GLAPIENTRY *ogluBindBufferBase) (GLenum target, GLuint index, GLuint buffer) = nullptr;
    void (GLAPIENTRY *ogluBindBufferRange) (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) = nullptr;
    void (GLAPIENTRY *ogluNamedBufferStorage_) (GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) = nullptr;
    void (GLAPIENTRY *ogluNamedBufferData_) (GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) = nullptr;
    void (GLAPIENTRY *ogluNamedBufferSubData_) (GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data) = nullptr;
    void* (GLAPIENTRY *ogluMapNamedBuffer_) (GLuint buffer, GLenum access) = nullptr;
    GLboolean (GLAPIENTRY *ogluUnmapNamedBuffer_) (GLuint buffer) = nullptr;

    void ogluNamedBufferStorage(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) const;
    void ogluNamedBufferData(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) const;
    void ogluNamedBufferSubData(GLenum target, GLuint buffer, GLintptr offset, GLsizeiptr size, const void* data) const;
    [[nodiscard]] void* ogluMapNamedBuffer(GLenum target, GLuint buffer, GLenum access) const;
    GLboolean ogluUnmapNamedBuffer(GLenum target, GLuint buffer) const;
    
    void (GLAPIENTRY *ogluGenVertexArrays) (GLsizei n, const GLuint* arrays) = nullptr;
    void (GLAPIENTRY *ogluDeleteVertexArrays) (GLsizei n, const GLuint* arrays) = nullptr;
    void (GLAPIENTRY *ogluBindVertexArray) (GLuint vaobj) = nullptr;
    void (GLAPIENTRY *ogluEnableVertexAttribArray) (GLuint index) = nullptr;
    void (GLAPIENTRY *ogluEnableVertexArrayAttrib_) (GLuint vaobj, GLuint index) = nullptr;
    void (GLAPIENTRY *ogluVertexAttribIPointer_) (GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRY *ogluVertexAttribLPointer_) (GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRY *ogluVertexAttribPointer_) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) = nullptr;
    void (GLAPIENTRY *ogluVertexAttribDivisor) (GLuint index, GLuint divisor) = nullptr;
    void (GLAPIENTRY *ogluMultiDrawArrays) (GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) = nullptr;
    void (GLAPIENTRY *ogluMultiDrawElements) (GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount) = nullptr;


    void ogluEnableVertexArrayAttrib(GLuint vaobj, GLuint index) const;
    void ogluVertexAttribPointer(GLuint index, GLint size, GLenum type, bool normalized, GLsizei stride, size_t offset) const;
    void ogluVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const;
    void ogluVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const;

    void (GLAPIENTRY *ogluCreateTextures) (GLenum target, GLsizei n, GLuint* textures) = nullptr;
    void (GLAPIENTRY *ogluGetTextureLevelParameteriv) (GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params) = nullptr;
    void (GLAPIENTRY *ogluTextureParameteri) (GLuint texture, GLenum target, GLenum pname, GLint param) = nullptr;
    void (GLAPIENTRY *ogluTextureImage1D) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureImage2D) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureImage3D) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureSubImage1D) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureSubImage2D) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureSubImage3D) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) = nullptr;
    void (GLAPIENTRY *ogluGetTextureImage) (GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, size_t bufSize, void *pixels) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureImage1D) (GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureImage2D) (GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureImage3D) (GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureSubImage1D) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureSubImage2D) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) = nullptr;
    void (GLAPIENTRY *ogluCompressedTextureSubImage3D) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data) = nullptr;
    void (GLAPIENTRY *ogluGetCompressedTextureImage) (GLuint texture, GLenum target, GLint level, size_t bufSize, void *img) = nullptr;
    void (GLAPIENTRY *ogluTextureStorage1D) (GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) = nullptr;
    void (GLAPIENTRY *ogluTextureStorage2D) (GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (GLAPIENTRY *ogluTextureStorage3D) (GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = nullptr;
    void (GLAPIENTRY *ogluBindTextureUnit) (GLuint unit, GLenum target, GLuint texture) = nullptr;
    void (GLAPIENTRY *ogluGenerateTextureMipmap) (GLuint texture, GLenum target) = nullptr;
    void (GLAPIENTRY *ogluTextureBuffer) (GLuint texture, GLenum target, GLenum internalformat, GLuint buffer) = nullptr;

    void (GLAPIENTRY *ogluCreateFramebuffers) (GLsizei n, GLuint *framebuffers) = nullptr;
    void (GLAPIENTRY *ogluFramebufferTexture) (GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void (GLAPIENTRY *ogluFramebufferTextureLayer) (GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) = nullptr;
    void (GLAPIENTRY *ogluFramebufferRenderbuffer) (GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = nullptr;

    void (GLAPIENTRY *ogluMultiDrawArraysIndirect) (GLenum mode, const std::shared_ptr<oglIndirectBuffer_>& indirect, GLint offset, GLsizei primcount) = nullptr;
    void (GLAPIENTRY *ogluMultiDrawElementsIndirect) (GLenum mode, GLenum type, const std::shared_ptr<oglIndirectBuffer_>& indirect, GLint offset, GLsizei primcount) = nullptr;

};
extern thread_local const DSAFuncs* DSA;
void InitDSAFuncs(DSAFuncs& dsa);

}