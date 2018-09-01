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
    void (GLAPIENTRY *ogluGetTextureLevelParameteriv) (GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params) = nullptr;
    void (GLAPIENTRY *ogluTextureParameteri) (GLuint texture, GLenum target, GLenum pname, GLint param) = nullptr;
    void (GLAPIENTRY *ogluTextureImage1D) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureImage2D) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureImage3D) (GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureSubImage1D) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureSubImage2D) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) = nullptr;
    void (GLAPIENTRY *ogluTextureSubImage3D) (GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) = nullptr;
};
extern thread_local const DSAFuncs* DSA;
void InitDSAFuncs(DSAFuncs& dsa);

}