#include "oglPch.h"
#include "DSAWrapper.h"
#include "oglUtil.h"
#include "oglException.h"
#include "oglContext.h"
#include "oglBuffer.h"

#if COMMON_OS_WIN
#   include "GL/wglext.h"
#elif COMMON_OS_UNIX
#   include "glew/glxew.h"
#   include "GL/glxext.h"
#else
#   error "unknown os"
#endif


namespace oglu
{

thread_local const DSAFuncs* DSA = nullptr;

template<typename T>
static forceinline T DecideFunc() { return nullptr; }
template<typename T, typename... Ts>
static forceinline T DecideFunc(const T& func, Ts... funcs)
{
    if constexpr (sizeof...(Ts) > 0)
        return func ? func : DecideFunc<T>(funcs...);
    else
        return func ? func : nullptr;
}
template<typename T, typename T2, typename... Ts>
static forceinline T DecideFunc(const std::pair<T2, T>& func, Ts... funcs)
{
    if constexpr (sizeof...(Ts) > 0)
        return func.first ? func.second : DecideFunc<T>(funcs...);
    else
        return func.first ? func.second : nullptr;
}


extern GLuint GetCurFBO();






static void GLAPIENTRY ogluCreateFramebuffers(GLsizei n, GLuint *framebuffers)
{
    glGenFramebuffers(n, framebuffers);
    for (int i = 0; i < n; ++i)
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);
    glBindFramebuffer(GL_FRAMEBUFFER, GetCurFBO());
}
static void GLAPIENTRY ogluFramebufferTextureARB(GLuint framebuffer, GLenum attachment, GLenum, GLuint texture, GLint level)
{
    glNamedFramebufferTexture(framebuffer, attachment, texture, level);
}
static void GLAPIENTRY ogluFramebufferTextureEXT(GLuint framebuffer, GLenum attachment, GLenum, GLuint texture, GLint level)
{
    glNamedFramebufferTextureEXT(framebuffer, attachment, texture, level);
}
static void GLAPIENTRY ogluFramebufferTexture(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    switch (textarget)
    {
    case GL_TEXTURE_1D:
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture1D(framebuffer, attachment, textarget, texture, level);
        glBindFramebuffer(GL_FRAMEBUFFER, GetCurFBO());
        break;
    case GL_TEXTURE_2D:
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(framebuffer, attachment, textarget, texture, level);
        glBindFramebuffer(GL_FRAMEBUFFER, GetCurFBO());
        break;
    default:
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"unsupported texture argument when calling Non-DSA FramebufferTexture");
    }
}
static void GLAPIENTRY ogluFramebufferTextureLayerARB(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    glNamedFramebufferTextureLayer(framebuffer, attachment, texture, level, layer);
}
static void GLAPIENTRY ogluFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, texture, level, layer);
    glBindFramebuffer(GL_FRAMEBUFFER, GetCurFBO());
}
static void GLAPIENTRY ogluFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, renderbuffertarget, renderbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, GetCurFBO());
}




template<typename T>
static forceinline T QueryFunc_(const std::string_view name)
{
#if COMMON_OS_WIN
    return reinterpret_cast<T>(wglGetProcAddress(name.data()));
#else 
    return reinterpret_cast<T>(glXGetProcAddress(reinterpret_cast<const GLubyte*>(name.data())));
#endif
}
template<typename T>
[[nodiscard]] static forceinline T QueryFunc_(const T func)
{
    return func;
}

template<typename T, typename Arg, typename... Args>
static forceinline void QueryFunc(T& target, const Arg& arg, const Args&... args)
{
    const auto ret = QueryFunc_<T>(arg);
    if constexpr (sizeof...(Args) == 0)
        target = ret;
    else
    {
        if (ret != nullptr)
            target = ret;
        else
            QueryFunc(target, args...);
    }
}



