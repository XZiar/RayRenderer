#include "oglPch.h"
#include "GLFuncWrapper.h"
#include "oglUtil.h"
#include "oglContext.h"
#include "oglBuffer.h"

#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/vmd/is_tuple.hpp>


//typedef void (APIENTRYP PFNGLDRAWARRAYSPROC) (GLenum mode, GLint first, GLsizei count);
//typedef void (APIENTRYP PFNGLDRAWELEMENTSPROC) (GLenum mode, GLsizei count, GLenum type, const void* indices);
//typedef void (APIENTRYP PFNGLGENTEXTURESPROC) (GLsizei n, GLuint* textures);
//typedef void (APIENTRYP PFNGLBINDTEXTUREPROC) (GLenum target, GLuint texture);
//typedef void (APIENTRYP PFNGLDELETETEXTURESPROC) (GLsizei n, const GLuint* textures);
//typedef void (APIENTRYP PFNGLGETTEXIMAGEPROC) (GLenum target, GLint level, GLenum format, GLenum type, void* pixels); 
//typedef void (APIENTRYP PFNGLTEXIMAGE1DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels);
//typedef void (APIENTRYP PFNGLTEXIMAGE2DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
//typedef void (APIENTRYP PFNGLTEXSUBIMAGE1DPROC) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels);
//typedef void (APIENTRYP PFNGLTEXSUBIMAGE2DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
//typedef void (APIENTRYP PFNGLGETTEXLEVELPARAMETERIVPROC) (GLenum target, GLint level, GLenum pname, GLint* params);
//typedef void (APIENTRYP PFNGLCULLFACEPROC) (GLenum mode);
//typedef void (APIENTRYP PFNGLFRONTFACEPROC) (GLenum mode);
//typedef void (APIENTRYP PFNGLHINTPROC) (GLenum target, GLenum mode);
//typedef void (APIENTRYP PFNGLDISABLEPROC) (GLenum cap);
//typedef void (APIENTRYP PFNGLENABLEPROC) (GLenum cap);
//typedef GLboolean (APIENTRYP PFNGLISENABLEDPROC) (GLenum cap);
//typedef void (APIENTRYP PFNGLFINISHPROC) (void);
//typedef void (APIENTRYP PFNGLFLUSHPROC) (void);
//typedef void (APIENTRYP PFNGLCLEARPROC) (GLbitfield mask);
//typedef void (APIENTRYP PFNGLCLEARCOLORPROC) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
//typedef void (APIENTRYP PFNGLCLEARSTENCILPROC) (GLint s);
//typedef void (APIENTRYP PFNGLCLEARDEPTHPROC) (GLdouble depth); typedef void (APIENTRYP PFNGLGETBOOLEANVPROC) (GLenum pname, GLboolean* data);
//typedef void (APIENTRYP PFNGLGETDOUBLEVPROC) (GLenum pname, GLdouble* data);
//typedef void (APIENTRYP PFNGLDEPTHFUNCPROC) (GLenum func);
//typedef void (APIENTRYP PFNGLVIEWPORTPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
//typedef GLenum (APIENTRYP PFNGLGETERRORPROC) (void);
//typedef void (APIENTRYP PFNGLGETFLOATVPROC) (GLenum pname, GLfloat* data);
//typedef void (APIENTRYP PFNGLGETINTEGERVPROC) (GLenum pname, GLint* data);
//typedef const GLubyte* (APIENTRYP PFNGLGETSTRINGPROC) (GLenum name);


#ifndef GL_APIENTRY
#   define GL_APIENTRY APIENTRY
#endif
#ifndef GL_APIENTRYP
#   define GL_APIENTRYP APIENTRY *
#endif
#undef GL_KHR_debug
#include "GLES2/gl2ext.h"
#undef APIENTRY
#if COMMON_OS_WIN
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include <Windows.h>
//fucking wingdi defines some terrible macro
#   undef ERROR
#   undef MemoryBarrier
#elif COMMON_OS_UNIX
#   include <dlfcn.h>
#else
#   error "unknown os"
#endif




namespace oglu
{
using namespace std::string_view_literals;
[[maybe_unused]] constexpr auto CtxFuncsSize = sizeof(CtxFuncs);


namespace detail
{
const common::container::FrozenDenseSet<std::string_view> GLBasicAPIs = std::vector<std::string_view>
{
    "glGenTextures"sv,      "glDeleteTextures"sv,   "glBindTexture"sv,      "glGetTexImage"sv,
    "glTexImage1D"sv,       "glTexImage2D"sv,       "glTexSubImage1D"sv,    "glTexSubImage2D"sv,
    "glTexParameteri"sv,    "glGetTexParameteriv"sv, "glGetTexLevelParameteriv"sv, "glClear"sv,
    "glClearColor"sv,       "glClearDepth"sv,       "glClearStencil"sv,     "glGetError"sv,
    "glGetFloatv"sv,        "glGetIntegerv"sv,      "glGetString"sv,        "glIsEnabled"sv,
    "glEnable"sv,           "glDisable"sv,          "glFinish"sv,           "glFlush"sv,
    "glDepthFunc"sv,        "glCullFace"sv,         "glFrontFace"sv,        "glViewport"sv,
    "glDrawArrays"sv,       "glDrawElements"sv,     "glReadPixels"sv,
};
}

void* GLHost::GetFunction(std::string_view name) const noexcept
{
    if (const auto func = LoadFunction(name); func)
        return func;
    if (const auto ptr = detail::GLBasicAPIs.Find(name); ptr)
        return GetBasicFunctions(ptr - detail::GLBasicAPIs.RawData().data(), *ptr);
    return nullptr;
}



template<typename T, typename... Args>
struct ResourceKeeper
{
    std::vector<std::unique_ptr<T>> Items;
    common::SpinLocker Locker;
    auto Lock() { return Locker.LockScope(); }
    T* FindOrCreate(void* key, Args... args)
    {
        const auto lock = Lock();
        for (const auto& item : Items)
        {
            if (item->Target == key)
                return item.get();
        }
        return Items.emplace_back(std::make_unique<T>(key, args...)).get();
    }
    void Remove(void* key)
    {
        const auto lock = Lock();
        for (auto it = Items.begin(); it != Items.end(); ++it)
        {
            if ((*it)->Target == key)
            {
                Items.erase(it);
                return;
            }
        }
    }
};
thread_local const CtxFuncs* CtxFunc = nullptr;

static auto& GetCtxFuncsMap()
{
    static ResourceKeeper<CtxFuncs, const GLHost&, std::pair<bool, bool>> CtxFuncMap;
    return CtxFuncMap;
}
static void PrepareCtxFuncs(void* hRC, const GLHost& host, std::pair<bool, bool> shouldPrint)
{
    if (hRC == nullptr)
        CtxFunc = nullptr;
    else
    {
        auto& rmap = GetCtxFuncsMap();
        CtxFunc = rmap.FindOrCreate(hRC, host, shouldPrint);
    }
}


static void ShowQuerySuc (const std::string_view tarName, const std::string_view ext, void* ptr)
{
    oglLog().Verbose(FmtString(u"Func [{}] uses [{}] ({:p})\n"sv), tarName, ext, (void*)ptr);
}
static void ShowQueryFail(const std::string_view tarName)
{
    oglLog().Warning(FmtString(u"Func [{}] not found\n"sv), tarName);
}
//static void ShowQueryFall(const std::string_view tarName)
//{
//    oglLog().Warning(FMT_STRING(u"Func [{}] fallback to default\n"), tarName);
//}


bool GLHost::MakeGLContextCurrent(void* hRC) const
{
    const auto ret = MakeGLContextCurrent_(hRC);
    if (ret || hRC == nullptr)
    {
        PrepareCtxFuncs(hRC, *this, {false, false});
    }
    return ret;
}
const CtxFuncs* CtxFuncs::PrepareCurrent(const GLHost& host, void* hRC, std::pair<bool, bool> shouldPrint)
{
    auto& rmap = GetCtxFuncsMap();
    return rmap.FindOrCreate(hRC, host, shouldPrint);
}


void oglUtil::InJectRenderDoc(const common::fs::path& dllPath)
{
    std::error_code ec;
#if COMMON_OS_WIN
    if (common::fs::exists(dllPath, ec))
        LoadLibrary(dllPath.c_str());
    else
        LoadLibrary(L"renderdoc.dll");
#else
    if (common::fs::exists(dllPath, ec))
        dlopen(dllPath.c_str(), RTLD_LAZY);
    else
        dlopen("renderdoc.dll", RTLD_LAZY);
#endif
}


template<typename F, typename T, size_t N>
static void QueryGLFunc(T& target, F&& getFunc, const std::string_view tarName, const std::pair<bool, bool> shouldPrint, const std::array<std::string_view, N> suffixes)
{
    Expects(tarName.size() < 96);
    const auto [printSuc, printFail] = shouldPrint;
    std::array<char, 128> funcName = { 0 };
    funcName[0] = 'g', funcName[1] = 'l';
    memcpy_s(&funcName[2], funcName.size() - 2, tarName.data(), tarName.size());
    const size_t baseLen = tarName.size() + 2;
    for (const auto& sfx : suffixes)
    {
        memcpy_s(&funcName[baseLen], funcName.size() - baseLen, sfx.data(), sfx.size());
        funcName[baseLen + sfx.size()] = '\0';
        const auto ptr = getFunc(funcName.data());
        if (ptr)
        {
            target = reinterpret_cast<T>(ptr);
            if (printSuc)
                ShowQuerySuc(tarName, sfx, (void*)ptr);
            return;
        }
    }
    /*if (fallback)
    {
        target = reinterpret_cast<T>(fallback);
        if (printFail)
            ShowQueryFall(tarName);
    }
    else*/
    {
        target = nullptr;
        if (printFail)
            ShowQueryFail(tarName);
    }
}

template <typename L, typename R> struct FuncTypeMatcher
{
    static constexpr bool IsMatch = false;
};
template<typename L, typename R>
inline constexpr bool ArgTypeMatcher()
{
    if constexpr (std::is_same_v<L, R>) return true;
    if constexpr (std::is_same_v<L, GLenum> && std::is_same_v<R, GLint>) return true; // some function use GLint for GLEnum
    return false;
}
template <typename R, typename... A, typename... B>
struct FuncTypeMatcher<R(*)(A...), R(*)(B...)>
{
    static constexpr bool IsMatch = (... && ArgTypeMatcher<A, B>());
};


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4003)
#endif


static void FillConextBaseInfo(ContextBaseInfo& info, PFNGLGETSTRINGPROC GetString, PFNGLGETINTEGERVPROC GetIntegerv)
{
    info.RendererString = common::str::to_u16string(
        reinterpret_cast<const char*>(GetString(GL_RENDERER)), common::str::Encoding::UTF8);
    info.VendorString = common::str::to_u16string(
        reinterpret_cast<const char*>(GetString(GL_VENDOR)), common::str::Encoding::UTF8);
    info.VersionString = common::str::to_u16string(
        reinterpret_cast<const char*>(GetString(GL_VERSION)), common::str::Encoding::UTF8);
    int32_t major = 0, minor = 0;
    GetIntegerv(GL_MAJOR_VERSION, &major);
    GetIntegerv(GL_MINOR_VERSION, &minor);
    info.Version = major * 10 + minor;
    if (common::str::IsBeginWith(info.VersionString, u"OpenGL ES"sv))
        info.ContextType = GLType::ES;
}

