#pragma once
#include "oglRely.h"

namespace oglu
{

struct DSAFuncs
{
    void  (GLAPIENTRY *ogluNamedBufferData) (GLuint buffer, GLsizeiptr size, const void *data, GLenum usage) = nullptr;
    void* (GLAPIENTRY *ogluMapNamedBuffer) (GLuint buffer, GLenum access) = nullptr;
    GLboolean (GLAPIENTRY *ogluUnmapNamedBuffer) (GLuint buffer) = nullptr;
    void  (GLAPIENTRY *ogluEnableVertexArrayAttrib) (GLuint vaobj, GLuint index) = nullptr;

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

};
extern thread_local const DSAFuncs* DSA;
void InitDSAFuncs(DSAFuncs& dsa);

}