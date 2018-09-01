#include "oglRely.h"
#include "DSAWrapper.h"
#include "oglUtil.h"

namespace oglu
{

thread_local const DSAFuncs* DSA = nullptr;

template<typename T>
T DecideFunc() { return nullptr; }
template<typename T, typename... Ts>
T DecideFunc(const T& func, Ts... funcs)
{
    if constexpr (sizeof...(Ts) > 0)
        return func ? func : DecideFunc<T>(funcs...);
    else
        return func ? func : nullptr;
}
template<typename T, typename T2, typename... Ts>
T DecideFunc(const std::pair<T2, T>& func, Ts... funcs)
{
    if constexpr (sizeof...(Ts) > 0)
        return func.first ? func.second : DecideFunc<T>(funcs...);
    else
        return func.first ? func.second : nullptr;
}

static void GLAPIENTRY ogluEnableVertexArrayAttrib(GLuint vaobj, GLuint index)
{
    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(index);
    glBindVertexArray(0);
}

static void GLAPIENTRY ogluCreateTextures(GLenum, GLsizei n, GLuint* textures)
{
    glGenTextures(n, textures); // leave type initialize to later steps
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
    glBindTexture(target, texture);
    glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluTextureImage2D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels)
{
    glBindTexture(target, texture);
    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluTextureImage3D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels)
{
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
    glBindTexture(target, texture);
    glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluTextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    glBindTexture(target, texture);
    glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluTextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels)
{
    glBindTexture(target, texture);
    glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluGetTextureImageARB(GLuint texture, GLenum, GLint level, GLenum format, GLenum type, size_t bufSize, void *pixels)
{
    glGetTextureImage(texture, level, format, type, bufSize > INT32_MAX ? INT32_MAX : (GLsizei)bufSize, pixels);
}
static void GLAPIENTRY ogluGetTextureImageEXT(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, size_t, void *pixels)
{
    glGetTextureImageEXT(texture, target, level, format, type, pixels);
}
static void GLAPIENTRY ogluCompressedTextureImage1D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data)
{
    glBindTexture(target, texture);
    glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluCompressedTextureImage2D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data)
{
    glBindTexture(target, texture);
    glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluCompressedTextureImage3D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data)
{
    glBindTexture(target, texture);
    glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluCompressedTextureSubImage1DARB(GLuint texture, GLenum, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data)
{
    glCompressedTextureSubImage1D(texture, level, xoffset, width, format, imageSize, data);
}
static void GLAPIENTRY ogluCompressedTextureSubImage2DARB(GLuint texture, GLenum, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data)
{
    glCompressedTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, imageSize, data);
}
static void GLAPIENTRY ogluCompressedTextureSubImage3DARB(GLuint texture, GLenum, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data)
{
    glCompressedTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}
static void GLAPIENTRY ogluCompressedTextureSubImage1D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data)
{
    glBindTexture(target, texture);
    glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluCompressedTextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data)
{
    glBindTexture(target, texture);
    glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluCompressedTextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data)
{
    glBindTexture(target, texture);
    glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluGetCompressedTextureImageARB(GLuint texture, GLenum, GLint level, size_t bufSize, void *img)
{
    glGetCompressedTextureImage(texture, level, bufSize > INT32_MAX ? INT32_MAX : (GLsizei)bufSize, img);
}
static void GLAPIENTRY ogluGetCompressedTextureImageEXT(GLuint texture, GLenum target, GLint level, size_t, void *img)
{
    glGetCompressedTextureImageEXT(texture, target, level, img);
}
static void GLAPIENTRY ogluTextureStorage1DARB(GLuint texture, GLenum, GLsizei levels, GLenum internalformat, GLsizei width)
{
    glTextureStorage1D(texture, levels, internalformat, width);
}
static void GLAPIENTRY ogluTextureStorage2DARB(GLuint texture, GLenum, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    glTextureStorage2D(texture, levels, internalformat, width, height);
}
static void GLAPIENTRY ogluTextureStorage3DARB(GLuint texture, GLenum, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    glTextureStorage3D(texture, levels, internalformat, width, height, depth);
}
static void GLAPIENTRY ogluTextureStorage1D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width)
{
    glBindTexture(target, texture);
    glTexStorage1D(target, levels, internalformat, width);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluTextureStorage2D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    glBindTexture(target, texture);
    glTexStorage2D(target, levels, internalformat, width, height);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluTextureStorage3D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    glBindTexture(target, texture);
    glTexStorage3D(target, levels, internalformat, width, height, depth);
    glBindTexture(target, 0);
}
static void GLAPIENTRY ogluBindTextureUnitARB(GLuint unit, GLenum, GLuint texture)
{
    glBindTextureUnit(unit, texture);
}
static void GLAPIENTRY ogluBindTextureUnitEXT(GLuint unit, GLenum target, GLuint texture)
{
    glBindMultiTextureEXT(GL_TEXTURE0 + unit, target, texture);
}
static void GLAPIENTRY ogluBindTextureUnit(GLuint unit, GLenum target, GLuint texture)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(target, texture);
}