std::optional<ContextBaseInfo> GLHost::FillBaseInfo(void* hRC) const
{
    const auto getFunc = [&](std::string_view func) { return GetFunction(func); };
#define QUERY_FUNC(part, name) PFNGL##part##PROC name = nullptr; QueryGLFunc(name, getFunc, #name, {false, false}, std::array{""sv})
    QUERY_FUNC(GETSTRING,   GetString);
    QUERY_FUNC(GETINTEGERV, GetIntegerv);
#undef QUERY_FUNC
    std::optional<ContextBaseInfo> binfo;
    TemporalInsideContext(hRC, [&](const auto)
    {
        binfo.emplace();
        FillConextBaseInfo(binfo.value(), GetString, GetIntegerv);
    });
    return binfo;
}


CtxFuncs::CtxFuncs(void* target, const GLHost& host, std::pair<bool, bool> shouldPrint) : Target(target)
{
    const auto getFunc = [&](std::string_view func) { return host.GetFunction(func); };
#define FUNC_TYPE_CHECK_(dst, part, name, sfx) static_assert(FuncTypeMatcher<decltype(dst), \
    BOOST_PP_CAT(BOOST_PP_CAT(PFNGL, part), BOOST_PP_CAT(sfx, PROC))>::IsMatch, \
    "mismatch type for variant [" #sfx "] on [" STRINGIZE(name) "]");
#define FUNC_TYPE_CHECK(r, tp, sfx) FUNC_TYPE_CHECK_(BOOST_PP_TUPLE_ELEM(0, tp), BOOST_PP_TUPLE_ELEM(1, tp), BOOST_PP_TUPLE_ELEM(2, tp), sfx)
#define SFX_STR(r, data, i, sfx) BOOST_PP_COMMA_IF(i) STRINGIZE(sfx)""sv
    
#define TOSEQ(...) BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)
#define QUERY_FUNC__(dst, part, name, sfxs, op) BOOST_PP_SEQ_FOR_EACH(FUNC_TYPE_CHECK, (dst, part, name), sfxs)\
    if (ContextType op GLType::ES) QueryGLFunc(dst, getFunc, #name, shouldPrint, std::array{ BOOST_PP_SEQ_FOR_EACH_I(SFX_STR, , sfxs) })
#define QUERY_FUNC_(dst, part, name, GLSfx, ESSfx) do {          \
    BOOST_PP_IF(BOOST_VMD_IS_TUPLE(GLSfx), QUERY_FUNC__(dst, part, name, BOOST_PP_TUPLE_TO_SEQ(GLSfx), !=), );  \
    BOOST_PP_IF(BOOST_VMD_IS_TUPLE(ESSfx), QUERY_FUNC__(dst, part, name, BOOST_PP_TUPLE_TO_SEQ(ESSfx), ==), );  \
    } while (0)
#define QUERY_FUNC(direct, part, name, GLSfx, ESSfx) QUERY_FUNC_(BOOST_PP_CAT(name, BOOST_PP_IF(direct, , _)), part, name, GLSfx, ESSfx)

#define QUERY_FUNC2(part, name) QueryGLFunc(name, getFunc, #name, {false, false}, std::array{""sv})
    QUERY_FUNC2(GETSTRING,   GetString);
    QUERY_FUNC2(GETINTEGERV, GetIntegerv);
#undef QUERY_FUNC2
    FillConextBaseInfo(*this, GetString, GetIntegerv);

    // buffer related
    QUERY_FUNC(1, GENBUFFERS,         GenBuffers,         (, ARB),     ());
    QUERY_FUNC(1, DELETEBUFFERS,      DeleteBuffers,      (, ARB),     ());
    QUERY_FUNC(1, BINDBUFFER,         BindBuffer,         (, ARB),     ());
    QUERY_FUNC(1, BINDBUFFERBASE,     BindBufferBase,     (, EXT, NV), ());
    QUERY_FUNC(1, BINDBUFFERRANGE,    BindBufferRange,    (, EXT, NV), ());
    QUERY_FUNC(0, NAMEDBUFFERSTORAGE, NamedBufferStorage, (, EXT),     _);
    QUERY_FUNC(0, BUFFERSTORAGE,      BufferStorage,      (),          (EXT));
    QUERY_FUNC(0, NAMEDBUFFERDATA,    NamedBufferData,    (, EXT),     _);
    QUERY_FUNC(0, BUFFERDATA,         BufferData,         (, ARB),     ());
    QUERY_FUNC(0, NAMEDBUFFERSUBDATA, NamedBufferSubData, (, EXT),     _);
    QUERY_FUNC(0, BUFFERSUBDATA,      BufferSubData,      (, ARB),     ());
    QUERY_FUNC(0, MAPNAMEDBUFFER,     MapNamedBuffer,     (, EXT),     _);
    QUERY_FUNC(0, MAPBUFFER,          MapBuffer,          (, ARB),     (OES));
    QUERY_FUNC(1, MAPBUFFERRANGE,     MapBufferRange,     (),          (, EXT));
    QUERY_FUNC(0, UNMAPNAMEDBUFFER,   UnmapNamedBuffer,   (, EXT),     _);
    QUERY_FUNC(0, UNMAPBUFFER,        UnmapBuffer,        (, ARB),     (, OES));

    // vao related
    QUERY_FUNC(1, GENVERTEXARRAYS,         GenVertexArrays,         (, APPLE), (, OES));
    QUERY_FUNC(1, DELETEVERTEXARRAYS,      DeleteVertexArrays,      (, APPLE), (, OES));
    QUERY_FUNC(0, BINDVERTEXARRAY,         BindVertexArray,         (, APPLE), (, OES));
    QUERY_FUNC(0, ENABLEVERTEXATTRIBARRAY, EnableVertexAttribArray, (, ARB),   ());
    QUERY_FUNC(0, ENABLEVERTEXARRAYATTRIB, EnableVertexArrayAttrib, (, EXT),   _);
    QUERY_FUNC(0, VERTEXATTRIBIPOINTER,    VertexAttribIPointer,    (, EXT),   ());
    QUERY_FUNC(0, VERTEXATTRIBLPOINTER,    VertexAttribLPointer,    (, EXT),   _);
    QUERY_FUNC(0, VERTEXATTRIBPOINTER,     VertexAttribPointer,     (, ARB),   ());
    QUERY_FUNC(1, VERTEXATTRIBDIVISOR,     VertexAttribDivisor,     (, ARB),   (, EXT, ANGLE, NV));

    // draw related
    QUERY_FUNC(1, DRAWARRAYS,                                  DrawArrays,                                  (, EXT),      ());
    QUERY_FUNC(1, DRAWELEMENTS,                                DrawElements,                                (),           ());
    QUERY_FUNC(1, MULTIDRAWARRAYS,                             MultiDrawArrays,                             (, EXT),      (EXT));
    QUERY_FUNC(1, MULTIDRAWELEMENTS,                           MultiDrawElements,                           (, EXT),      (EXT));
    QUERY_FUNC(0, MULTIDRAWARRAYSINDIRECT,                     MultiDrawArraysIndirect,                     (, AMD),      (EXT));
    QUERY_FUNC(0, DRAWARRAYSINSTANCEDBASEINSTANCE,             DrawArraysInstancedBaseInstance,             (),           (EXT));
    QUERY_FUNC(0, DRAWARRAYSINSTANCED,                         DrawArraysInstanced,                         (, ARB, EXT), (, EXT, ANGLE, NV));
    QUERY_FUNC(0, MULTIDRAWELEMENTSINDIRECT,                   MultiDrawElementsIndirect,                   (, AMD),      (EXT));
    QUERY_FUNC(0, DRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE, DrawElementsInstancedBaseVertexBaseInstance, (),           (EXT));
    QUERY_FUNC(0, DRAWELEMENTSINSTANCEDBASEINSTANCE,           DrawElementsInstancedBaseInstance,           (),           (EXT));
    QUERY_FUNC(0, DRAWELEMENTSINSTANCED,                       DrawElementsInstanced,                       (, ARB, EXT), (, EXT, ANGLE, NV));

    //texture related
    QUERY_FUNC(1, GENTEXTURES,                    GenTextures,                    (),           ());
    QUERY_FUNC(0, CREATETEXTURES,                 CreateTextures,                 (),           _);
    QUERY_FUNC(1, DELETETEXTURES,                 DeleteTextures,                 (),           ());
    QUERY_FUNC(1, ACTIVETEXTURE,                  ActiveTexture,                  (, ARB),      ());
    QUERY_FUNC(1, BINDTEXTURE,                    BindTexture,                    (),           ());
    QUERY_FUNC(0, BINDTEXTUREUNIT,                BindTextureUnit,                (),           _);
    QUERY_FUNC(0, BINDMULTITEXTUREEXT,            BindMultiTextureEXT,            (),           _);
    QUERY_FUNC(1, BINDIMAGETEXTURE,               BindImageTexture,               (, EXT),      ());
    QUERY_FUNC(1, TEXTUREVIEW,                    TextureView,                    (),           (EXT, OES));
    QUERY_FUNC(0, TEXTUREBUFFER,                  TextureBuffer,                  (),           ());
    QUERY_FUNC(0, TEXTUREBUFFEREXT,               TextureBufferEXT,               (),           ());
    QUERY_FUNC(0, TEXBUFFER,                      TexBuffer,                      (, ARB, EXT), (, EXT, OES));
    QUERY_FUNC(0, GENERATETEXTUREMIPMAP,          GenerateTextureMipmap,          (),           _);
    QUERY_FUNC(0, GENERATETEXTUREMIPMAPEXT,       GenerateTextureMipmapEXT,       (),           _);
    QUERY_FUNC(0, GENERATEMIPMAP,                 GenerateMipmap,                 (, EXT),      ());
    QUERY_FUNC(1, GETTEXTUREHANDLE,               GetTextureHandle,               (ARB, NV),    (IMG, NV));
    QUERY_FUNC(1, MAKETEXTUREHANDLERESIDENT,      MakeTextureHandleResident,      (ARB, NV),    (NV));
    QUERY_FUNC(1, MAKETEXTUREHANDLENONRESIDENT,   MakeTextureHandleNonResident,   (ARB, NV),    (NV));
    QUERY_FUNC(1, GETIMAGEHANDLE,                 GetImageHandle,                 (ARB, NV),    (NV));
    QUERY_FUNC(1, MAKEIMAGEHANDLERESIDENT,        MakeImageHandleResident,        (ARB, NV),    (NV));
    QUERY_FUNC(1, MAKEIMAGEHANDLENONRESIDENT,     MakeImageHandleNonResident,     (ARB, NV),    (NV));
    QUERY_FUNC(1, TEXPARAMETERI,                  TexParameteri,                  (),           ());
    QUERY_FUNC(0, TEXTUREPARAMETERI,              TextureParameteri,              (),           _);
    QUERY_FUNC(0, TEXTUREPARAMETERIEXT,           TextureParameteriEXT,           (),           _);
    QUERY_FUNC(1, TEXIMAGE1D,                     TexImage1D,                     (),           _);
    QUERY_FUNC(1, TEXIMAGE2D,                     TexImage2D,                     (),           ());
    QUERY_FUNC(0, TEXIMAGE3D,                     TexImage3D,                     (, EXT),      (, OES));
    QUERY_FUNC(0, TEXSUBIMAGE1D,                  TexSubImage1D,                  (, EXT),      _);
    QUERY_FUNC(0, TEXSUBIMAGE2D,                  TexSubImage2D,                  (, EXT),      ());
    QUERY_FUNC(0, TEXSUBIMAGE3D,                  TexSubImage3D,                  (, EXT),      (, OES));
    QUERY_FUNC(0, TEXTURESUBIMAGE1D,              TextureSubImage1D,              (),           _);
    QUERY_FUNC(0, TEXTURESUBIMAGE2D,              TextureSubImage2D,              (),           _);
    QUERY_FUNC(0, TEXTURESUBIMAGE3D,              TextureSubImage3D,              (),           _);
    QUERY_FUNC(0, TEXTURESUBIMAGE1DEXT,           TextureSubImage1DEXT,           (),           _);
    QUERY_FUNC(0, TEXTURESUBIMAGE2DEXT,           TextureSubImage2DEXT,           (),           _);
    QUERY_FUNC(0, TEXTURESUBIMAGE3DEXT,           TextureSubImage3DEXT,           (),           _);
    QUERY_FUNC(0, TEXTUREIMAGE1DEXT,              TextureImage1DEXT,              (),           _);
    QUERY_FUNC(0, TEXTUREIMAGE2DEXT,              TextureImage2DEXT,              (),           _);
    QUERY_FUNC(0, TEXTUREIMAGE3DEXT,              TextureImage3DEXT,              (),           _);
    QUERY_FUNC(0, COMPRESSEDTEXTURESUBIMAGE1D,    CompressedTextureSubImage1D,    (),           _);
    QUERY_FUNC(0, COMPRESSEDTEXTURESUBIMAGE2D,    CompressedTextureSubImage2D,    (),           _);
    QUERY_FUNC(0, COMPRESSEDTEXTURESUBIMAGE3D,    CompressedTextureSubImage3D,    (),           _);
    QUERY_FUNC(0, COMPRESSEDTEXTURESUBIMAGE1DEXT, CompressedTextureSubImage1DEXT, (),           _);
    QUERY_FUNC(0, COMPRESSEDTEXTURESUBIMAGE2DEXT, CompressedTextureSubImage2DEXT, (),           _);
    QUERY_FUNC(0, COMPRESSEDTEXTURESUBIMAGE3DEXT, CompressedTextureSubImage3DEXT, (),           _);
    QUERY_FUNC(0, COMPRESSEDTEXSUBIMAGE1D,        CompressedTexSubImage1D,        (, ARB),      _);
    QUERY_FUNC(0, COMPRESSEDTEXSUBIMAGE2D,        CompressedTexSubImage2D,        (, ARB),      ());
    QUERY_FUNC(0, COMPRESSEDTEXSUBIMAGE3D,        CompressedTexSubImage3D,        (, ARB),      (, OES));
    QUERY_FUNC(0, COMPRESSEDTEXTUREIMAGE1DEXT,    CompressedTextureImage1DEXT,    (),           _);
    QUERY_FUNC(0, COMPRESSEDTEXTUREIMAGE2DEXT,    CompressedTextureImage2DEXT,    (),           _);
    QUERY_FUNC(0, COMPRESSEDTEXTUREIMAGE3DEXT,    CompressedTextureImage3DEXT,    (),           _);
    QUERY_FUNC(0, COMPRESSEDTEXIMAGE1D,           CompressedTexImage1D,           (, ARB),      _);
    QUERY_FUNC(0, COMPRESSEDTEXIMAGE2D,           CompressedTexImage2D,           (, ARB),      ());
    QUERY_FUNC(0, COMPRESSEDTEXIMAGE3D,           CompressedTexImage3D,           (, ARB),      (, OES));
    QUERY_FUNC(1, COPYIMAGESUBDATA,               CopyImageSubData,               (, NV),       (, EXT, OES));
    QUERY_FUNC(0, TEXTURESTORAGE1D,               TextureStorage1D,               (),           _);
    QUERY_FUNC(0, TEXTURESTORAGE2D,               TextureStorage2D,               (),           _);
    QUERY_FUNC(0, TEXTURESTORAGE3D,               TextureStorage3D,               (),           _);
    QUERY_FUNC(0, TEXTURESTORAGE1DEXT,            TextureStorage1DEXT,            (),           ());
    QUERY_FUNC(0, TEXTURESTORAGE2DEXT,            TextureStorage2DEXT,            (),           ());
    QUERY_FUNC(0, TEXTURESTORAGE3DEXT,            TextureStorage3DEXT,            (),           ());
    QUERY_FUNC(0, TEXSTORAGE1D,                   TexStorage1D,                   (),           (EXT));
    QUERY_FUNC(0, TEXSTORAGE2D,                   TexStorage2D,                   (),           (, EXT));
    QUERY_FUNC(0, TEXSTORAGE3D,                   TexStorage3D,                   (),           (, EXT));
    QUERY_FUNC(1, CLEARTEXIMAGE,                  ClearTexImage,                  (),           (EXT));
    QUERY_FUNC(1, CLEARTEXSUBIMAGE,               ClearTexSubImage,               (),           (EXT));
    QUERY_FUNC(0, GETCOMPRESSEDTEXTUREIMAGE,      GetCompressedTextureImage,      (),           _);
    QUERY_FUNC(0, GETCOMPRESSEDTEXTUREIMAGEEXT,   GetCompressedTextureImageEXT,   (),           _);
    QUERY_FUNC(0, GETCOMPRESSEDTEXIMAGE,          GetCompressedTexImage,          (, ARB),      _);
    QUERY_FUNC(0, GETTEXTUREIMAGE,                GetTextureImage,                (),           _);
    QUERY_FUNC(0, GETTEXTUREIMAGEEXT,             GetTextureImageEXT,             (),           _);
    QUERY_FUNC(1, GETTEXIMAGE,                    GetTexImage,                    (),           _);
    QUERY_FUNC(0, GETTEXTURELEVELPARAMETERIV,     GetTextureLevelParameteriv,     (),           _);
    QUERY_FUNC(0, GETTEXTURELEVELPARAMETERIVEXT,  GetTextureLevelParameterivEXT,  (),           _);
    QUERY_FUNC(0, GETTEXLEVELPARAMETERIV,         GetTexLevelParameteriv,         (),           ());
    QUERY_FUNC(0, GETTEXTUREPARAMETERIV,          GetTextureParameteriv,          (),           _);
    QUERY_FUNC(0, GETTEXTUREPARAMETERIVEXT,       GetTextureParameterivEXT,       (),           _);
    QUERY_FUNC(0, GETTEXPARAMETERIV,              GetTexParameteriv,              (),           ());
    QUERY_FUNC(1, EGLIMAGETARGETTEXTURE2DOES,     EGLImageTargetTexture2DOES,     _,            ());

    //rbo related
    QUERY_FUNC(0, GENRENDERBUFFERS,                               GenRenderbuffers,                               (, EXT), ());
    QUERY_FUNC(0, CREATERENDERBUFFERS,                            CreateRenderbuffers,                            (),      _);
    QUERY_FUNC(1, DELETERENDERBUFFERS,                            DeleteRenderbuffers,                            (, EXT), ());
    QUERY_FUNC(0, BINDRENDERBUFFER,                               BindRenderbuffer,                               (, EXT), ());
    QUERY_FUNC(0, NAMEDRENDERBUFFERSTORAGE,                       NamedRenderbufferStorage,                       (, EXT), _);
    QUERY_FUNC(0, RENDERBUFFERSTORAGE,                            RenderbufferStorage,                            (, EXT), ());
    QUERY_FUNC(0, NAMEDRENDERBUFFERSTORAGEMULTISAMPLE,            NamedRenderbufferStorageMultisample,            (, EXT), _);
    QUERY_FUNC(0, RENDERBUFFERSTORAGEMULTISAMPLE,                 RenderbufferStorageMultisample,                 (, EXT), (, EXT, IMG, ANGLE, NV, APPLE));
    QUERY_FUNC(0, NAMEDRENDERBUFFERSTORAGEMULTISAMPLECOVERAGEEXT, NamedRenderbufferStorageMultisampleCoverageEXT, (),      _);
    QUERY_FUNC(0, RENDERBUFFERSTORAGEMULTISAMPLECOVERAGENV,       RenderbufferStorageMultisampleCoverageNV,       (),      _);
    QUERY_FUNC(0, GETRENDERBUFFERPARAMETERIV,                     GetRenderbufferParameteriv,                     (, EXT), ());
    QUERY_FUNC(1, EGLIMAGETARGETRENDERBUFFERSTORAGEOES,           EGLImageTargetRenderbufferStorageOES,           _,       ());
    
    //fbo related
    QUERY_FUNC(0, GENFRAMEBUFFERS,                          GenFramebuffers,                          (, EXT),      ());
    QUERY_FUNC(0, CREATEFRAMEBUFFERS,                       CreateFramebuffers,                       (),           _);
    QUERY_FUNC(1, DELETEFRAMEBUFFERS,                       DeleteFramebuffers,                       (, EXT),      ());
    QUERY_FUNC(0, BINDFRAMEBUFFER,                          BindFramebuffer,                          (, EXT),      ());
    QUERY_FUNC(0, BLITNAMEDFRAMEBUFFER,                     BlitNamedFramebuffer,                     (),           _);
    QUERY_FUNC(0, BLITFRAMEBUFFER,                          BlitFramebuffer,                          (, EXT),      (, ANGLE, NV));
    QUERY_FUNC(1, DRAWBUFFERS,                              DrawBuffers,                              (, ARB, ATI), (, EXT, NV));
    QUERY_FUNC(0, INVALIDATENAMEDFRAMEBUFFERDATA,           InvalidateNamedFramebufferData,           (),           _);
    QUERY_FUNC(0, INVALIDATEFRAMEBUFFER,                    InvalidateFramebuffer,                    (),           ());
    QUERY_FUNC(0, DISCARDFRAMEBUFFEREXT,                    DiscardFramebufferEXT,                    _,            ());
    QUERY_FUNC(0, NAMEDFRAMEBUFFERRENDERBUFFER,             NamedFramebufferRenderbuffer,             (, EXT),      _);
    QUERY_FUNC(0, FRAMEBUFFERRENDERBUFFER,                  FramebufferRenderbuffer,                  (, EXT),      ());
    QUERY_FUNC(0, NAMEDFRAMEBUFFERTEXTURE1DEXT,             NamedFramebufferTexture1DEXT,             (),           _);
    QUERY_FUNC(0, FRAMEBUFFERTEXTURE1D,                     FramebufferTexture1D,                     (, EXT),      _);
    QUERY_FUNC(0, NAMEDFRAMEBUFFERTEXTURE2DEXT,             NamedFramebufferTexture2DEXT,             (),           _);
    QUERY_FUNC(0, FRAMEBUFFERTEXTURE2D,                     FramebufferTexture2D,                     (, EXT),      ());
    QUERY_FUNC(0, NAMEDFRAMEBUFFERTEXTURE3DEXT,             NamedFramebufferTexture3DEXT,             (),           _);
    QUERY_FUNC(0, FRAMEBUFFERTEXTURE3D,                     FramebufferTexture3D,                     (, EXT),      (OES));
    QUERY_FUNC(0, NAMEDFRAMEBUFFERTEXTURE,                  NamedFramebufferTexture,                  (, EXT),      _);
    QUERY_FUNC(0, FRAMEBUFFERTEXTURE,                       FramebufferTexture,                       (, EXT),      (, EXT, OES));
    QUERY_FUNC(0, NAMEDFRAMEBUFFERTEXTURELAYER,             NamedFramebufferTextureLayer,             (, EXT),      _);
    QUERY_FUNC(0, FRAMEBUFFERTEXTURELAYER,                  FramebufferTextureLayer,                  (, EXT),      ());
    QUERY_FUNC(0, CHECKNAMEDFRAMEBUFFERSTATUS,              CheckNamedFramebufferStatus,              (, EXT),      _);
    QUERY_FUNC(0, CHECKFRAMEBUFFERSTATUS,                   CheckFramebufferStatus,                   (, EXT),      ());
    QUERY_FUNC(0, GETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIV, GetNamedFramebufferAttachmentParameteriv, (, EXT),      _);
    QUERY_FUNC(0, GETFRAMEBUFFERATTACHMENTPARAMETERIV,      GetFramebufferAttachmentParameteriv,      (, EXT),      ());
    QUERY_FUNC(1, CLEAR,                                    Clear,                                    (),           ());
    QUERY_FUNC(1, CLEARCOLOR,                               ClearColor,                               (),           ());
    QUERY_FUNC(0, CLEARDEPTH,                               ClearDepth,                               (),           _);
    QUERY_FUNC(0, CLEARDEPTHF,                              ClearDepthf,                              (),           ());
    QUERY_FUNC(1, CLEARSTENCIL,                             ClearStencil,                             (),           ());
    QUERY_FUNC(0, CLEARNAMEDFRAMEBUFFERIV,                  ClearNamedFramebufferiv,                  (),           _);
    QUERY_FUNC(0, CLEARBUFFERIV,                            ClearBufferiv,                            (),           ());
    QUERY_FUNC(0, CLEARNAMEDFRAMEBUFFERUIV,                 ClearNamedFramebufferuiv,                 (),           _);
    QUERY_FUNC(0, CLEARBUFFERUIV,                           ClearBufferuiv,                           (),           ());
    QUERY_FUNC(0, CLEARNAMEDFRAMEBUFFERFV,                  ClearNamedFramebufferfv,                  (),           _);
    QUERY_FUNC(0, CLEARBUFFERFV,                            ClearBufferfv,                            (),           ());
    QUERY_FUNC(0, CLEARNAMEDFRAMEBUFFERFI,                  ClearNamedFramebufferfi,                  (),           _);
    QUERY_FUNC(0, CLEARBUFFERFI,                            ClearBufferfi,                            (),           ());
    QUERY_FUNC(0, READPIXELS,                               ReadPixels,                               (),           ());

    //shader related
    QUERY_FUNC(1, CREATESHADER,     CreateShader,     (), ());
    QUERY_FUNC(1, DELETESHADER,     DeleteShader,     (), ());
    QUERY_FUNC(1, SHADERSOURCE,     ShaderSource,     (), ());
    QUERY_FUNC(1, COMPILESHADER,    CompileShader,    (), ());
    QUERY_FUNC(1, GETSHADERINFOLOG, GetShaderInfoLog, (), ());
    QUERY_FUNC(1, GETSHADERSOURCE,  GetShaderSource,  (), ());
    QUERY_FUNC(1, GETSHADERIV,      GetShaderiv,      (), ());

    //program related
    QUERY_FUNC(1, CREATEPROGRAM,           CreateProgram,           (), ());
    QUERY_FUNC(1, DELETEPROGRAM,           DeleteProgram,           (), ());
    QUERY_FUNC(1, ATTACHSHADER,            AttachShader,            (), ());
    QUERY_FUNC(1, DETACHSHADER,            DetachShader,            (), ());
    QUERY_FUNC(1, LINKPROGRAM,             LinkProgram,             (), ());
    QUERY_FUNC(1, USEPROGRAM,              UseProgram,              (), ());
    QUERY_FUNC(1, DISPATCHCOMPUTE,         DispatchCompute,         (), ());
    QUERY_FUNC(1, DISPATCHCOMPUTEINDIRECT, DispatchComputeIndirect, (), ()); 

    QUERY_FUNC(0, UNIFORM1F,        Uniform1f,        (), ());
    QUERY_FUNC(0, UNIFORM1FV,       Uniform1fv,       (), ());
    QUERY_FUNC(0, UNIFORM1I,        Uniform1i,        (), ());
    QUERY_FUNC(0, UNIFORM1IV,       Uniform1iv,       (), ());
    QUERY_FUNC(0, UNIFORM2F,        Uniform2f,        (), ());
    QUERY_FUNC(0, UNIFORM2FV,       Uniform2fv,       (), ());
    QUERY_FUNC(0, UNIFORM2I,        Uniform2i,        (), ());
    QUERY_FUNC(0, UNIFORM2IV,       Uniform2iv,       (), ());
    QUERY_FUNC(0, UNIFORM3F,        Uniform3f,        (), ());
    QUERY_FUNC(0, UNIFORM3FV,       Uniform3fv,       (), ());
    QUERY_FUNC(0, UNIFORM3I,        Uniform3i,        (), ());
    QUERY_FUNC(0, UNIFORM3IV,       Uniform3iv,       (), ());
    QUERY_FUNC(0, UNIFORM4F,        Uniform4f,        (), ());
    QUERY_FUNC(0, UNIFORM4FV,       Uniform4fv,       (), ());
    QUERY_FUNC(0, UNIFORM4I,        Uniform4i,        (), ());
    QUERY_FUNC(0, UNIFORM4IV,       Uniform4iv,       (), ());
    QUERY_FUNC(0, UNIFORMMATRIX2FV, UniformMatrix2fv, (), ());
    QUERY_FUNC(0, UNIFORMMATRIX3FV, UniformMatrix3fv, (), ());
    QUERY_FUNC(0, UNIFORMMATRIX4FV, UniformMatrix4fv, (), ());
    
    QUERY_FUNC(1, PROGRAMUNIFORM1D,          ProgramUniform1d,          (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORM1DV,         ProgramUniform1dv,         (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORM1F,          ProgramUniform1f,          (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM1FV,         ProgramUniform1fv,         (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM1I,          ProgramUniform1i,          (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM1IV,         ProgramUniform1iv,         (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM1UI,         ProgramUniform1ui,         (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM1UIV,        ProgramUniform1uiv,        (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM2D,          ProgramUniform2d,          (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORM2DV,         ProgramUniform2dv,         (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORM2F,          ProgramUniform2f,          (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM2FV,         ProgramUniform2fv,         (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM2I,          ProgramUniform2i,          (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM2IV,         ProgramUniform2iv,         (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM2UI,         ProgramUniform2ui,         (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM2UIV,        ProgramUniform2uiv,        (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM3D,          ProgramUniform3d,          (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORM3DV,         ProgramUniform3dv,         (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORM3F,          ProgramUniform3f,          (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM3FV,         ProgramUniform3fv,         (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM3I,          ProgramUniform3i,          (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM3IV,         ProgramUniform3iv,         (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM3UI,         ProgramUniform3ui,         (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM3UIV,        ProgramUniform3uiv,        (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM4D,          ProgramUniform4d,          (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORM4DV,         ProgramUniform4dv,         (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORM4F,          ProgramUniform4f,          (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM4FV,         ProgramUniform4fv,         (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM4I,          ProgramUniform4i,          (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM4IV,         ProgramUniform4iv,         (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM4UI,         ProgramUniform4ui,         (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORM4UIV,        ProgramUniform4uiv,        (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX2DV,   ProgramUniformMatrix2dv,   (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX2FV,   ProgramUniformMatrix2fv,   (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX2X3DV, ProgramUniformMatrix2x3dv, (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX2X3FV, ProgramUniformMatrix2x3fv, (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX2X4DV, ProgramUniformMatrix2x4dv, (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX2X4FV, ProgramUniformMatrix2x4fv, (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX3DV,   ProgramUniformMatrix3dv,   (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX3FV,   ProgramUniformMatrix3fv,   (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX3X2DV, ProgramUniformMatrix3x2dv, (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX3X2FV, ProgramUniformMatrix3x2fv, (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX3X4DV, ProgramUniformMatrix3x4dv, (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX3X4FV, ProgramUniformMatrix3x4fv, (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX4DV,   ProgramUniformMatrix4dv,   (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX4FV,   ProgramUniformMatrix4fv,   (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX4X2DV, ProgramUniformMatrix4x2dv, (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX4X2FV, ProgramUniformMatrix4x2fv, (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX4X3DV, ProgramUniformMatrix4x3dv, (),        _);
    QUERY_FUNC(1, PROGRAMUNIFORMMATRIX4X3FV, ProgramUniformMatrix4x3fv, (, EXT),   (, EXT));
    QUERY_FUNC(1, PROGRAMUNIFORMHANDLEUI64,  ProgramUniformHandleui64,  (ARB, NV), (IMG, NV));

    QUERY_FUNC(1, GETUNIFORMFV,                    GetUniformfv,                    (), ());
    QUERY_FUNC(1, GETUNIFORMIV,                    GetUniformiv,                    (), ());
    QUERY_FUNC(1, GETUNIFORMUIV,                   GetUniformuiv,                   (), ());
    QUERY_FUNC(1, GETPROGRAMINFOLOG,               GetProgramInfoLog,               (), ());
    QUERY_FUNC(1, GETPROGRAMIV,                    GetProgramiv,                    (), ());
    QUERY_FUNC(1, GETPROGRAMINTERFACEIV,           GetProgramInterfaceiv,           (), ());
    QUERY_FUNC(1, GETPROGRAMRESOURCEINDEX,         GetProgramResourceIndex,         (), ());
    QUERY_FUNC(1, GETPROGRAMRESOURCELOCATION,      GetProgramResourceLocation,      (), ());
    QUERY_FUNC(1, GETPROGRAMRESOURCELOCATIONINDEX, GetProgramResourceLocationIndex, (), (EXT)); 
    QUERY_FUNC(1, GETPROGRAMRESOURCENAME,          GetProgramResourceName,          (), ());
    QUERY_FUNC(1, GETPROGRAMRESOURCEIV,            GetProgramResourceiv,            (), ());
    QUERY_FUNC(1, GETACTIVESUBROUTINENAME,         GetActiveSubroutineName,         (), _);
    QUERY_FUNC(1, GETACTIVESUBROUTINEUNIFORMNAME,  GetActiveSubroutineUniformName,  (), _);
    QUERY_FUNC(1, GETACTIVESUBROUTINEUNIFORMIV,    GetActiveSubroutineUniformiv,    (), _);
    QUERY_FUNC(1, GETPROGRAMSTAGEIV,               GetProgramStageiv,               (), _);
    QUERY_FUNC(1, GETSUBROUTINEINDEX,              GetSubroutineIndex,              (), _);
    QUERY_FUNC(1, GETSUBROUTINEUNIFORMLOCATION,    GetSubroutineUniformLocation,    (), _);
    QUERY_FUNC(1, GETUNIFORMSUBROUTINEUIV,         GetUniformSubroutineuiv,         (), _);
    QUERY_FUNC(1, UNIFORMSUBROUTINESUIV,           UniformSubroutinesuiv,           (), _);
    QUERY_FUNC(1, GETACTIVEUNIFORMBLOCKNAME,       GetActiveUniformBlockName,       (), ());
    QUERY_FUNC(1, GETACTIVEUNIFORMBLOCKIV,         GetActiveUniformBlockiv,         (), ());
    QUERY_FUNC(1, GETACTIVEUNIFORMNAME,            GetActiveUniformName,            (), _);
    QUERY_FUNC(1, GETACTIVEUNIFORMSIV,             GetActiveUniformsiv,             (), ());
    QUERY_FUNC(1, GETINTEGERI_V,                   GetIntegeri_v,                   (), (, EXT));
    QUERY_FUNC(1, GETUNIFORMBLOCKINDEX,            GetUniformBlockIndex,            (), ());
    QUERY_FUNC(1, GETUNIFORMINDICES,               GetUniformIndices,               (), ());
    QUERY_FUNC(1, UNIFORMBLOCKBINDING,             UniformBlockBinding,             (), ());

    //query related
    QUERY_FUNC(1, GENQUERIES,          GenQueries,          (),      (, EXT));
    QUERY_FUNC(1, DELETEQUERIES,       DeleteQueries,       (),      (, EXT));
    QUERY_FUNC(1, BEGINQUERY,          BeginQuery,          (),      (, EXT));
    QUERY_FUNC(1, QUERYCOUNTER,        QueryCounter,        (),      (EXT));
    QUERY_FUNC(1, GETQUERYOBJECTIV,    GetQueryObjectiv,    (),      (EXT));
    QUERY_FUNC(1, GETQUERYOBJECTUIV,   GetQueryObjectuiv,   (),      (, EXT));
    QUERY_FUNC(1, GETQUERYOBJECTI64V,  GetQueryObjecti64v,  (, EXT), (EXT));
    QUERY_FUNC(1, GETQUERYOBJECTUI64V, GetQueryObjectui64v, (, EXT), (EXT));
    QUERY_FUNC(1, GETQUERYIV,          GetQueryiv,          (),      (, EXT));
    QUERY_FUNC(1, FENCESYNC,           FenceSync,           (),      (, APPLE));
    QUERY_FUNC(1, DELETESYNC,          DeleteSync,          (),      (, APPLE));
    QUERY_FUNC(1, CLIENTWAITSYNC,      ClientWaitSync,      (),      (, APPLE));
    QUERY_FUNC(1, WAITSYNC,            WaitSync,            (),      (, APPLE));
    QUERY_FUNC(1, GETINTEGER64V,       GetInteger64v,       (),      (, EXT, APPLE));
    QUERY_FUNC(1, GETSYNCIV,           GetSynciv,           (),      (, APPLE));

    //debug
    QUERY_FUNC(1, DEBUGMESSAGECALLBACK,    DebugMessageCallback,    (, ARB), (, KHR));
    QUERY_FUNC(1, DEBUGMESSAGECALLBACKAMD, DebugMessageCallbackAMD, (),      _);
    QUERY_FUNC(0, OBJECTLABEL,             ObjectLabel,             (),      (, KHR));
    QUERY_FUNC(0, LABELOBJECTEXT,          LabelObjectEXT,          (),      ());
    QUERY_FUNC(0, OBJECTPTRLABEL,          ObjectPtrLabel,          (),      (, KHR));
    QUERY_FUNC(0, PUSHDEBUGGROUP,          PushDebugGroup,          (),      (, KHR));
    QUERY_FUNC(0, POPDEBUGGROUP,           PopDebugGroup,           (),      (, KHR));
    QUERY_FUNC(0, PUSHGROUPMARKEREXT,      PushGroupMarkerEXT,      (),      ());
    QUERY_FUNC(0, POPGROUPMARKEREXT,       PopGroupMarkerEXT,       (),      ());
    QUERY_FUNC(0, DEBUGMESSAGEINSERT,      DebugMessageInsert,      (, ARB), (, KHR));
    QUERY_FUNC(1, DEBUGMESSAGEINSERTAMD,   DebugMessageInsertAMD,   (),      _);
    QUERY_FUNC(1, INSERTEVENTMARKEREXT,    InsertEventMarkerEXT,    (),      ());

    //others
    QUERY_FUNC(0, GETERROR,              GetError,              (),      ());
    QUERY_FUNC(1, GETFLOATV,             GetFloatv,             (),      ());
    QUERY_FUNC(1, GETUNSIGNEDBYTEVEXT,   GetUnsignedBytevEXT,   (),      ());
    QUERY_FUNC(1, GETUNSIGNEDBYTEI_VEXT, GetUnsignedBytei_vEXT, (),      ());
    QUERY_FUNC(1, GETSTRINGI,            GetStringi,            (),      ());
    QUERY_FUNC(1, ISENABLED,             IsEnabled,             (),      ());
    QUERY_FUNC(1, ENABLE,                Enable,                (),      ());
    QUERY_FUNC(1, DISABLE,               Disable,               (),      ());
    QUERY_FUNC(1, FINISH,                Finish,                (),      ());
    QUERY_FUNC(1, FLUSH,                 Flush,                 (),      ());
    QUERY_FUNC(1, DEPTHFUNC,             DepthFunc,             (),      ());
    QUERY_FUNC(1, CULLFACE,              CullFace,              (),      ());
    QUERY_FUNC(1, FRONTFACE,             FrontFace,             (),      ());
    QUERY_FUNC(1, VIEWPORT,              Viewport,              (),      ());
    QUERY_FUNC(1, VIEWPORTARRAYV,        ViewportArrayv,        (),      (OES, NV));
    QUERY_FUNC(1, VIEWPORTINDEXEDF,      ViewportIndexedf,      (),      (OES, NV));
    QUERY_FUNC(1, VIEWPORTINDEXEDFV,     ViewportIndexedfv,     (),      (OES, NV));
    QUERY_FUNC(1, CLIPCONTROL,           ClipControl,           (),      (EXT));
    QUERY_FUNC(1, MEMORYBARRIER,         MemoryBarrier,         (, EXT), ());
    
    Extensions = GetExtensions();
    SupportDebug            = DebugMessageCallback != nullptr || DebugMessageCallbackAMD != nullptr;
    SupportClipControl      = ClipControl != nullptr;
    if (ContextType == GLType::ES)
    {
        //if (!Extensions.Has("GL_EXT_texture_storage"))
        //    TextureStorage1DEXT_ = nullptr, TextureStorage2DEXT_ = nullptr, TextureStorage3DEXT_ = nullptr;
        SupportSRGB             = Version >= 30 || Extensions.Has("GL_EXT_sRGB");
        SupportSRGBFrameBuffer  = host.SupportSRGBFrameBuffer && Extensions.Has("GL_EXT_sRGB_write_control");
        SupportGeometryShader   = Extensions.Has("GL_EXT_geometry_shader") || Extensions.Has("GL_INTEL_geometry_shader") || Extensions.Has("GL_OES_geometry_shader");
        SupportComputeShader    = Version >= 31 || Extensions.Has("GL_ARB_compute_shader");
        SupportTessShader       = Extensions.Has("GL_EXT_tessellation_shader") || Extensions.Has("GL_INTEL_tessellation_shader") || Extensions.Has("GL_OES_tessellation_shader");
        SupportBindlessTexture  = Extensions.Has("GL_IMG_bindless_texture") || Extensions.Has("GL_NV_bindless_texture");
        SupportImageLoadStore   = Version >= 31 || Extensions.Has("GL_ARB_shader_image_load_store");
        SupportSubroutine       = Extensions.Has("GL_ARB_shader_subroutine");
        SupportIndirectDraw     = Version >= 31 || Extensions.Has("GL_ARB_draw_indirect");
        SupportInstanceDraw     = Version >= 30 || Extensions.Has("GL_EXT_draw_instanced");
        SupportBaseInstance     = Extensions.Has("GL_EXT_base_instance") || Extensions.Has("GL_ANGLE_base_vertex_base_instance");
        SupportLayeredRender    = Version >= 32 || SupportGeometryShader;
        SupportVSMultiLayer     = false;
    }
    else
    {
        SupportSRGB             = Version >= 21 || Extensions.Has("GL_EXT_texture_sRGB");
        SupportSRGBFrameBuffer  = host.SupportSRGBFrameBuffer && (Extensions.Has("GL_ARB_framebuffer_sRGB") || Extensions.Has("GL_EXT_framebuffer_sRGB"));
        SupportGeometryShader   = Version >= 32 || Extensions.Has("GL_ARB_geometry_shader4");
        SupportComputeShader    = Extensions.Has("GL_ARB_compute_shader");
        SupportTessShader       = Extensions.Has("GL_ARB_tessellation_shader");
        SupportBindlessTexture  = Extensions.Has("GL_ARB_bindless_texture") || Extensions.Has("GL_NV_bindless_texture");
        SupportImageLoadStore   = Extensions.Has("GL_ARB_shader_image_load_store") || Extensions.Has("GL_EXT_shader_image_load_store");
        SupportSubroutine       = Extensions.Has("GL_ARB_shader_subroutine");
        SupportIndirectDraw     = Version >= 40 || Extensions.Has("GL_ARB_draw_indirect");
        SupportInstanceDraw     = Version >= 31 || Extensions.Has("GL_ARB_draw_instanced") || Extensions.Has("GL_EXT_draw_instanced");
        SupportBaseInstance     = Extensions.Has("GL_ARB_base_instance");
        SupportLayeredRender    = SupportGeometryShader;
        SupportVSMultiLayer     = Extensions.Has("GL_ARB_shader_viewport_layer_array") || Extensions.Has("GL_AMD_vertex_shader_layer");
    }
    
    XCompDevice = host.GetCommonDevice();
    if (!XCompDevice)
    {
        const auto luid = GetLUID();
        const auto uuid = GetUUID();
        const auto& devs = xcomp::ProbeDevice();
        XCompDevice = devs.LocateExactDevice(luid ? &*luid : nullptr, uuid ? &*uuid : nullptr, nullptr, {});
        if (!XCompDevice)
            XCompDevice = devs.TryLocateDevice(nullptr, nullptr, RendererString);
    }

    GetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS,         &MaxUBOUnits);
    GetIntegerv(GL_MAX_IMAGE_UNITS,                     &MaxImageUnits);
    GetIntegerv(GL_MAX_COLOR_ATTACHMENTS,               &MaxColorAttachment);
    GetIntegerv(GL_MAX_DRAW_BUFFERS,                    &MaxDrawBuffers);
    GetIntegerv(GL_MAX_LABEL_LENGTH,                    &MaxLabelLen);
    GetIntegerv(GL_MAX_DEBUG_MESSAGE_LENGTH,            &MaxMessageLen);
    if (SupportImageLoadStore)
        GetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &MaxTextureUnits);
    if (SupportLayeredRender)
        GetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &MaxFBOLayers);
    
#undef QUERY_FUNC
#undef QUERY_FUNC_
#undef SFX_STR
#undef FUNC_TYPE_CHECK
#undef FUNC_TYPE_CHECK_
}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


#define CALL_EXISTS(func, ...) if (func) { return func(__VA_ARGS__); }


void CtxFuncs::NamedBufferStorage(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) const
{
    CALL_EXISTS(NamedBufferStorage_, buffer, size, data, flags)
    {
        BindBuffer(target, buffer);
        BufferStorage_(target, size, data, flags);
    }
}
void CtxFuncs::NamedBufferData(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) const
{
    CALL_EXISTS(NamedBufferData_, buffer, size, data, usage)
    {
        BindBuffer(target, buffer);
        BufferData_(target, size, data, usage);
    }
}
void CtxFuncs::NamedBufferSubData(GLenum target, GLuint buffer, GLintptr offset, GLsizeiptr size, const void* data) const
{
    CALL_EXISTS(NamedBufferSubData_, buffer, offset, size, data)
    {
        BindBuffer(target, buffer);
        BufferSubData_(target, offset, size, data);
    }
}
void* CtxFuncs::MapNamedBuffer(GLenum target, GLuint buffer, GLenum access) const
{
    CALL_EXISTS(MapNamedBuffer_, buffer, access)
    {
        BindBuffer(target, buffer);
        return MapBuffer_(target, access);
    }
}
GLboolean CtxFuncs::UnmapNamedBuffer(GLenum target, GLuint buffer) const
{
    CALL_EXISTS(UnmapNamedBuffer_, buffer)
    {
        BindBuffer(target, buffer);
        return UnmapBuffer_(target);
    }
}


struct VAOBinder : public common::NonCopyable
{
    common::SpinLocker::ScopeType Lock;
    const CtxFuncs& DSA;
    bool Changed;
    VAOBinder(const CtxFuncs* dsa, GLuint newVAO) noexcept :
        Lock(dsa->DataLock.LockScope()), DSA(*dsa), Changed(false)
    {
        if (newVAO != DSA.VAO)
            Changed = true, DSA.BindVertexArray_(newVAO);
    }
    ~VAOBinder()
    {
        if (Changed)
            DSA.BindVertexArray_(DSA.VAO);
    }
};
void CtxFuncs::RefreshVAOState() const
{
    const auto lock = DataLock.LockScope();
    GetIntegerv(GL_VERTEX_ARRAY_BINDING, reinterpret_cast<GLint*>(&VAO));
}
void CtxFuncs::BindVertexArray(GLuint vaobj) const
{
    const auto lock = DataLock.LockScope();
    BindVertexArray_(vaobj);
    VAO = vaobj;
}
void CtxFuncs::EnableVertexArrayAttrib(GLuint vaobj, GLuint index) const
{
    CALL_EXISTS(EnableVertexArrayAttrib_, vaobj, index)
    {
        BindVertexArray_(vaobj); // ensure be in binding
        EnableVertexAttribArray_(index);
    }
}
void CtxFuncs::VertexAttribPointer(GLuint index, GLint size, GLenum type, bool normalized, GLsizei stride, size_t offset) const
{
    const auto pointer = reinterpret_cast<const void*>(uintptr_t(offset));
    VertexAttribPointer_(index, size, type, normalized ? GL_TRUE : GL_FALSE, stride, pointer);
}
void CtxFuncs::VertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const
{
    const auto pointer = reinterpret_cast<const void*>(uintptr_t(offset));
    VertexAttribIPointer_(index, size, type, stride, pointer);
}
void CtxFuncs::VertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const
{
    const auto pointer = reinterpret_cast<const void*>(uintptr_t(offset));
    VertexAttribLPointer_(index, size, type, stride, pointer);
}


void CtxFuncs::DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, uint32_t instancecount, uint32_t baseinstance) const
{
    if (baseinstance != 0)
        CALL_EXISTS(DrawArraysInstancedBaseInstance_, mode, first, count, static_cast<GLsizei>(instancecount), baseinstance)
    CALL_EXISTS(DrawArraysInstanced_, mode, first, count, static_cast<GLsizei>(instancecount)) // baseInstance ignored
    {
        for (uint32_t i = 0; i < instancecount; i++)
        {
            DrawArrays(mode, first, count);
        }
    }
}
void CtxFuncs::DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, uint32_t instancecount, uint32_t baseinstance) const
{
    if (baseinstance != 0)
        CALL_EXISTS(DrawElementsInstancedBaseInstance_, mode, count, type, indices, static_cast<GLsizei>(instancecount), baseinstance)
    CALL_EXISTS(DrawElementsInstanced_, mode, count, type, indices, static_cast<GLsizei>(instancecount)) // baseInstance ignored
    {
        for (uint32_t i = 0; i < instancecount; i++)
        {
            DrawElements(mode, count, type, indices);
        }
    }
}
void CtxFuncs::MultiDrawArraysIndirect(GLenum mode, const oglIndirectBuffer_& indirect, GLint offset, GLsizei drawcount) const
{
    if (MultiDrawArraysIndirect_)
    {
        BindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect.BufferID); //IBO not included in VAO
        const auto pointer = reinterpret_cast<const void*>(uintptr_t(sizeof(oglIndirectBuffer_::DrawArraysIndirectCommand) * offset));
        MultiDrawArraysIndirect_(mode, pointer, drawcount, 0);
    }
    else if (DrawArraysInstancedBaseInstance_)
    {
        const auto& cmd = indirect.GetArrayCommands();
        for (GLsizei i = offset; i < drawcount; i++)
        {
            DrawArraysInstancedBaseInstance_(mode, cmd[i].first, cmd[i].count, cmd[i].instanceCount, cmd[i].baseInstance);
        }
    }
    else if (DrawArraysInstanced_)
    {
        const auto& cmd = indirect.GetArrayCommands();
        for (GLsizei i = offset; i < drawcount; i++)
        {
            DrawArraysInstanced_(mode, cmd[i].first, cmd[i].count, cmd[i].instanceCount); // baseInstance ignored
        }
    }
    else
    {
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"no avaliable implementation for MultiDrawArraysIndirect");
    }
}
void CtxFuncs::MultiDrawElementsIndirect(GLenum mode, GLenum type, const oglIndirectBuffer_& indirect, GLint offset, GLsizei drawcount) const
{
    if (MultiDrawElementsIndirect_)
    {
        BindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect.BufferID); //IBO not included in VAO
        const auto pointer = reinterpret_cast<const void*>(uintptr_t(sizeof(oglIndirectBuffer_::DrawArraysIndirectCommand) * offset));
        MultiDrawElementsIndirect_(mode, type, pointer, drawcount, 0);
    }
    else if (DrawElementsInstancedBaseVertexBaseInstance_)
    {
        const auto& cmd = indirect.GetElementCommands();
        for (GLsizei i = offset; i < drawcount; i++)
        {
            const auto pointer = reinterpret_cast<const void*>(uintptr_t(cmd[i].firstIndex));
            DrawElementsInstancedBaseVertexBaseInstance_(mode, cmd[i].count, type, pointer, cmd[i].instanceCount, cmd[i].baseVertex, cmd[i].baseInstance);
        }
    }
    else if (DrawElementsInstancedBaseInstance_)
    {
        const auto& cmd = indirect.GetElementCommands();
        for (GLsizei i = offset; i < drawcount; i++)
        {
            const auto pointer = reinterpret_cast<const void*>(uintptr_t(cmd[i].firstIndex));
            DrawElementsInstancedBaseInstance_(mode, cmd[i].count, type, pointer, cmd[i].instanceCount, cmd[i].baseInstance); // baseInstance ignored
        }
    }
    else if (DrawElementsInstanced_)
    {
        const auto& cmd = indirect.GetElementCommands();
        for (GLsizei i = offset; i < drawcount; i++)
        {
            const auto pointer = reinterpret_cast<const void*>(uintptr_t(cmd[i].firstIndex));
            DrawElementsInstanced_(mode, cmd[i].count, type, pointer, cmd[i].instanceCount); // baseInstance & baseVertex ignored
        }
    }
    else
    {
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"no avaliable implementation for MultiDrawElementsIndirect");
    }
}


void CtxFuncs::CreateTextures(GLenum target, GLsizei n, GLuint* textures) const
{
    CALL_EXISTS(CreateTextures_, target, n, textures)
    {
        GenTextures(n, textures);
        ActiveTexture(GL_TEXTURE0);
        for (GLsizei i = 0; i < n; ++i)
            BindTexture(target, textures[i]);
        BindTexture(target, 0);
    }
}
void CtxFuncs::BindTextureUnit(GLuint unit, GLuint texture, GLenum target) const
{
    CALL_EXISTS(BindTextureUnit_,                   unit,         texture)
    CALL_EXISTS(BindMultiTextureEXT_, GL_TEXTURE0 + unit, target, texture)
    {
        ActiveTexture(GL_TEXTURE0 + unit);
        BindTexture(target, texture);
    }
}
void CtxFuncs::TextureBuffer(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer) const
{
    CALL_EXISTS(TextureBuffer_,    target,         internalformat, buffer)
    CALL_EXISTS(TextureBufferEXT_, target, target, internalformat, buffer)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        TexBuffer_(target, internalformat, buffer);
        BindTexture(target, 0);
    }
}
void CtxFuncs::GenerateTextureMipmap(GLuint texture, GLenum target) const
{
    CALL_EXISTS(GenerateTextureMipmap_,    texture)
    CALL_EXISTS(GenerateTextureMipmapEXT_, texture, target)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        GenerateMipmap_(target);
    }
}
void CtxFuncs::TextureParameteri(GLuint texture, GLenum target, GLenum pname, GLint param) const
{
    CALL_EXISTS(TextureParameteri_,    texture,         pname, param)
    CALL_EXISTS(TextureParameteriEXT_, texture, target, pname, param)
    {
        BindTexture(target, texture);
        TexParameteri(target, pname, param);
    }
}
void CtxFuncs::TextureSubImage1D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(TextureSubImage1D_,    texture,         level, xoffset, width, format, type, pixels)
    CALL_EXISTS(TextureSubImage1DEXT_, texture, target, level, xoffset, width, format, type, pixels)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        TexSubImage1D_(target, level, xoffset, width, format, type, pixels);
    }
}
void CtxFuncs::TextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(TextureSubImage2D_,    texture,         level, xoffset, yoffset, width, height, format, type, pixels)
    CALL_EXISTS(TextureSubImage2DEXT_, texture, target, level, xoffset, yoffset, width, height, format, type, pixels)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        TexSubImage2D_(target, level, xoffset, yoffset, width, height, format, type, pixels);
    }
}
void CtxFuncs::TextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(TextureSubImage3D_,    texture,         level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels)
    CALL_EXISTS(TextureSubImage3DEXT_, texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        TexSubImage3D_(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    }
}
void CtxFuncs::TextureImage1D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(TextureImage1DEXT_, texture, target, level, internalformat, width, border, format, type, pixels)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        TexImage1D(target, level, internalformat, width, border, format, type, pixels);
    }
}
void CtxFuncs::TextureImage2D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(TextureImage2DEXT_, texture, target, level, internalformat, width, height, border, format, type, pixels)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        TexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    }
}
void CtxFuncs::TextureImage3D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(TextureImage3DEXT_, texture, target, level, internalformat, width, height, depth, border, format, type, pixels)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        TexImage3D_(target, level, internalformat, width, height, depth, border, format, type, pixels);
    }
}
void CtxFuncs::CompressedTextureSubImage1D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(CompressedTextureSubImage1D_,    texture,         level, xoffset, width, format, imageSize, data)
    CALL_EXISTS(CompressedTextureSubImage1DEXT_, texture, target, level, xoffset, width, format, imageSize, data)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        CompressedTexSubImage1D_(target, level, xoffset, width, format, imageSize, data);
    }
}
void CtxFuncs::CompressedTextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(CompressedTextureSubImage2D_,    texture,         level, xoffset, yoffset, width, height, format, imageSize, data)
    CALL_EXISTS(CompressedTextureSubImage2DEXT_, texture, target, level, xoffset, yoffset, width, height, format, imageSize, data)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        CompressedTexSubImage2D_(target, level, xoffset, yoffset, width, height, format, imageSize, data);
    }
}
void CtxFuncs::CompressedTextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(CompressedTextureSubImage3D_,    texture,         level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data)
    CALL_EXISTS(CompressedTextureSubImage3DEXT_, texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        CompressedTexSubImage3D_(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    }
}
void CtxFuncs::CompressedTextureImage1D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(CompressedTextureImage1DEXT_, texture, target, level, internalformat, width, border, imageSize, data)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        CompressedTexImage1D_(target, level, internalformat, width, border, imageSize, data);
    }
}
void CtxFuncs::CompressedTextureImage2D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(CompressedTextureImage2DEXT_, texture, target, level, internalformat, width, height, border, imageSize, data)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        CompressedTexImage2D_(target, level, internalformat, width, height, border, imageSize, data);
    }
}
void CtxFuncs::CompressedTextureImage3D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(CompressedTextureImage3DEXT_, texture, target, level, internalformat, width, height, depth, border, imageSize, data)
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        CompressedTexImage3D_(target, level, internalformat, width, height, depth, border, imageSize, data);
    }
}
void CtxFuncs::TextureStorage1D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) const
{
    CALL_EXISTS(TextureStorage1D_,    texture,         levels, internalformat, width)
    if (TextureStorage1DEXT_ && !(ContextType == GLType::ES && Version >= 30))
        return TextureStorage1DEXT_(texture, target, levels, internalformat, width);
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        TexStorage1D_(target, levels, internalformat, width);
    }
}
void CtxFuncs::TextureStorage2D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) const
{
    CALL_EXISTS(TextureStorage2D_,    texture,         levels, internalformat, width, height)
    if (TextureStorage2DEXT_ && !(ContextType == GLType::ES && Version >= 30))
        return TextureStorage2DEXT_(texture, target, levels, internalformat, width, height);
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        TexStorage2D_(target, levels, internalformat, width, height);
    }
}
void CtxFuncs::TextureStorage3D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) const
{
    CALL_EXISTS(TextureStorage3D_, texture, levels, internalformat, width, height, depth)
    if (TextureStorage3DEXT_ && !(ContextType == GLType::ES && Version >= 30))
        return TextureStorage3DEXT_(texture, target, levels, internalformat, width, height, depth);
    {
        ActiveTexture(GL_TEXTURE0);
        BindTexture(target, texture);
        TexStorage3D_(target, levels, internalformat, width, height, depth);
    }
}
void CtxFuncs::GetTextureLevelParameteriv(GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params) const
{
    CALL_EXISTS(GetTextureLevelParameteriv_,    texture,         level, pname, params)
    CALL_EXISTS(GetTextureLevelParameterivEXT_, texture, target, level, pname, params)
    {
        BindTexture(target, texture);
        GetTexLevelParameteriv_(target, level, pname, params);
    }
}
void CtxFuncs::GetTextureParameteriv(GLuint texture, GLenum target, GLenum pname, GLint* params) const
{
    CALL_EXISTS(GetTextureParameteriv_, texture, pname, params)
    CALL_EXISTS(GetTextureParameterivEXT_, texture, target, pname, params)
    {
        BindTexture(target, texture);
        GetTexParameteriv_(target, pname, params);
    }
}
void CtxFuncs::GetTextureImage(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, size_t bufSize, void* pixels) const
{
    CALL_EXISTS(GetTextureImage_,    texture,         level, format, type, bufSize > INT32_MAX ? INT32_MAX : static_cast<GLsizei>(bufSize), pixels)
    CALL_EXISTS(GetTextureImageEXT_, texture, target, level, format, type, pixels)
    if (GetTexImage)
    {
        BindTexture(target, texture);
        GetTexImage(target, level, format, type, pixels);
        return;
    }
    {
        GLuint fbo = 0;
        GenFramebuffers_(1, &fbo);
        const auto lock = DataLock.LockScope();
        const auto backup = this->ReadFBO;
        constexpr auto FBTarget = GL_READ_FRAMEBUFFER;
        constexpr auto FBAttach = GL_COLOR_ATTACHMENT0 + 0;
        BindFramebuffer_(FBTarget, fbo);
        BindTexture(target, texture);
        const bool isLayeres = target != GL_TEXTURE_1D && target != GL_TEXTURE_2D;
        GLint width = 0, height = 0, layers = 1;
        GetTexLevelParameteriv_(target, level, GL_TEXTURE_WIDTH,  &width);
        GetTexLevelParameteriv_(target, level, GL_TEXTURE_HEIGHT, &height);
        if (isLayeres)
            GetTexLevelParameteriv_(target, level, GL_TEXTURE_DEPTH, &layers);
        for (GLint layer = 0; layer < layers; ++layer)
        {
            const auto ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pixels) + (bufSize / layers * layer));
            if (isLayeres)
                FramebufferTextureLayer_(FBTarget, FBAttach, texture, level, layer);
            else if (FramebufferTexture_)
                FramebufferTexture_(FBTarget, FBAttach, texture, level);
            else
            {
                switch (target)
                {
                case GL_TEXTURE_1D: FramebufferTexture1D_(FBTarget, FBAttach, target, texture, level); break;
                case GL_TEXTURE_2D: FramebufferTexture2D_(FBTarget, FBAttach, target, texture, level); break;
                default:            COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"should not enter");
                }
            }
            const auto status = CheckFramebufferStatus_(FBTarget);
            if (status != GL_FRAMEBUFFER_COMPLETE)
                oglLog().Warning("incomplete fbo.\n");
            ReadPixels_(0, 0, width, height, format, type, ptr);
        }
        BindFramebuffer_(FBTarget, backup);
        DeleteFramebuffers(1, &fbo);
    }
}
void CtxFuncs::GetCompressedTextureImage(GLuint texture, GLenum target, GLint level, size_t bufSize, void* img) const
{
    CALL_EXISTS(GetCompressedTextureImage_,    texture,         level, bufSize > INT32_MAX ? INT32_MAX : static_cast<GLsizei>(bufSize), img)
    CALL_EXISTS(GetCompressedTextureImageEXT_, texture, target, level, img)
    {
        BindTexture(target, texture);
        GetCompressedTexImage_(target, level, img);
    }
}


void CtxFuncs::CreateRenderbuffers(GLsizei n, GLuint* renderbuffers) const
{
    CALL_EXISTS(CreateRenderbuffers_, n, renderbuffers)
    {
        GenRenderbuffers_(n, renderbuffers);
        for (GLsizei i = 0; i < n; ++i)
            BindRenderbuffer_(GL_RENDERBUFFER, renderbuffers[i]);
    }
}
void CtxFuncs::NamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) const
{
    CALL_EXISTS(NamedRenderbufferStorage_, renderbuffer, internalformat, width, height)
    {
        BindRenderbuffer_(GL_RENDERBUFFER, renderbuffer);
        RenderbufferStorage_(GL_RENDERBUFFER, internalformat, width, height);
    }
}
void CtxFuncs::NamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) const
{
    CALL_EXISTS(NamedRenderbufferStorageMultisample_, renderbuffer, samples, internalformat, width, height)
    {
        BindRenderbuffer_(GL_RENDERBUFFER, renderbuffer);
        RenderbufferStorageMultisample_(GL_RENDERBUFFER, samples, internalformat, width, height);
    }
}
void CtxFuncs::NamedRenderbufferStorageMultisampleCoverage(GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) const
{
    CALL_EXISTS(NamedRenderbufferStorageMultisampleCoverageEXT_, renderbuffer, coverageSamples, colorSamples, internalformat, width, height)
    if (RenderbufferStorageMultisampleCoverageNV_)
    {
        BindRenderbuffer_(GL_RENDERBUFFER, renderbuffer);
        RenderbufferStorageMultisampleCoverageNV_(GL_RENDERBUFFER, coverageSamples, colorSamples, internalformat, width, height);
    }
    else
    {
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"no avaliable implementation for RenderbufferStorageMultisampleCoverage");
    }
}


struct FBOBinder : public common::NonCopyable
{
    common::SpinLocker::ScopeType Lock;
    const CtxFuncs& DSA;
    bool ChangeRead, ChangeDraw;
    FBOBinder(const CtxFuncs* dsa) noexcept :
        Lock(dsa->DataLock.LockScope()), DSA(*dsa), ChangeRead(true), ChangeDraw(true)
    { }
    FBOBinder(const CtxFuncs* dsa, const std::pair<GLuint, GLuint> newFBO) noexcept :
        Lock(dsa->DataLock.LockScope()), DSA(*dsa), ChangeRead(false), ChangeDraw(false)
    {
        if (DSA.ReadFBO != newFBO.first)
            ChangeRead = true, DSA.BindFramebuffer_(GL_READ_FRAMEBUFFER, newFBO.first);
        if (DSA.DrawFBO != newFBO.second)
            ChangeDraw = true, DSA.BindFramebuffer_(GL_DRAW_FRAMEBUFFER, newFBO.second);
    }
    FBOBinder(const CtxFuncs* dsa, const GLenum target, const GLuint newFBO) noexcept :
        Lock(dsa->DataLock.LockScope()), DSA(*dsa), ChangeRead(false), ChangeDraw(false)
    {
        if (target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
        {
            if (DSA.ReadFBO != newFBO)
                ChangeRead = true, DSA.BindFramebuffer_(GL_READ_FRAMEBUFFER, newFBO);
        }
        if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
        {
            if (DSA.DrawFBO != newFBO)
                ChangeDraw = true, DSA.BindFramebuffer_(GL_DRAW_FRAMEBUFFER, newFBO);
        }
    }
    ~FBOBinder()
    {
        if (ChangeRead)
            DSA.BindFramebuffer_(GL_READ_FRAMEBUFFER, DSA.ReadFBO);
        if (ChangeDraw)
            DSA.BindFramebuffer_(GL_DRAW_FRAMEBUFFER, DSA.DrawFBO);
    }
};
void CtxFuncs::RefreshFBOState() const
{
    const auto lock = DataLock.LockScope();
    GetIntegerv(GL_READ_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&ReadFBO));
    GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&DrawFBO));
}
void CtxFuncs::CreateFramebuffers(GLsizei n, GLuint* framebuffers) const
{
    CALL_EXISTS(CreateFramebuffers_, n, framebuffers)
    {
        GenFramebuffers_(n, framebuffers);
        const auto backup = FBOBinder(this);
        for (GLsizei i = 0; i < n; ++i)
            BindFramebuffer_(GL_READ_FRAMEBUFFER, framebuffers[i]);
    }
}
void CtxFuncs::BindFramebuffer(GLenum target, GLuint framebuffer) const
{
    const auto lock = DataLock.LockScope();
    BindFramebuffer_(target, framebuffer);
    if (target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
        ReadFBO = framebuffer;
    if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
        DrawFBO = framebuffer;
}
void CtxFuncs::BlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) const
{
    CALL_EXISTS(BlitNamedFramebuffer_, readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
    {
        const auto backup = FBOBinder(this, { readFramebuffer, drawFramebuffer });
        BlitFramebuffer_(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    }
}
void CtxFuncs::InvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum* attachments) const
{
    CALL_EXISTS(InvalidateNamedFramebufferData_, framebuffer, numAttachments, attachments)
    const auto invalidator = InvalidateFramebuffer_ ? InvalidateFramebuffer_ : DiscardFramebufferEXT_;
    if (invalidator)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        invalidator(GL_DRAW_FRAMEBUFFER, numAttachments, attachments);
    }
}
void CtxFuncs::NamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) const
{
    CALL_EXISTS(NamedFramebufferRenderbuffer_, framebuffer, attachment, renderbuffertarget, renderbuffer)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        FramebufferRenderbuffer_(GL_DRAW_FRAMEBUFFER, attachment, renderbuffertarget, renderbuffer);
    }
}
void CtxFuncs::NamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) const
{
    CALL_EXISTS(NamedFramebufferTexture_, framebuffer, attachment, texture, level)
    if (FramebufferTexture_)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        FramebufferTexture_(GL_DRAW_FRAMEBUFFER, attachment, texture, level);
    }
    else
    {
        switch (textarget)
        {
        case GL_TEXTURE_1D:
            CALL_EXISTS(NamedFramebufferTexture1DEXT_, framebuffer, attachment, textarget, texture, level)
            {
                const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
                FramebufferTexture1D_(GL_DRAW_FRAMEBUFFER, attachment, textarget, texture, level);
            }
            break;
        case GL_TEXTURE_2D:
            CALL_EXISTS(NamedFramebufferTexture2DEXT_, framebuffer, attachment, textarget, texture, level)
            {
                const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
                FramebufferTexture2D_(GL_DRAW_FRAMEBUFFER, attachment, textarget, texture, level);
            }
            break;
        default:
            COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"unsupported textarget with calling FramebufferTexture");
        }
    }
}
void CtxFuncs::NamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer) const
{
    CALL_EXISTS(NamedFramebufferTextureLayer_, framebuffer, attachment, texture, level, layer)
    if (FramebufferTextureLayer_)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        FramebufferTextureLayer_(GL_DRAW_FRAMEBUFFER, attachment, texture, level, layer);
    }
    else
    {
        switch (textarget)
        {
        case GL_TEXTURE_3D:
            CALL_EXISTS(NamedFramebufferTexture3DEXT_, framebuffer, attachment, textarget, texture, level, layer)
            {
                const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
                FramebufferTexture3D_(GL_DRAW_FRAMEBUFFER, attachment, textarget, texture, level, layer);
            }
            break;
        default:
            COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"unsupported textarget with calling FramebufferTextureLayer");
        }
    }
}
GLenum CtxFuncs::CheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) const
{
    CALL_EXISTS(CheckNamedFramebufferStatus_, framebuffer, target)
    {
        const auto backup = FBOBinder(this, target, framebuffer);
        return CheckFramebufferStatus_(target);
    }
}
void CtxFuncs::GetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params) const
{
    CALL_EXISTS(GetNamedFramebufferAttachmentParameteriv_, framebuffer, attachment, pname, params)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        GetFramebufferAttachmentParameteriv_(GL_DRAW_FRAMEBUFFER, attachment, pname, params);
    }
}
void CtxFuncs::ClearDepth(GLclampd d) const
{
    CALL_EXISTS(ClearDepth_, d)
    CALL_EXISTS(ClearDepthf_, static_cast<GLclampf>(d))
    {
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"unsupported textarget with calling ClearDepth");
    }
}
void CtxFuncs::ClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value) const
{
    CALL_EXISTS(ClearNamedFramebufferiv_, framebuffer, buffer, drawbuffer, value)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        ClearBufferiv_(buffer, drawbuffer, value);
    }
}
void CtxFuncs::ClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value) const
{
    CALL_EXISTS(ClearNamedFramebufferuiv_, framebuffer, buffer, drawbuffer, value)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        ClearBufferuiv_(buffer, drawbuffer, value);
    }
}
void CtxFuncs::ClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value) const
{
    CALL_EXISTS(ClearNamedFramebufferfv_, framebuffer, buffer, drawbuffer, value)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        ClearBufferfv_(buffer, drawbuffer, value);
    }
}
void CtxFuncs::ClearNamedFramebufferDepthStencil(GLuint framebuffer, GLfloat depth, GLint stencil) const
{
    CALL_EXISTS(ClearNamedFramebufferfi_, framebuffer, GL_DEPTH_STENCIL, 0, depth, stencil)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        ClearBufferfi_(GL_DEPTH_STENCIL, 0, depth, stencil);
    }
}


