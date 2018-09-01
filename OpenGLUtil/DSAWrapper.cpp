#include "oglRely.h"
#include "DSAWrapper.h"
#include "oglUtil.h"

namespace oglu
{

thread_local const DSAFuncs* DSA = nullptr;

template<typename T>
T DecideFunc(T func)
{
    return func ? func : nullptr;
}
template<typename T, typename... Ts>
T DecideFunc(T func, Ts... funcs)
{
    return func ? func : DecideFunc(funcs...);
}

template<typename T>
T DecideFunc2() { return nullptr; }
template<typename T, typename... Ts>
T DecideFunc2(T func, Ts... funcs)
{
    return func ? func : DecideFunc<T>(funcs...);
}
template<typename T, typename T2, typename... Ts>
T DecideFunc2(T2 func, T realfunc, Ts... funcs)
{
    return func ? realfunc : DecideFunc<T>(funcs...);
}

static void GLAPIENTRY ogluEnableVertexArrayAttrib(GLuint vaobj, GLuint index)
{
    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(index);
    glBindVertexArray(0);
}
static void GLAPIENTRY ogluGetTextureLevelParameterivARB(GLuint texture, GLenum, GLint level, GLenum pname, GLint* params)
{
    glGetTextureLevelParameteriv(texture, level, pname, params);
}
static void GLAPIENTRY ogluTextureParameteriARB(GLuint texture, GLenum, GLenum pname, GLint param)
{
    glTextureParameteri(texture, pname, param);
}
static void GLAPIENTRY ogluTextureImage1D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels)
{
    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, texture);
    glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluTextureImage2D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels)
{
    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, texture);
    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluTextureImage3D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels)
{
    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, texture);
    glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluTextureSubImage1DARB(GLuint texture, GLenum, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels)
{
    glTextureSubImage1D(texture, level, xoffset, width, format, type, pixels);
}
static void GLAPIENTRY ogluTextureSubImage2DARB(GLuint texture, GLenum, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, pixels);
}
static void GLAPIENTRY ogluTextureSubImage3DARB(GLuint texture, GLenum, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels)
{
    glTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
static void GLAPIENTRY ogluTextureSubImage1D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels)
{
    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, texture);
    glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluTextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, texture);
    glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluTextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels)
{
    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, texture);
    glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    glBindTexture(target, 0);
}

void InitDSAFuncs(DSAFuncs& dsa)
{
    dsa.ogluNamedBufferData = DecideFunc(glNamedBufferData, glNamedBufferDataEXT);
    dsa.ogluMapNamedBuffer = DecideFunc(glMapNamedBuffer, glMapNamedBufferEXT);
    dsa.ogluUnmapNamedBuffer = DecideFunc(glUnmapNamedBuffer, glUnmapNamedBufferEXT);
    dsa.ogluEnableVertexArrayAttrib = DecideFunc(glEnableVertexArrayAttrib, glEnableVertexArrayAttribEXT, ogluEnableVertexArrayAttrib);
    dsa.ogluGetTextureLevelParameteriv = DecideFunc2(glGetTextureLevelParameteriv, ogluGetTextureLevelParameterivARB, glGetTextureLevelParameterivEXT);
    dsa.ogluTextureParameteri = DecideFunc2(glTextureParameteri, ogluTextureParameteriARB, glTextureParameteriEXT);
    dsa.ogluTextureImage1D = DecideFunc(glTextureImage1DEXT, ogluTextureImage1D);
    dsa.ogluTextureImage2D = DecideFunc(glTextureImage2DEXT, ogluTextureImage2D);
    dsa.ogluTextureImage3D = DecideFunc(glTextureImage3DEXT, ogluTextureImage3D);
    dsa.ogluTextureSubImage1D = DecideFunc2(glTextureSubImage1D, ogluTextureSubImage1DARB, glTextureSubImage1DEXT, ogluTextureSubImage1D);
    dsa.ogluTextureSubImage2D = DecideFunc2(glTextureSubImage2D, ogluTextureSubImage2DARB, glTextureSubImage2DEXT, ogluTextureSubImage2D);
    dsa.ogluTextureSubImage3D = DecideFunc2(glTextureSubImage3D, ogluTextureSubImage3DARB, glTextureSubImage3DEXT, ogluTextureSubImage3D);
}

}