void InitDSAFuncs(DSAFuncs& dsa)
{
#define WITH_SUFFIX(r, name, i, sfx)    BOOST_PP_COMMA_IF(i) "gl" name sfx
#define WITH_SUFFIXS(name, ...) BOOST_PP_SEQ_FOR_EACH_I(WITH_SUFFIX, name, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define QUERY_FUNC(name, ...)   QueryFunc(PPCAT(dsa.oglu, name),          WITH_SUFFIXS(#name, __VA_ARGS__))
#define QUERY_FUNC_(name, ...)  QueryFunc(PPCAT(PPCAT(dsa.oglu, name),_), WITH_SUFFIXS(#name, __VA_ARGS__))

    // buffer related
    QUERY_FUNC (GenBuffers,                     "", "ARB");
    QUERY_FUNC (DeleteBuffers,                  "", "ARB");
    QUERY_FUNC (BindBuffer,                     "", "ARB");
    QUERY_FUNC (BindBufferBase,                 "", "EXT", "NV");
    QUERY_FUNC (BindBufferRange,                "", "EXT", "NV");
    QUERY_FUNC_(NamedBufferStorage,             "", "EXT");
    QUERY_FUNC_(BufferStorage,                  "", "EXT");
    QUERY_FUNC_(NamedBufferData,                "", "EXT");
    QUERY_FUNC_(BufferData,                     "", "ARB");
    QUERY_FUNC_(NamedBufferSubData,             "", "EXT");
    QUERY_FUNC_(BufferSubData,                  "", "ARB");
    QUERY_FUNC_(MapNamedBuffer,                 "", "EXT");
    QUERY_FUNC_(MapBuffer,                      "", "ARB");
    QUERY_FUNC_(UnmapNamedBuffer,               "", "EXT");
    QUERY_FUNC_(UnmapBuffer,                    "", "ARB");

    // vao related
    QUERY_FUNC (GenVertexArrays,                "", "ARB");
    QUERY_FUNC (DeleteVertexArrays,             "", "ARB");
    QUERY_FUNC (BindVertexArray,                "", "ARB");
    QUERY_FUNC (EnableVertexAttribArray,        "", "ARB");
    QUERY_FUNC_(EnableVertexArrayAttrib,        "", "EXT");
    QUERY_FUNC_(VertexAttribIPointer,           "", "ARB");
    QUERY_FUNC_(VertexAttribLPointer,           "", "EXT");
    QUERY_FUNC_(VertexAttribPointer,            "", "ARB");
    QUERY_FUNC (VertexAttribDivisor,            "", "ARB", "EXT", "NV");

    // draw related
    QUERY_FUNC (MultiDrawArrays,                                "", "EXT");
    QUERY_FUNC (MultiDrawElements,                              "", "EXT");
    QUERY_FUNC_(MultiDrawArraysIndirect,                        "", "EXT", "AMD");
    QUERY_FUNC_(DrawArraysInstancedBaseInstance,                "", "EXT");
    QUERY_FUNC_(DrawArraysInstanced,                            "", "ARB", "EXT", "NV", "ANGLE");
    QUERY_FUNC_(MultiDrawElementsIndirect,                      "", "EXT", "AMD");
    QUERY_FUNC_(DrawElementsInstancedBaseVertexBaseInstance,    "", "EXT");
    QUERY_FUNC_(DrawElementsInstancedBaseInstance,              "", "EXT");
    QUERY_FUNC_(DrawElementsInstanced,                          "", "ARB", "EXT", "NV", "ANGLE");

    //texture related
    QUERY_FUNC (ActiveTexture,                  "", "ARB");
    QUERY_FUNC_(CreateTextures,                 "");
    QUERY_FUNC_(BindTextureUnit,                "");
    QUERY_FUNC_(BindMultiTextureEXT,            "");
    QUERY_FUNC (BindImageTexture,               "", "EXT");
    QUERY_FUNC (TextureView,                    "", "EXT");
    QUERY_FUNC_(TextureBuffer,                  "");
    QUERY_FUNC_(TextureBufferEXT,               "");
    QUERY_FUNC_(TexBuffer,                      "", "ARB", "EXT");
    QUERY_FUNC_(GenerateTextureMipmap,          "");
    QUERY_FUNC_(GenerateTextureMipmapEXT,       "");
    QUERY_FUNC_(GenerateMipmap,                 "", "EXT");
    QUERY_FUNC_(TextureParameteri,              "");
    QUERY_FUNC_(TextureParameteriEXT,           "");
    QUERY_FUNC_(TextureSubImage1D,              "");
    QUERY_FUNC_(TextureSubImage1DEXT,           "");
    QUERY_FUNC_(TextureSubImage2D,              "");
    QUERY_FUNC_(TextureSubImage2DEXT,           "");
    QUERY_FUNC_(TextureSubImage3D,              "");
    QUERY_FUNC_(TextureSubImage3DEXT,           "");
    QUERY_FUNC_(TexSubImage3D,                  "", "EXT", "NV");
    QUERY_FUNC_(TextureImage1DEXT,              "");
    QUERY_FUNC_(TextureImage2DEXT,              "");
    QUERY_FUNC_(TextureImage3DEXT,              "");
    QUERY_FUNC_(TexImage3D,                     "", "EXT", "NV");
    QUERY_FUNC_(CompressedTextureSubImage1D,    "");
    QUERY_FUNC_(CompressedTextureSubImage1DEXT, "");
    QUERY_FUNC_(CompressedTextureSubImage2D,    "");
    QUERY_FUNC_(CompressedTextureSubImage2DEXT, "");
    QUERY_FUNC_(CompressedTextureSubImage3D,    "");
    QUERY_FUNC_(CompressedTextureSubImage3DEXT, "");
    QUERY_FUNC_(CompressedTexSubImage1D,        "", "ARB");
    QUERY_FUNC_(CompressedTexSubImage2D,        "", "ARB");
    QUERY_FUNC_(CompressedTexSubImage3D,        "", "ARB");
    QUERY_FUNC_(CompressedTextureImage1DEXT,    "");
    QUERY_FUNC_(CompressedTextureImage2DEXT,    "");
    QUERY_FUNC_(CompressedTextureImage3DEXT,    "");
    QUERY_FUNC_(CompressedTexImage1D,           "", "ARB");
    QUERY_FUNC_(CompressedTexImage2D,           "", "ARB");
    QUERY_FUNC_(CompressedTexImage3D,           "", "ARB");
    QUERY_FUNC (CopyImageSubData,               "", "EXT", "NV");
    QUERY_FUNC_(TextureStorage1D,               "");
    QUERY_FUNC_(TextureStorage1DEXT,            "");
    QUERY_FUNC_(TextureStorage2D,               "");
    QUERY_FUNC_(TextureStorage2DEXT,            "");
    QUERY_FUNC_(TextureStorage3D,               "");
    QUERY_FUNC_(TextureStorage3DEXT,            "");
    QUERY_FUNC_(TexStorage1D,                   "", "EXT");
    QUERY_FUNC_(TexStorage2D,                   "", "EXT");
    QUERY_FUNC_(TexStorage3D,                   "", "EXT");
    QUERY_FUNC (ClearTexImage,                  "", "EXT");
    QUERY_FUNC (ClearTexSubImage,               "", "EXT");
    QUERY_FUNC_(GetTextureLevelParameteriv,     "");
    QUERY_FUNC_(GetTextureLevelParameterivEXT,  "");
    QUERY_FUNC_(GetTextureImage,                "");
    QUERY_FUNC_(GetTextureImageEXT,             "");
    QUERY_FUNC_(GetCompressedTextureImage,      "");
    QUERY_FUNC_(GetCompressedTextureImageEXT,   "");
    QUERY_FUNC_(GetCompressedTexImage,          "", "ARB");

    
    dsa.ogluCreateFramebuffers = DecideFunc(std::pair{ glNamedFramebufferTexture, glCreateFramebuffers }, std::pair{ glNamedFramebufferTextureEXT, glGenFramebuffers }, ogluCreateFramebuffers);
    dsa.ogluFramebufferTexture = DecideFunc(std::pair{ glNamedFramebufferTexture, &ogluFramebufferTextureARB }, std::pair{ glNamedFramebufferTextureEXT, &ogluFramebufferTextureEXT }, &ogluFramebufferTexture);
    dsa.ogluFramebufferTextureLayer = DecideFunc(std::pair{ glNamedFramebufferTextureLayer, &ogluFramebufferTextureLayerARB }, glNamedFramebufferTextureLayerEXT, &ogluFramebufferTextureLayer);
    dsa.ogluFramebufferRenderbuffer = DecideFunc(glNamedFramebufferRenderbuffer, glNamedFramebufferRenderbufferEXT, &ogluFramebufferRenderbuffer);
}