void CtxFuncs::SetObjectLabel(GLenum identifier, GLuint id, std::u16string_view name) const
{
    if (ObjectLabel_)
    {
        const auto str = common::str::to_u8string(name, common::str::Encoding::UTF16LE);
        ObjectLabel_(identifier, id,
            static_cast<GLsizei>(std::min<size_t>(str.size(), MaxLabelLen)),
            reinterpret_cast<const GLchar*>(str.c_str()));
    }
    else if (LabelObjectEXT_)
    {
        GLenum type;
        switch (identifier)
        {
        case GL_BUFFER:             type = GL_BUFFER_OBJECT_EXT;            break;
        case GL_SHADER:             type = GL_SHADER_OBJECT_EXT;            break;
        case GL_PROGRAM:            type = GL_PROGRAM_OBJECT_EXT;           break;
        case GL_VERTEX_ARRAY:       type = GL_VERTEX_ARRAY_OBJECT_EXT;      break;
        case GL_QUERY:              type = GL_QUERY_OBJECT_EXT;             break;
        case GL_PROGRAM_PIPELINE:   type = GL_PROGRAM_PIPELINE_OBJECT_EXT;  break;
        case GL_TRANSFORM_FEEDBACK: type = GL_TRANSFORM_FEEDBACK;           break;
        case GL_SAMPLER:            type = GL_SAMPLER;                      break;
        case GL_TEXTURE:            type = GL_TEXTURE;                      break;
        case GL_RENDERBUFFER:       type = GL_RENDERBUFFER;                 break;
        case GL_FRAMEBUFFER:        type = GL_FRAMEBUFFER;                  break;
        default:                    return;
        }
        const auto str = common::str::to_u8string(name, common::str::Encoding::UTF16LE);
        LabelObjectEXT_(type, id,
            static_cast<GLsizei>(str.size()),
            reinterpret_cast<const GLchar*>(str.c_str()));
    }
}
void CtxFuncs::SetObjectLabel(GLsync sync, std::u16string_view name) const
{
    if (ObjectPtrLabel_)
    {
        const auto str = common::str::to_u8string(name, common::str::Encoding::UTF16LE);
        ObjectPtrLabel_(sync,
            static_cast<GLsizei>(std::min<size_t>(str.size(), MaxLabelLen)),
            reinterpret_cast<const GLchar*>(str.c_str()));
    }
}
void CtxFuncs::PushDebugGroup(GLenum source, GLuint id, std::u16string_view message) const
{
    if (PushDebugGroup_)
    {
        const auto str = common::str::to_u8string(message, common::str::Encoding::UTF16LE);
        PushDebugGroup_(source, id,
            static_cast<GLsizei>(std::min<size_t>(str.size(), MaxMessageLen)),
            reinterpret_cast<const GLchar*>(str.c_str()));
    }
    else if (PushGroupMarkerEXT_)
    {
        const auto str = common::str::to_u8string(message, common::str::Encoding::UTF16LE);
        PushGroupMarkerEXT_(static_cast<GLsizei>(str.size()), reinterpret_cast<const GLchar*>(str.c_str()));
    }
}
void CtxFuncs::PopDebugGroup() const
{
    if (PopDebugGroup_)
        PopDebugGroup_();
    else if (PopGroupMarkerEXT_)
        PopGroupMarkerEXT_();
}
void CtxFuncs::InsertDebugMarker(GLuint id, std::u16string_view name) const
{
    if (DebugMessageInsert_)
    {
        const auto str = common::str::to_u8string(name, common::str::Encoding::UTF16LE);
        DebugMessageInsert_(GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_TYPE_MARKER, id, GL_DEBUG_SEVERITY_NOTIFICATION,
            static_cast<GLsizei>(std::min<size_t>(str.size(), MaxMessageLen)),
            reinterpret_cast<const GLchar*>(str.c_str()));
    }
    else if (InsertEventMarkerEXT)
    {
        const auto str = common::str::to_u8string(name, common::str::Encoding::UTF16LE);
        InsertEventMarkerEXT(static_cast<GLsizei>(str.size()), reinterpret_cast<const GLchar*>(str.c_str()));
    }
    else if (DebugMessageInsertAMD)
    {
        const auto str = common::str::to_u8string(name, common::str::Encoding::UTF16LE);
        DebugMessageInsertAMD(GL_DEBUG_CATEGORY_OTHER_AMD, GL_DEBUG_SEVERITY_LOW_AMD, id,
            static_cast<GLsizei>(std::min<size_t>(str.size(), MaxMessageLen)),
            reinterpret_cast<const GLchar*>(str.c_str()));
    }
}