void InitDSAFuncs(DSAFuncs& dsa)
{
    dsa.ogluNamedBufferData = DecideFunc(glNamedBufferData, glNamedBufferDataEXT);
    dsa.ogluMapNamedBuffer = DecideFunc(glMapNamedBuffer, glMapNamedBufferEXT);
    dsa.ogluUnmapNamedBuffer = DecideFunc(glUnmapNamedBuffer, glUnmapNamedBufferEXT);
    dsa.ogluEnableVertexArrayAttrib = DecideFunc(glEnableVertexArrayAttrib, glEnableVertexArrayAttribEXT, ogluEnableVertexArrayAttrib);

    dsa.ogluCreateTextures = DecideFunc(glCreateTextures, ogluCreateTextures);
    dsa.ogluGetTextureLevelParameteriv = DecideFunc(std::pair{ glGetTextureLevelParameteriv, ogluGetTextureLevelParameterivARB }, glGetTextureLevelParameterivEXT);
    dsa.ogluTextureParameteri = DecideFunc(std::pair{ glTextureParameteri, ogluTextureParameteriARB }, glTextureParameteriEXT);
    dsa.ogluTextureImage1D = DecideFunc(glTextureImage1DEXT, ogluTextureImage1D);
    dsa.ogluTextureImage2D = DecideFunc(glTextureImage2DEXT, ogluTextureImage2D);
    dsa.ogluTextureImage3D = DecideFunc(glTextureImage3DEXT, ogluTextureImage3D);
    dsa.ogluTextureSubImage1D = DecideFunc(std::pair{ glTextureSubImage1D, ogluTextureSubImage1DARB }, glTextureSubImage1DEXT, ogluTextureSubImage1D);
    dsa.ogluTextureSubImage2D = DecideFunc(std::pair{ glTextureSubImage2D, ogluTextureSubImage2DARB }, glTextureSubImage2DEXT, ogluTextureSubImage2D);
    dsa.ogluTextureSubImage3D = DecideFunc(std::pair{ glTextureSubImage3D, ogluTextureSubImage3DARB }, glTextureSubImage3DEXT, ogluTextureSubImage3D);
    dsa.ogluGetTextureImage = DecideFunc(std::pair{ glGetTextureImage, ogluGetTextureImageARB }, std::pair{ glGetTextureImageEXT, ogluGetTextureImageEXT });
    dsa.ogluCompressedTextureImage1D = DecideFunc(glCompressedTextureImage1DEXT, ogluCompressedTextureImage1D);
    dsa.ogluCompressedTextureImage2D = DecideFunc(glCompressedTextureImage2DEXT, ogluCompressedTextureImage2D);
    dsa.ogluCompressedTextureImage3D = DecideFunc(glCompressedTextureImage3DEXT, ogluCompressedTextureImage3D);
    dsa.ogluCompressedTextureSubImage1D = DecideFunc(std::pair{ glCompressedTextureSubImage1D, ogluCompressedTextureSubImage1DARB }, glCompressedTextureSubImage1DEXT, ogluCompressedTextureSubImage1D);
    dsa.ogluCompressedTextureSubImage2D = DecideFunc(std::pair{ glCompressedTextureSubImage2D, ogluCompressedTextureSubImage2DARB }, glCompressedTextureSubImage2DEXT, ogluCompressedTextureSubImage2D);
    dsa.ogluCompressedTextureSubImage3D = DecideFunc(std::pair{ glCompressedTextureSubImage3D, ogluCompressedTextureSubImage3DARB }, glCompressedTextureSubImage3DEXT, ogluCompressedTextureSubImage3D);
    dsa.ogluGetCompressedTextureImage = DecideFunc(std::pair{ glGetCompressedTextureImage, ogluGetCompressedTextureImageARB }, std::pair{ glGetCompressedTextureImageEXT, ogluGetCompressedTextureImageEXT });
    dsa.ogluTextureStorage1D = DecideFunc(std::pair{ glTextureStorage1D, ogluTextureStorage1DARB }, glTextureStorage1DEXT, ogluTextureStorage1D);
    dsa.ogluTextureStorage2D = DecideFunc(std::pair{ glTextureStorage2D, ogluTextureStorage2DARB }, glTextureStorage2DEXT, ogluTextureStorage2D);
    dsa.ogluTextureStorage3D = DecideFunc(std::pair{ glTextureStorage3D, ogluTextureStorage3DARB }, glTextureStorage3DEXT, ogluTextureStorage3D);
    dsa.ogluBindTextureUnit = DecideFunc(std::pair{ glBindTextureUnit, ogluBindTextureUnitARB }, std::pair{ glBindMultiTextureEXT, ogluBindTextureUnitEXT }, ogluBindTextureUnit);
}

}