#define CALL_EXISTS(func, ...) if (func) { return func(__VA_ARGS__); }


void DSAFuncs::ogluNamedBufferStorage(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) const
{
    CALL_EXISTS(ogluNamedBufferStorage_, buffer, size, data, flags)
    {
        ogluBindBuffer(target, buffer);
        ogluBufferStorage_(target, size, data, flags);
    }
}
void DSAFuncs::ogluNamedBufferData(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) const
{
    CALL_EXISTS(ogluNamedBufferData_, buffer, size, data, usage)
    {
        ogluBindBuffer(target, buffer);
        ogluBufferData_(target, size, data, usage);
    }
}
void DSAFuncs::ogluNamedBufferSubData(GLenum target, GLuint buffer, GLintptr offset, GLsizeiptr size, const void* data) const
{
    CALL_EXISTS(ogluNamedBufferSubData_, buffer, offset, size, data)
    {
        ogluBindBuffer(target, buffer);
        ogluBufferSubData_(target, offset, size, data);
    }
}
void* DSAFuncs::ogluMapNamedBuffer(GLenum target, GLuint buffer, GLenum access) const
{
    CALL_EXISTS(ogluMapNamedBuffer_, buffer, access)
    {
        ogluBindBuffer(target, buffer);
        return ogluMapBuffer_(target, access);
    }
}
GLboolean DSAFuncs::ogluUnmapNamedBuffer(GLenum target, GLuint buffer) const
{
    CALL_EXISTS(ogluUnmapNamedBuffer_, buffer)
    {
        ogluBindBuffer(target, buffer);
        return ogluUnmapBuffer_(target);
    }
}