common::container::FrozenDenseSet<std::string_view> CtxFuncs::GetExtensions() const
{
    if (GetStringi)
    {
        GLint count;
        GetIntegerv(GL_NUM_EXTENSIONS, &count);
        std::vector<std::string_view> exts;
        exts.reserve(count);
        for (GLint i = 0; i < count; i++)
        {
            const GLubyte* ext = GetStringi(GL_EXTENSIONS, i);
            exts.emplace_back(reinterpret_cast<const char*>(ext));
        }
        return exts;
    }
    else
    {
        const GLubyte* exts = GetString(GL_EXTENSIONS);
        return common::str::Split(reinterpret_cast<const char*>(exts), ' ', false);
    }
}
std::optional<std::string_view> CtxFuncs::GetError() const
{
    const auto err = GetError_();
    switch (err)
    {
    case GL_NO_ERROR:                       return {};
    case GL_INVALID_ENUM:                   return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:                  return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:              return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:  return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:                  return "GL_OUT_OF_MEMORY";
    case GL_STACK_UNDERFLOW:                return "GL_STACK_UNDERFLOW";
    case GL_STACK_OVERFLOW:                 return "GL_STACK_OVERFLOW";
    default:                                return "UNKNOWN_ERROR";
    }
}
template<size_t N>
forceinline bool AllZero(const std::array<std::byte, N>& data) noexcept
{
    for (const auto b : data)
        if (b != std::byte(0))
            return false;
    return true;
}
std::optional<std::array<std::byte, 8>> CtxFuncs::GetLUID() const noexcept
{
    if (GetUnsignedBytevEXT && (Extensions.Has("GL_EXT_memory_object_win32") || Extensions.Has("GL_EXT_semaphore_win32")))
    {
        std::array<std::byte, 8> luid = { std::byte(0) };
        GetUnsignedBytevEXT(GL_DEVICE_LUID_EXT, reinterpret_cast<GLubyte*>(luid.data()));
        if (!AllZero(luid))
            return luid;
    }
    return {};
}
std::optional<std::array<std::byte, 16>> CtxFuncs::GetUUID() const noexcept
{
    if (GetUnsignedBytei_vEXT && Extensions.Has("GL_EXT_memory_object"))
    {
        std::array<std::byte, 16> uuid = { std::byte(0) };
        GetUnsignedBytei_vEXT(GL_DEVICE_UUID_EXT, 0, reinterpret_cast<GLubyte*>(uuid.data()));
        if (!AllZero(uuid))
            return uuid;
    }
    return {};
}




}