void DSAFuncs::ogluEnableVertexArrayAttrib(GLuint vaobj, GLuint index) const
{
    CALL_EXISTS(ogluEnableVertexArrayAttrib_, vaobj, index)
    {
        ogluBindVertexArray(vaobj); // ensure be in binding
        ogluEnableVertexAttribArray(index);
        // ogluBindVertexArray(0); // may be in binding
    }
}
void DSAFuncs::ogluVertexAttribPointer(GLuint index, GLint size, GLenum type, bool normalized, GLsizei stride, size_t offset) const
{
    const auto pointer = reinterpret_cast<const void*>(uintptr_t(offset));
    ogluVertexAttribPointer_(index, size, type, normalized ? GL_TRUE : GL_FALSE, stride, pointer);
}
void DSAFuncs::ogluVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const
{
    const auto pointer = reinterpret_cast<const void*>(uintptr_t(offset));
    ogluVertexAttribIPointer_(index, size, type, stride, pointer);
}
void DSAFuncs::ogluVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const
{
    const auto pointer = reinterpret_cast<const void*>(uintptr_t(offset));
    ogluVertexAttribLPointer_(index, size, type, stride, pointer);
}


void DSAFuncs::ogluMultiDrawArraysIndirect(GLenum mode, const oglIndirectBuffer_& indirect, GLint offset, GLsizei primcount) const
{
    if (ogluMultiDrawArraysIndirect_)
    {
        ogluBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect.BufferID); //IBO not included in VAO
        const auto pointer = reinterpret_cast<const void*>(uintptr_t(sizeof(oglIndirectBuffer_::DrawArraysIndirectCommand) * offset));
        ogluMultiDrawArraysIndirect_(mode, pointer, primcount, 0);
    }
    else if (ogluDrawArraysInstancedBaseInstance_)
    {
        const auto& cmd = indirect.GetArrayCommands();
        for (GLsizei i = offset; i < primcount; i++)
        {
            ogluDrawArraysInstancedBaseInstance_(mode, cmd[i].first, cmd[i].count, cmd[i].instanceCount, cmd[i].baseInstance);
        }
    }
    else if (ogluDrawArraysInstanced_)
    {
        const auto& cmd = indirect.GetArrayCommands();
        for (GLsizei i = offset; i < primcount; i++)
        {
            ogluDrawArraysInstanced_(mode, cmd[i].first, cmd[i].count, cmd[i].instanceCount); // baseInstance ignored
        }
    }
    else
    {
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"no avaliable implementation for MultiDrawArraysIndirect");
    }
}
void DSAFuncs::ogluMultiDrawElementsIndirect(GLenum mode, GLenum type, const oglIndirectBuffer_& indirect, GLint offset, GLsizei primcount) const
{
    if (ogluMultiDrawElementsIndirect_)
    {
        ogluBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect.BufferID); //IBO not included in VAO
        const auto pointer = reinterpret_cast<const void*>(uintptr_t(sizeof(oglIndirectBuffer_::DrawArraysIndirectCommand) * offset));
        ogluMultiDrawElementsIndirect_(mode, type, pointer, primcount, 0);
    }
    else if (ogluDrawElementsInstancedBaseVertexBaseInstance_)
    {
        const auto& cmd = indirect.GetElementCommands();
        for (GLsizei i = offset; i < primcount; i++)
        {
            const auto pointer = reinterpret_cast<const void*>(uintptr_t(cmd[i].firstIndex));
            ogluDrawElementsInstancedBaseVertexBaseInstance_(mode, cmd[i].count, type, pointer, cmd[i].instanceCount, cmd[i].baseVertex, cmd[i].baseInstance);
        }
    }
    else if (ogluDrawElementsInstancedBaseInstance_)
    {
        const auto& cmd = indirect.GetElementCommands();
        for (GLsizei i = offset; i < primcount; i++)
        {
            const auto pointer = reinterpret_cast<const void*>(uintptr_t(cmd[i].firstIndex));
            ogluDrawElementsInstancedBaseInstance_(mode, cmd[i].count, type, pointer, cmd[i].instanceCount, cmd[i].baseInstance); // baseInstance ignored
        }
    }
    else if (ogluDrawElementsInstanced_)
    {
        const auto& cmd = indirect.GetElementCommands();
        for (GLsizei i = offset; i < primcount; i++)
        {
            const auto pointer = reinterpret_cast<const void*>(uintptr_t(cmd[i].firstIndex));
            ogluDrawElementsInstanced_(mode, cmd[i].count, type, pointer, cmd[i].instanceCount); // baseInstance & baseVertex ignored
        }
    }
    else
    {
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"no avaliable implementation for MultiDrawElementsIndirect");
    }
}


void DSAFuncs::ogluCreateTextures(GLenum target, GLsizei n, GLuint* textures) const
{
    CALL_EXISTS(ogluCreateTextures_, target, n, textures)
    {
        glGenTextures(n, textures);
        ogluActiveTexture(GL_TEXTURE0);
        for (GLsizei i = 0; i < n; ++i)
            glBindTexture(target, textures[i]);
        glBindTexture(target, 0);
    }
}
void DSAFuncs::ogluBindTextureUnit(GLuint unit, GLuint texture, GLenum target) const
{
    CALL_EXISTS(ogluBindTextureUnit_,                   unit,         texture)
    CALL_EXISTS(ogluBindMultiTextureEXT_, GL_TEXTURE0 + unit, target, texture)
    {
        ogluActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(target, texture);
    }
}
void DSAFuncs::ogluTextureBuffer(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer) const
{
    CALL_EXISTS(ogluTextureBuffer_,    target,         internalformat, buffer)
    CALL_EXISTS(ogluTextureBufferEXT_, target, target, internalformat, buffer)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluTexBuffer_(target, internalformat, buffer);
        glBindTexture(target, 0);
    }
}
void DSAFuncs::ogluGenerateTextureMipmap(GLuint texture, GLenum target) const
{
    CALL_EXISTS(ogluGenerateTextureMipmap_,    texture)
    CALL_EXISTS(ogluGenerateTextureMipmapEXT_, texture, target)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluGenerateMipmap_(target);
    }
}
void DSAFuncs::ogluTextureParameteri(GLuint texture, GLenum target, GLenum pname, GLint param) const
{
    CALL_EXISTS(ogluTextureParameteri_,    texture,         pname, param)
    CALL_EXISTS(ogluTextureParameteriEXT_, texture, target, pname, param)
    {
        glBindTexture(target, texture);
        glTexParameteri(target, pname, param);
    }
}
void DSAFuncs::ogluTextureSubImage1D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(ogluTextureSubImage1D_,    texture,         level, xoffset, width, format, type, pixels)
    CALL_EXISTS(ogluTextureSubImage1DEXT_, texture, target, level, xoffset, width, format, type, pixels)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
    }
}
void DSAFuncs::ogluTextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(ogluTextureSubImage2D_,    texture,         level, xoffset, yoffset, width, height, format, type, pixels)
    CALL_EXISTS(ogluTextureSubImage2DEXT_, texture, target, level, xoffset, yoffset, width, height, format, type, pixels)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
    }
}
void DSAFuncs::ogluTextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(ogluTextureSubImage3D_,    texture,         level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels)
    CALL_EXISTS(ogluTextureSubImage3DEXT_, texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluTexSubImage3D_(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    }
}
void DSAFuncs::ogluTextureImage1D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(ogluTextureImage1DEXT_, texture, target, level, internalformat, width, border, format, type, pixels)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
    }
}
void DSAFuncs::ogluTextureImage2D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(ogluTextureImage2DEXT_, texture, target, level, internalformat, width, height, border, format, type, pixels)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    }
}
void DSAFuncs::ogluTextureImage3D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(ogluTextureImage3DEXT_, texture, target, level, internalformat, width, height, depth, border, format, type, pixels)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluTexImage3D_(target, level, internalformat, width, height, depth, border, format, type, pixels);
    }
}
void DSAFuncs::ogluCompressedTextureSubImage1D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(ogluCompressedTextureSubImage1D_,    texture,         level, xoffset, width, format, imageSize, data)
    CALL_EXISTS(ogluCompressedTextureSubImage1DEXT_, texture, target, level, xoffset, width, format, imageSize, data)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluCompressedTexSubImage1D_(target, level, xoffset, width, format, imageSize, data);
    }
}
void DSAFuncs::ogluCompressedTextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(ogluCompressedTextureSubImage2D_,    texture,         level, xoffset, yoffset, width, height, format, imageSize, data)
    CALL_EXISTS(ogluCompressedTextureSubImage2DEXT_, texture, target, level, xoffset, yoffset, width, height, format, imageSize, data)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluCompressedTexSubImage2D_(target, level, xoffset, yoffset, width, height, format, imageSize, data);
    }
}
void DSAFuncs::ogluCompressedTextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(ogluCompressedTextureSubImage3D_,    texture,         level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data)
    CALL_EXISTS(ogluCompressedTextureSubImage3DEXT_, texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluCompressedTexSubImage3D_(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    }
}
void DSAFuncs::ogluCompressedTextureImage1D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(ogluCompressedTextureImage1DEXT_, texture, target, level, internalformat, width, border, imageSize, data)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluCompressedTexImage1D_(target, level, internalformat, width, border, imageSize, data);
    }
}
void DSAFuncs::ogluCompressedTextureImage2D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(ogluCompressedTextureImage2DEXT_, texture, target, level, internalformat, width, height, border, imageSize, data)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluCompressedTexImage2D_(target, level, internalformat, width, height, border, imageSize, data);
    }
}
void DSAFuncs::ogluCompressedTextureImage3D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(ogluCompressedTextureImage3DEXT_, texture, target, level, internalformat, width, height, depth, border, imageSize, data)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluCompressedTexImage3D_(target, level, internalformat, width, height, depth, border, imageSize, data);
    }
}
void DSAFuncs::ogluTextureStorage1D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) const
{
    CALL_EXISTS(ogluTextureStorage1D_,    texture,         levels, internalformat, width)
    CALL_EXISTS(ogluTextureStorage1DEXT_, texture, target, levels, internalformat, width)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluTexStorage1D_(target, levels, internalformat, width);
    }
}
void DSAFuncs::ogluTextureStorage2D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) const
{
    CALL_EXISTS(ogluTextureStorage2D_,    texture,         levels, internalformat, width, height)
    CALL_EXISTS(ogluTextureStorage2DEXT_, texture, target, levels, internalformat, width, height)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluTexStorage2D_(target, levels, internalformat, width, height);
    }
}
void DSAFuncs::ogluTextureStorage3D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) const
{
    CALL_EXISTS(ogluTextureStorage3D_,    texture,         levels, internalformat, width, height, depth)
    CALL_EXISTS(ogluTextureStorage3DEXT_, texture, target, levels, internalformat, width, height, depth)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluTexStorage3D_(target, levels, internalformat, width, height, depth);
    }
}
void DSAFuncs::ogluGetTextureLevelParameteriv(GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params) const
{
    CALL_EXISTS(ogluGetTextureLevelParameteriv_,    texture,         level, pname, params)
    CALL_EXISTS(ogluGetTextureLevelParameterivEXT_, texture, target, level, pname, params)
    {
        glBindTexture(target, texture);
        glGetTexLevelParameteriv(target, level, pname, params);
    }
}
void DSAFuncs::ogluGetTextureImage(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, size_t bufSize, void* pixels) const
{
    CALL_EXISTS(ogluGetTextureImage_,    texture,         level, format, type, bufSize > INT32_MAX ? INT32_MAX : static_cast<GLsizei>(bufSize), pixels)
    CALL_EXISTS(ogluGetTextureImageEXT_, texture, target, level, format, type, pixels)
    {
        glBindTexture(target, texture);
        glGetTexImage(target, level, format, type, pixels);
    }
}
void DSAFuncs::ogluGetCompressedTextureImage(GLuint texture, GLenum target, GLint level, size_t bufSize, void* img) const
{
    CALL_EXISTS(ogluGetCompressedTextureImage_,    texture,         level, bufSize > INT32_MAX ? INT32_MAX : static_cast<GLsizei>(bufSize), img)
    CALL_EXISTS(ogluGetCompressedTextureImageEXT_, texture, target, level, img)
    {
        glBindTexture(target, texture);
        ogluGetCompressedTexImage_(target, level, img);
    }
}



}