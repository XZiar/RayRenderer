#include "oglPch.h"
#include "DSAWrapper.h"
#include "oglUtil.h"
#include "oglException.h"
#include "oglContext.h"
#include "oglBuffer.h"


#undef APIENTRY
#if COMMON_OS_WIN
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include <Windows.h>
#   include "GL/wglext.h"
//fucking wingdi defines some terrible macro
#   undef ERROR
#   undef MemoryBarrier
#   pragma message("Compiling OpenGLUtil with wgl-ext[" STRINGIZE(WGL_WGLEXT_VERSION) "]")
#elif COMMON_OS_UNIX
#   include <X11/X.h>
#   include <X11/Xlib.h>
#   include <GL/glx.h>
#   include "GL/glxext.h"
//fucking X11 defines some terrible macro
#   undef Success
#   undef Always
#   undef None
#   undef Bool
#   undef Int
#   pragma message("Compiling OpenGLUtil with glx-ext[" STRINGIZE(GLX_GLXEXT_VERSION) "]")
#else
#   error "unknown os"
#endif


namespace oglu
{
template<typename T>
struct ResourceKeeper
{
    std::map<void*, std::unique_ptr<T>> Map;
    common::SpinLocker Locker;
    auto Lock() { return Locker.LockScope(); }
    T* FindOrCreate(void* key) 
    { 
        const auto lock = Lock();
        if (const auto funcs = common::container::FindInMap(Map, key); funcs)
            return funcs->get();
        else
        {
            const auto [it, ret] = Map.emplace(key, std::make_unique<T>());
            return it->second.get();
        }
    }
    void Remove(void* key)
    {
        const auto lock = Lock();
        Map.erase(key);
    }
};
thread_local const PlatFuncs* PlatFunc = nullptr;
thread_local const DSAFuncs*  DSA      = nullptr;

static auto& GetPlatFuncsMap()
{
    static ResourceKeeper<PlatFuncs> PlatFuncMap;
    return PlatFuncMap;
}
static auto& GetDSAFuncsMap()
{
    static ResourceKeeper<DSAFuncs> DSAFuncMap;
    return DSAFuncMap;
}
static void PreparePlatFuncs(void* hDC)
{
    if (hDC == nullptr)
        PlatFunc = nullptr;
    else
    {
        auto& rmap = GetPlatFuncsMap();
        PlatFunc = rmap.FindOrCreate(hDC);
    }
}
static void PrepareDSAFuncs(void* hRC)
{
    if (hRC == nullptr)
        DSA = nullptr;
    else
    {
        auto& rmap = GetDSAFuncsMap();
        DSA = rmap.FindOrCreate(hRC);
    }
}

#if COMMON_OS_UNIX
static int TmpXErrorHandler(Display* disp, XErrorEvent* evt)
{
    thread_local std::string txtBuf;
    txtBuf.resize(1024, '\0');
    XGetErrorText(disp, evt->error_code, txtBuf.data(), 1024); // return value undocumented, cannot rely on that
    txtBuf.resize(std::char_traits<char>::length(txtBuf.data()));
    oglLog().warning(u"X11 report an error with code[{}][{}]:\t{}\n", evt->error_code, evt->minor_code,
        common::strchset::to_u16string(txtBuf, common::str::Charset::UTF8));
    return 0;
}
#endif


static void ShowQuerySuc (const std::string_view tarName, const std::string_view name, void* ptr)
{
    oglLog().verbose(FMT_STRING(u"Func [{}] uses [{}] ({:p})\n"), tarName, name, (void*)ptr);
}
static void ShowQueryFail(const std::string_view tarName)
{
    oglLog().warning(FMT_STRING(u"Func [{}] not found\n"), tarName);
}

template<typename T>
static void QueryFunc(T& target, const std::string_view tarName, 
    const std::pair<bool, bool> shouldPrint, const std::initializer_list<std::string_view> names)
{
    const auto [printSuc, printFail] = shouldPrint;
    for (const auto& name : names)
    {
#if COMMON_OS_WIN
        const auto ptr = wglGetProcAddress(name.data());
#elif COMMON_OS_MACOS
        const auto ptr = NSGLGetProcAddress(name.data());
#else
        const auto ptr = glXGetProcAddress(reinterpret_cast<const GLubyte*>(name.data()));
#endif
        if (ptr)
        {
            target = reinterpret_cast<T>(ptr);
            if (printSuc)
                ShowQuerySuc(tarName, name, (void*)ptr);
            return;
        }
    }
    target = nullptr;
    if (printFail)
        ShowQueryFail(tarName);
}


static std::atomic<uint32_t>& GetDebugPrintCore() noexcept
{
    static std::atomic<uint32_t> ShouldPrint = 0;
    return ShouldPrint;
}
void SetFuncShouldPrint(const bool printSuc, const bool printFail) noexcept
{
    const uint32_t val = (printSuc ? 0x1u : 0x0u) | (printFail ? 0x2u : 0x0u);
    GetDebugPrintCore() = val;
}
static std::pair<bool, bool> GetFuncShouldPrint() noexcept
{
    const uint32_t val = GetDebugPrintCore();
    const bool printSuc  = (val & 0x1u) == 0x1u;
    const bool printFail = (val & 0x2u) == 0x2u;
    return { printSuc, printFail };
}




#if COMMON_OS_WIN
common::container::FrozenDenseSet<std::string_view> PlatFuncs::GetExtensions(void* hdc) const
{
    const char* exts = nullptr;
    if (wglGetExtensionsStringARB_)
    {
        exts = wglGetExtensionsStringARB_(hdc);
    }
    else if (wglGetExtensionsStringEXT_)
    {
        exts = wglGetExtensionsStringEXT_();
    }
    else
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"no avaliable implementation for wglGetExtensionsString");
    return common::str::Split(exts, ' ', false);
}
#else
common::container::FrozenDenseSet<std::string_view> PlatFuncs::GetExtensions(void* dpy, int screen) const
{
    const char* exts = glXQueryExtensionsString((Display*)dpy, screen);
    return common::str::Split(exts, ' ', false);
}
#endif

PlatFuncs::PlatFuncs()
{
    const auto shouldPrint = GetFuncShouldPrint();

#define WITH_SUFFIX(r, name, i, sfx)    BOOST_PP_COMMA_IF(i) name sfx
#define WITH_SUFFIXS(name, ...) { BOOST_PP_SEQ_FOR_EACH_I(WITH_SUFFIX, name, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) }
#define QUERY_FUNC(name, ...)  QueryFunc(name,          STRINGIZE(name), shouldPrint, WITH_SUFFIXS(#name, __VA_ARGS__))
#define QUERY_FUNC_(name, ...) QueryFunc(PPCAT(name,_), STRINGIZE(name), shouldPrint, WITH_SUFFIXS(#name, __VA_ARGS__))
    
#if COMMON_OS_WIN
    QUERY_FUNC_(wglGetExtensionsStringARB,  "");
    QUERY_FUNC_(wglGetExtensionsStringEXT,  "");
    Extensions = GetExtensions(wglGetCurrentDC());
    QUERY_FUNC_(wglCreateContextAttribsARB, "");
#else
    QUERY_FUNC_(glXGetCurrentDisplay,       "");
    const auto display = glXGetCurrentDisplay_();
    Extensions = GetExtensions(display, DefaultScreen(display));
    QUERY_FUNC_(glXCreateContextAttribsARB, "");
#endif

#if COMMON_OS_WIN
    SupportFlushControl = Extensions.Has("WGL_ARB_context_flush_control");
    SupportSRGB = Extensions.Has("WGL_ARB_framebuffer_sRGB") || Extensions.Has("WGL_EXT_framebuffer_sRGB");
#else
    SupportFlushControl = Extensions.Has("GLX_ARB_context_flush_control");
    SupportSRGB = Extensions.Has("GLX_ARB_framebuffer_sRGB") || Extensions.Has("GLX_EXT_framebuffer_sRGB");
#endif
    

#undef WITH_SUFFIX
#undef WITH_SUFFIXS
#undef QUERY_FUNC
#undef QUERY_FUNC_
}


void* PlatFuncs::GetCurrentDeviceContext()
{
#if COMMON_OS_WIN
    const auto hDC = wglGetCurrentDC();
#else
    const auto hDC = glXGetCurrentDisplay();
    XSetErrorHandler(&TmpXErrorHandler);
#endif
    PreparePlatFuncs(hDC);
    return hDC;
}
void* PlatFuncs::GetCurrentGLContext()
{
    [[maybe_unused]] const auto dummy = GetCurrentDeviceContext();
#if COMMON_OS_WIN
    const auto hRC = wglGetCurrentContext();
#else
    const auto hRC = glXGetCurrentContext();
#endif
    PrepareDSAFuncs(hRC);
    return hRC;
}
void  PlatFuncs::DeleteGLContext([[maybe_unused]] void* hDC, void* hRC)
{
#if COMMON_OS_WIN
    wglDeleteContext((HGLRC)hRC);
#else
    glXDestroyContext((Display*)hDC, (GLXContext)hRC);
#endif
    GetDSAFuncsMap().Remove(hRC);
}
#if COMMON_OS_WIN
bool  PlatFuncs::MakeGLContextCurrent(void* hDC, void* hRC)
{
    const bool ret = wglMakeCurrent((HDC)hDC, (HGLRC)hRC);
#else
bool  PlatFuncs::MakeGLContextCurrent(void* hDC, unsigned long DRW, void* hRC)
{
    const bool ret = glXMakeCurrent((Display*)hDC, DRW, (GLXContext)hRC);
#endif
    if (ret)
    {
        PreparePlatFuncs(hDC);
        PrepareDSAFuncs(hRC);
    }
    return ret;
}
#if COMMON_OS_WIN
uint32_t PlatFuncs::GetSystemError()
{
    return GetLastError();
}
#else
unsigned long PlatFuncs::GetCurrentDrawable()
{
    return glXGetCurrentDrawable();
}
int32_t  PlatFuncs::GetSystemError()
{
    return errno;
}
#endif
std::vector<int32_t> PlatFuncs::GenerateContextAttrib(const uint32_t version, bool needFlushControl)
{
    std::vector<int32_t> ctxAttrb;
#if COMMON_OS_WIN
    ctxAttrb.insert(ctxAttrb.end(),
    {
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB
    });
    constexpr int32_t VerMajor  = WGL_CONTEXT_MAJOR_VERSION_ARB;
    constexpr int32_t VerMinor  = WGL_CONTEXT_MINOR_VERSION_ARB;
    constexpr int32_t FlushFlag = WGL_CONTEXT_RELEASE_BEHAVIOR_ARB;
    constexpr int32_t FlushVal  = WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB;
#else
    ctxAttrb.insert(ctxAttrb.end(),
    {
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB
    });
    constexpr int32_t VerMajor  = GLX_CONTEXT_MAJOR_VERSION_ARB;
    constexpr int32_t VerMinor  = GLX_CONTEXT_MINOR_VERSION_ARB;
    constexpr int32_t FlushFlag = GLX_CONTEXT_RELEASE_BEHAVIOR_ARB;
    constexpr int32_t FlushVal  = GLX_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB;
    needFlushControl = false;
#endif
    if (version != 0)
    {
        ctxAttrb.insert(ctxAttrb.end(),
            {
                VerMajor, static_cast<int32_t>(version / 10),
                VerMinor, static_cast<int32_t>(version % 10),
            });
    }
    if (needFlushControl && PlatFunc->SupportFlushControl)
    {
        ctxAttrb.insert(ctxAttrb.end(), { FlushFlag, FlushVal }); // all the same
    }
    ctxAttrb.push_back(0);
    return ctxAttrb;
}
void* PlatFuncs::CreateNewContext(const oglContext_* prevCtx, const bool isShared, const int32_t* attribs)
{
    if (prevCtx == nullptr || PlatFunc == nullptr)
    {
        return nullptr;
    }
    else
    {
#if defined(_WIN32)
        const auto newHrc = PlatFunc->wglCreateContextAttribsARB_(
            prevCtx->Hdc, isShared ? prevCtx->Hrc : nullptr, attribs);
        return newHrc;
#else
        static int visual_attribs[] =
        {
            GLX_X_RENDERABLE, true,
            GLX_RENDER_TYPE, GLX_RGBA_BIT,
            GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
            GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
            GLX_DOUBLEBUFFER, true,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_ALPHA_SIZE, 8,
            GLX_DEPTH_SIZE, 24,
            GLX_STENCIL_SIZE, 8,
            0
        };
        int num_fbc = 0;
        const auto fbc = glXChooseFBConfig(
            (Display*)prevCtx->Hdc, DefaultScreen((Display*)prevCtx->Hdc), visual_attribs, &num_fbc);
        const auto newHrc = PlatFunc->glXCreateContextAttribsARB_(
            prevCtx->Hdc, fbc[0], isShared ? prevCtx->Hrc : nullptr, true, attribs);
        return newHrc;
#endif
    }
}




DSAFuncs::DSAFuncs()
{
    const auto shouldPrint = GetFuncShouldPrint();

#define WITH_SUFFIX(r, name, i, sfx)    BOOST_PP_COMMA_IF(i) "gl" name sfx
#define WITH_SUFFIXS(name, ...) { BOOST_PP_SEQ_FOR_EACH_I(WITH_SUFFIX, name, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) }
#define QUERY_FUNC(name, ...)   QueryFunc(PPCAT(oglu, name),          STRINGIZE(name), shouldPrint, WITH_SUFFIXS(#name, __VA_ARGS__))
#define QUERY_FUNC_(name, ...)  QueryFunc(PPCAT(PPCAT(oglu, name),_), STRINGIZE(name), shouldPrint, WITH_SUFFIXS(#name, __VA_ARGS__))
#define DIRECT_FUNC(name)  PPCAT(oglu, name)          = &PPCAT(gl, name)
#define DIRECT_FUNC_(name) PPCAT(PPCAT(oglu, name),_) = &PPCAT(gl, name)

    // buffer related
    QUERY_FUNC (GenBuffers,             "", "ARB");
    QUERY_FUNC (DeleteBuffers,          "", "ARB");
    QUERY_FUNC (BindBuffer,             "", "ARB");
    QUERY_FUNC (BindBufferBase,         "", "EXT", "NV");
    QUERY_FUNC (BindBufferRange,        "", "EXT", "NV");
    QUERY_FUNC_(NamedBufferStorage,     "", "EXT");
    QUERY_FUNC_(BufferStorage,          "", "EXT");
    QUERY_FUNC_(NamedBufferData,        "", "EXT");
    QUERY_FUNC_(BufferData,             "", "ARB");
    QUERY_FUNC_(NamedBufferSubData,     "", "EXT");
    QUERY_FUNC_(BufferSubData,          "", "ARB");
    QUERY_FUNC_(MapNamedBuffer,         "", "EXT");
    QUERY_FUNC_(MapBuffer,              "", "ARB");
    QUERY_FUNC_(UnmapNamedBuffer,       "", "EXT");
    QUERY_FUNC_(UnmapBuffer,            "", "ARB");

    // vao related
    QUERY_FUNC (GenVertexArrays,            "", "ARB");
    QUERY_FUNC (DeleteVertexArrays,         "", "ARB");
    QUERY_FUNC_(BindVertexArray,            "", "ARB");
    QUERY_FUNC_(EnableVertexAttribArray,    "", "ARB");
    QUERY_FUNC_(EnableVertexArrayAttrib,    "", "EXT");
    QUERY_FUNC_(VertexAttribIPointer,       "", "ARB");
    QUERY_FUNC_(VertexAttribLPointer,       "", "EXT");
    QUERY_FUNC_(VertexAttribPointer,        "", "ARB");
    QUERY_FUNC (VertexAttribDivisor,        "", "ARB", "EXT", "NV");

    // draw related
    DIRECT_FUNC(DrawArrays);
    DIRECT_FUNC(DrawElements);
    QUERY_FUNC (MultiDrawArrays,                                "", "EXT");
    QUERY_FUNC (MultiDrawElements,                              "", "EXT");
    QUERY_FUNC_(MultiDrawArraysIndirect,                        "", "EXT", "AMD");
    QUERY_FUNC_(DrawArraysInstancedBaseInstance,                "", "EXT");
    QUERY_FUNC_(DrawArraysInstanced,                            "", "ARB", "EXT", "NV");
    QUERY_FUNC_(MultiDrawElementsIndirect,                      "", "EXT", "AMD");
    QUERY_FUNC_(DrawElementsInstancedBaseVertexBaseInstance,    "", "EXT");
    QUERY_FUNC_(DrawElementsInstancedBaseInstance,              "", "EXT");
    QUERY_FUNC_(DrawElementsInstanced,                          "", "ARB", "EXT", "NV");

    //texture related
    DIRECT_FUNC(GenTextures);
    QUERY_FUNC_(CreateTextures,                 "");
    DIRECT_FUNC(DeleteTextures);
    QUERY_FUNC (ActiveTexture,                  "", "ARB");
    DIRECT_FUNC(BindTexture);
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

    //rbo related
    QUERY_FUNC_(GenRenderbuffers,                               "", "EXT");
    QUERY_FUNC_(CreateRenderbuffers,                            "");
    QUERY_FUNC (DeleteRenderbuffers,                            "", "EXT");
    QUERY_FUNC_(BindRenderbuffer,                               "", "EXT");
    QUERY_FUNC_(NamedRenderbufferStorage,                       "", "EXT");
    QUERY_FUNC_(RenderbufferStorage,                            "", "EXT");
    QUERY_FUNC_(NamedRenderbufferStorageMultisample,            "", "EXT");
    QUERY_FUNC_(RenderbufferStorageMultisample,                 "", "EXT", "NV", "APPLE");
    QUERY_FUNC_(NamedRenderbufferStorageMultisampleCoverageEXT, "");
    QUERY_FUNC_(RenderbufferStorageMultisampleCoverageNV,       "");
    QUERY_FUNC_(GetRenderbufferParameteriv,                     "", "EXT");

    
    //fbo related
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &MaxColorAttachment);
    QUERY_FUNC_(GenFramebuffers,                            "", "EXT");
    QUERY_FUNC_(CreateFramebuffers,                         "", "EXT");
    QUERY_FUNC (DeleteFramebuffers,                         "", "EXT");
    QUERY_FUNC_(BindFramebuffer,                            "", "EXT");
    QUERY_FUNC_(BlitNamedFramebuffer,                       "");
    QUERY_FUNC_(BlitFramebuffer,                            "", "EXT");
    QUERY_FUNC_(NamedFramebufferRenderbuffer,               "", "EXT");
    QUERY_FUNC_(FramebufferRenderbuffer,                    "", "EXT");
    QUERY_FUNC_(NamedFramebufferTexture1DEXT,               "");
    QUERY_FUNC_(FramebufferTexture1D,                       "", "EXT");
    QUERY_FUNC_(NamedFramebufferTexture2DEXT,               "");
    QUERY_FUNC_(FramebufferTexture2D,                       "", "EXT");
    QUERY_FUNC_(NamedFramebufferTexture3DEXT,               "");
    QUERY_FUNC_(FramebufferTexture3D,                       "", "EXT");
    QUERY_FUNC_(NamedFramebufferTexture,                    "", "EXT");
    QUERY_FUNC_(FramebufferTexture,                         "", "EXT");
    QUERY_FUNC_(NamedFramebufferTextureLayer,               "", "EXT");
    QUERY_FUNC_(FramebufferTextureLayer,                    "", "EXT");
    QUERY_FUNC_(CheckNamedFramebufferStatus,                "", "EXT");
    QUERY_FUNC_(CheckFramebufferStatus,                     "", "EXT");
    QUERY_FUNC_(GetNamedFramebufferAttachmentParameteriv,   "", "EXT");
    QUERY_FUNC_(GetFramebufferAttachmentParameteriv,        "", "EXT");

    //shader related
    QUERY_FUNC (CreateShader,       "");
    QUERY_FUNC (DeleteShader,       "");
    QUERY_FUNC (ShaderSource,       "");
    QUERY_FUNC (CompileShader,      "");
    QUERY_FUNC (GetShaderInfoLog,   "");
    QUERY_FUNC (GetShaderSource,    "");
    QUERY_FUNC (GetShaderiv,        "");

    //program related
    QUERY_FUNC (CreateProgram,                      "");
    QUERY_FUNC (DeleteProgram,                      "");
    QUERY_FUNC (AttachShader,                       "");
    QUERY_FUNC (DetachShader,                       "");
    QUERY_FUNC (LinkProgram,                        "");
    QUERY_FUNC (UseProgram,                         "");
    QUERY_FUNC (DispatchCompute,                    "");
    QUERY_FUNC (DispatchComputeIndirect,            "");

    QUERY_FUNC_(Uniform1f,                          "");
    QUERY_FUNC_(Uniform1fv,                         "");
    QUERY_FUNC_(Uniform1i,                          "");
    QUERY_FUNC_(Uniform1iv,                         "");
    QUERY_FUNC_(Uniform2f,                          "");
    QUERY_FUNC_(Uniform2fv,                         "");
    QUERY_FUNC_(Uniform2i,                          "");
    QUERY_FUNC_(Uniform2iv,                         "");
    QUERY_FUNC_(Uniform3f,                          "");
    QUERY_FUNC_(Uniform3fv,                         "");
    QUERY_FUNC_(Uniform3i,                          "");
    QUERY_FUNC_(Uniform3iv,                         "");
    QUERY_FUNC_(Uniform4f,                          "");
    QUERY_FUNC_(Uniform4fv,                         "");
    QUERY_FUNC_(Uniform4i,                          "");
    QUERY_FUNC_(Uniform4iv,                         "");
    QUERY_FUNC_(UniformMatrix2fv,                   "");
    QUERY_FUNC_(UniformMatrix3fv,                   "");
    QUERY_FUNC_(UniformMatrix4fv,                   "");

    QUERY_FUNC (ProgramUniform1d,                   "");
    QUERY_FUNC (ProgramUniform1dv,                  "");
    QUERY_FUNC (ProgramUniform1f,                   "", "EXT");
    QUERY_FUNC (ProgramUniform1fv,                  "", "EXT");
    QUERY_FUNC (ProgramUniform1i,                   "", "EXT");
    QUERY_FUNC (ProgramUniform1iv,                  "", "EXT");
    QUERY_FUNC (ProgramUniform1ui,                  "", "EXT");
    QUERY_FUNC (ProgramUniform1uiv,                 "", "EXT");
    QUERY_FUNC (ProgramUniform2d,                   "");
    QUERY_FUNC (ProgramUniform2dv,                  "");
    QUERY_FUNC (ProgramUniform2f,                   "", "EXT");
    QUERY_FUNC (ProgramUniform2fv,                  "", "EXT");
    QUERY_FUNC (ProgramUniform2i,                   "", "EXT");
    QUERY_FUNC (ProgramUniform2iv,                  "", "EXT");
    QUERY_FUNC (ProgramUniform2ui,                  "", "EXT");
    QUERY_FUNC (ProgramUniform2uiv,                 "", "EXT");
    QUERY_FUNC (ProgramUniform3d,                   "");
    QUERY_FUNC (ProgramUniform3dv,                  "");
    QUERY_FUNC (ProgramUniform3f,                   "", "EXT");
    QUERY_FUNC (ProgramUniform3fv,                  "", "EXT");
    QUERY_FUNC (ProgramUniform3i,                   "", "EXT");
    QUERY_FUNC (ProgramUniform3iv,                  "", "EXT");
    QUERY_FUNC (ProgramUniform3ui,                  "", "EXT");
    QUERY_FUNC (ProgramUniform3uiv,                 "", "EXT");
    QUERY_FUNC (ProgramUniform4d,                   "");
    QUERY_FUNC (ProgramUniform4dv,                  "");
    QUERY_FUNC (ProgramUniform4f,                   "", "EXT");
    QUERY_FUNC (ProgramUniform4fv,                  "", "EXT");
    QUERY_FUNC (ProgramUniform4i,                   "", "EXT");
    QUERY_FUNC (ProgramUniform4iv,                  "", "EXT");
    QUERY_FUNC (ProgramUniform4ui,                  "", "EXT");
    QUERY_FUNC (ProgramUniform4uiv,                 "", "EXT");
    QUERY_FUNC (ProgramUniformMatrix2dv,            "");
    QUERY_FUNC (ProgramUniformMatrix2fv,            "", "EXT");
    QUERY_FUNC (ProgramUniformMatrix2x3dv,          "");
    QUERY_FUNC (ProgramUniformMatrix2x3fv,          "", "EXT");
    QUERY_FUNC (ProgramUniformMatrix2x4dv,          "");
    QUERY_FUNC (ProgramUniformMatrix2x4fv,          "", "EXT");
    QUERY_FUNC (ProgramUniformMatrix3dv,            "");
    QUERY_FUNC (ProgramUniformMatrix3fv,            "", "EXT");
    QUERY_FUNC (ProgramUniformMatrix3x2dv,          "");
    QUERY_FUNC (ProgramUniformMatrix3x2fv,          "", "EXT");
    QUERY_FUNC (ProgramUniformMatrix3x4dv,          "");
    QUERY_FUNC (ProgramUniformMatrix3x4fv,          "", "EXT");
    QUERY_FUNC (ProgramUniformMatrix4dv,            "");
    QUERY_FUNC (ProgramUniformMatrix4fv,            "", "EXT");
    QUERY_FUNC (ProgramUniformMatrix4x2dv,          "");
    QUERY_FUNC (ProgramUniformMatrix4x2fv,          "", "EXT");
    QUERY_FUNC (ProgramUniformMatrix4x3dv,          "");
    QUERY_FUNC (ProgramUniformMatrix4x3fv,          "", "EXT");

    QUERY_FUNC (GetUniformfv,                       "");
    QUERY_FUNC (GetUniformiv,                       "");
    QUERY_FUNC (GetUniformuiv,                      "");
    QUERY_FUNC (GetProgramInfoLog,                  "");
    QUERY_FUNC (GetProgramiv,                       "");
    QUERY_FUNC (GetProgramInterfaceiv,              "");
    QUERY_FUNC (GetProgramResourceIndex,            "");
    QUERY_FUNC (GetProgramResourceLocation,         "");
    QUERY_FUNC (GetProgramResourceLocationIndex,    "");
    QUERY_FUNC (GetProgramResourceName,             "");
    QUERY_FUNC (GetProgramResourceiv,               "");
    QUERY_FUNC (GetActiveSubroutineName,            "");
    QUERY_FUNC (GetActiveSubroutineUniformName,     "");
    QUERY_FUNC (GetActiveSubroutineUniformiv,       "");
    QUERY_FUNC (GetProgramStageiv,                  "");
    QUERY_FUNC (GetSubroutineIndex,                 "");
    QUERY_FUNC (GetSubroutineUniformLocation,       "");
    QUERY_FUNC (GetUniformSubroutineuiv,            "");
    QUERY_FUNC (UniformSubroutinesuiv,              "");
    QUERY_FUNC (GetActiveUniformBlockName,          "");
    QUERY_FUNC (GetActiveUniformBlockiv,            "");
    QUERY_FUNC (GetActiveUniformName,               "");
    QUERY_FUNC (GetActiveUniformsiv,                "");
    QUERY_FUNC (GetIntegeri_v,                      "");
    QUERY_FUNC (GetUniformBlockIndex,               "");
    QUERY_FUNC (GetUniformIndices,                  "");
    QUERY_FUNC (UniformBlockBinding,                "");

    //query related
    QUERY_FUNC (GenQueries,             "");
    QUERY_FUNC (DeleteQueries,          "");
    QUERY_FUNC (BeginQuery,             "");
    QUERY_FUNC (QueryCounter,           "");
    QUERY_FUNC (GetQueryObjectiv,       "");
    QUERY_FUNC (GetQueryObjectuiv,      "");
    QUERY_FUNC (GetQueryObjecti64v,     "", "EXT");
    QUERY_FUNC (GetQueryObjectui64v,    "", "EXT");
    QUERY_FUNC (GetQueryiv,             "");
    QUERY_FUNC (FenceSync,              "", "APPLE");
    QUERY_FUNC (DeleteSync,             "", "APPLE");
    QUERY_FUNC (ClientWaitSync,         "", "APPLE");
    QUERY_FUNC (WaitSync,               "", "APPLE");
    QUERY_FUNC (GetInteger64v,          "", "APPLE");
    QUERY_FUNC (GetSynciv,              "", "APPLE");

    //debug
    QUERY_FUNC (DebugMessageCallback,   "", "ARB", "AMD");
    QUERY_FUNC_(ObjectLabel,            "");
    QUERY_FUNC_(ObjectPtrLabel,         "");

    //others
    DIRECT_FUNC(GetError);
    DIRECT_FUNC(GetFloatv);
    DIRECT_FUNC(GetIntegerv);
    DIRECT_FUNC(GetString);
    QUERY_FUNC (GetStringi,             "");
    DIRECT_FUNC(IsEnabled);
    DIRECT_FUNC(Enable);
    DIRECT_FUNC(Disable);
    DIRECT_FUNC(Finish);
    DIRECT_FUNC(Flush);
    DIRECT_FUNC(DepthFunc);
    DIRECT_FUNC(CullFace);
    DIRECT_FUNC(FrontFace);
    DIRECT_FUNC(Clear);
    DIRECT_FUNC(Viewport);
    QUERY_FUNC (ViewportArrayv,         "", "NV");
    QUERY_FUNC (ViewportIndexedf,       "", "NV");
    QUERY_FUNC (ViewportIndexedfv,      "", "NV");
    DIRECT_FUNC_(ClearDepth);
    QUERY_FUNC_(ClearDepthf,            "");
    QUERY_FUNC (ClipControl,            "", "EXT");
    QUERY_FUNC (MemoryBarrier,          "", "EXT");


    Extensions = GetExtensions();
    ogluGetIntegerv(GL_MAX_LABEL_LENGTH, &MaxLabelLen);
    SupportDebug            = ogluDebugMessageCallback != nullptr;
    SupportSRGB             = PlatFunc->SupportSRGB && (Extensions.Has("GL_ARB_framebuffer_sRGB") || Extensions.Has("GL_EXT_framebuffer_sRGB"));
    SupportClipControl      = ogluClipControl != nullptr;
    SupportImageLoadStore   = Extensions.Has("GL_ARB_shader_image_load_store") || Extensions.Has("GL_EXT_shader_image_load_store");
    SupportComputeShader    = Extensions.Has("GL_ARB_compute_shader");
    SupportTessShader       = Extensions.Has("GL_ARB_tessellation_shader");

#undef WITH_SUFFIX
#undef WITH_SUFFIXS
#undef QUERY_FUNC
#undef QUERY_FUNC_
#undef DIRECT_FUNC
}

constexpr auto kkk = sizeof(DSAFuncs);

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


struct VAOBinder : public common::NonCopyable
{
    common::SpinLocker::ScopeType Lock;
    const DSAFuncs& DSA;
    bool Changed;
    VAOBinder(const DSAFuncs* dsa, GLuint newVAO) noexcept :
        Lock(dsa->DataLock.LockScope()), DSA(*dsa), Changed(false)
    {
        if (newVAO != DSA.VAO)
            Changed = true, DSA.ogluBindVertexArray_(newVAO);
    }
    ~VAOBinder()
    {
        if (Changed)
            DSA.ogluBindVertexArray_(DSA.VAO);
    }
};
void DSAFuncs::RefreshVAOState() const
{
    const auto lock = DataLock.LockScope();
    ogluGetIntegerv(GL_VERTEX_ARRAY_BINDING, reinterpret_cast<GLint*>(&VAO));
}
void DSAFuncs::ogluBindVertexArray(GLuint vaobj) const
{
    const auto lock = DataLock.LockScope();
    ogluBindVertexArray_(vaobj);
    VAO = vaobj;
}
void DSAFuncs::ogluEnableVertexArrayAttrib(GLuint vaobj, GLuint index) const
{
    CALL_EXISTS(ogluEnableVertexArrayAttrib_, vaobj, index)
    {
        ogluBindVertexArray_(vaobj); // ensure be in binding
        ogluEnableVertexAttribArray_(index);
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
        ogluGenTextures(n, textures);
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


void DSAFuncs::ogluCreateRenderbuffers(GLsizei n, GLuint* renderbuffers) const
{
    CALL_EXISTS(ogluCreateRenderbuffers_, n, renderbuffers)
    {
        ogluGenRenderbuffers_(n, renderbuffers);
        for (GLsizei i = 0; i < n; ++i)
            ogluBindRenderbuffer_(GL_RENDERBUFFER, renderbuffers[i]);
    }
}
void DSAFuncs::ogluNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) const
{
    CALL_EXISTS(ogluNamedRenderbufferStorage_, renderbuffer, internalformat, width, height)
    {
        ogluBindRenderbuffer_(GL_RENDERBUFFER, renderbuffer);
        ogluRenderbufferStorage_(GL_RENDERBUFFER, internalformat, width, height);
    }
}
void DSAFuncs::ogluNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) const
{
    CALL_EXISTS(ogluNamedRenderbufferStorageMultisample_, renderbuffer, samples, internalformat, width, height)
    {
        ogluBindRenderbuffer_(GL_RENDERBUFFER, renderbuffer);
        ogluRenderbufferStorageMultisample_(GL_RENDERBUFFER, samples, internalformat, width, height);
    }
}
void DSAFuncs::ogluNamedRenderbufferStorageMultisampleCoverage(GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) const
{
    CALL_EXISTS(ogluNamedRenderbufferStorageMultisampleCoverageEXT_, renderbuffer, coverageSamples, colorSamples, internalformat, width, height)
    if (ogluRenderbufferStorageMultisampleCoverageNV_)
    {
        ogluBindRenderbuffer_(GL_RENDERBUFFER, renderbuffer);
        ogluRenderbufferStorageMultisampleCoverageNV_(GL_RENDERBUFFER, coverageSamples, colorSamples, internalformat, width, height);
    }
    else
    {
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"no avaliable implementation for RenderbufferStorageMultisampleCoverage");
    }
}


struct FBOBinder : public common::NonCopyable
{
    common::SpinLocker::ScopeType Lock;
    const DSAFuncs& DSA;
    bool ChangeRead, ChangeDraw;
    FBOBinder(const DSAFuncs* dsa) noexcept :
        Lock(dsa->DataLock.LockScope()), DSA(*dsa), ChangeRead(true), ChangeDraw(true)
    { }
    FBOBinder(const DSAFuncs* dsa, const GLuint newReadFBO) noexcept :
        Lock(dsa->DataLock.LockScope()), DSA(*dsa), ChangeRead(false), ChangeDraw(false)
    {
        if (DSA.ReadFBO != newReadFBO)
            ChangeRead = true, DSA.ogluBindFramebuffer_(GL_READ_FRAMEBUFFER, newReadFBO);
    }
    FBOBinder(const DSAFuncs* dsa, const std::pair<GLuint, GLuint> newFBO) noexcept :
        Lock(dsa->DataLock.LockScope()), DSA(*dsa), ChangeRead(false), ChangeDraw(false)
    {
        if (DSA.ReadFBO != newFBO.first)
            ChangeRead = true, DSA.ogluBindFramebuffer_(GL_READ_FRAMEBUFFER, newFBO.first);
        if (DSA.DrawFBO != newFBO.second)
            ChangeDraw = true, DSA.ogluBindFramebuffer_(GL_DRAW_FRAMEBUFFER, newFBO.second);
    }
    FBOBinder(const DSAFuncs* dsa, const GLenum target, const GLuint newFBO) noexcept :
        Lock(dsa->DataLock.LockScope()), DSA(*dsa), ChangeRead(false), ChangeDraw(false)
    {
        if (target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
        {
            if (DSA.ReadFBO != newFBO)
                ChangeRead = true, DSA.ogluBindFramebuffer_(GL_READ_FRAMEBUFFER, newFBO);
        }
        if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
        {
            if (DSA.DrawFBO != newFBO)
                ChangeDraw = true, DSA.ogluBindFramebuffer_(GL_DRAW_FRAMEBUFFER, newFBO);
        }
    }
    ~FBOBinder()
    {
        if (ChangeRead)
            DSA.ogluBindFramebuffer_(GL_READ_FRAMEBUFFER, DSA.ReadFBO);
        if (ChangeDraw)
            DSA.ogluBindFramebuffer_(GL_DRAW_FRAMEBUFFER, DSA.DrawFBO);
    }
};
void DSAFuncs::RefreshFBOState() const
{
    const auto lock = DataLock.LockScope();
    ogluGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&ReadFBO));
    ogluGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&DrawFBO));
}
void DSAFuncs::ogluCreateFramebuffers(GLsizei n, GLuint* framebuffers) const
{
    CALL_EXISTS(ogluCreateFramebuffers_, n, framebuffers)
    {
        ogluGenFramebuffers_(n, framebuffers);
        const auto backup = FBOBinder(this);
        for (GLsizei i = 0; i < n; ++i)
            ogluBindFramebuffer_(GL_READ_FRAMEBUFFER_BINDING, framebuffers[i]);
    }
}
void DSAFuncs::ogluBindFramebuffer(GLenum target, GLuint framebuffer) const
{
    const auto lock = DataLock.LockScope();
    ogluBindFramebuffer_(target, framebuffer);
    if (target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
        ReadFBO = framebuffer;
    if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
        DrawFBO = framebuffer;
}
void DSAFuncs::ogluBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) const
{
    CALL_EXISTS(ogluBlitNamedFramebuffer_, readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
    {
        const auto backup = FBOBinder(this, { readFramebuffer, drawFramebuffer });
        ogluBlitFramebuffer_(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    }
}
void DSAFuncs::ogluNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) const
{
    CALL_EXISTS(ogluNamedFramebufferRenderbuffer_, framebuffer, attachment, renderbuffertarget, renderbuffer)
    {
        const auto backup = FBOBinder(this, framebuffer);
        ogluFramebufferRenderbuffer_(GL_READ_FRAMEBUFFER, attachment, renderbuffertarget, renderbuffer);
    }
}
void DSAFuncs::ogluNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) const
{
    CALL_EXISTS(ogluNamedFramebufferTexture_, framebuffer, attachment, texture, level)
    if (ogluFramebufferTexture_)
    {
        const auto backup = FBOBinder(this, framebuffer);
        ogluFramebufferTexture_(GL_READ_FRAMEBUFFER, attachment, texture, level);
    }
    else
    {
        switch (textarget)
        {
        case GL_TEXTURE_1D:
            CALL_EXISTS(ogluNamedFramebufferTexture1DEXT_, framebuffer, attachment, textarget, texture, level)
            {
                const auto backup = FBOBinder(this, framebuffer);
                ogluFramebufferTexture1D_(GL_READ_FRAMEBUFFER, attachment, textarget, texture, level);
            }
            break;
        case GL_TEXTURE_2D:
            CALL_EXISTS(ogluNamedFramebufferTexture2DEXT_, framebuffer, attachment, textarget, texture, level)
            {
                const auto backup = FBOBinder(this, framebuffer);
                ogluFramebufferTexture2D_(GL_READ_FRAMEBUFFER, attachment, textarget, texture, level);
            }
            break;
        default:
            COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"unsupported textarget with calling FramebufferTexture");
        }
    }
}
void DSAFuncs::ogluNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer) const
{
    CALL_EXISTS(ogluNamedFramebufferTextureLayer_, framebuffer, attachment, texture, level, layer)
    if (ogluFramebufferTextureLayer_)
    {
        const auto backup = FBOBinder(this, framebuffer);
        ogluFramebufferTextureLayer_(GL_READ_FRAMEBUFFER, attachment, texture, level, layer);
    }
    else
    {
        switch (textarget)
        {
        case GL_TEXTURE_3D:
            CALL_EXISTS(ogluNamedFramebufferTexture3DEXT_, framebuffer, attachment, textarget, texture, level, layer)
            {
                const auto backup = FBOBinder(this, framebuffer);
                ogluFramebufferTexture3D_(GL_READ_FRAMEBUFFER, attachment, textarget, texture, level, layer);
            }
            break;
        default:
            COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"unsupported textarget with calling FramebufferTextureLayer");
        }
    }
}
GLenum DSAFuncs::ogluCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) const
{
    CALL_EXISTS(ogluCheckNamedFramebufferStatus_, framebuffer, target)
    {
        const auto backup = FBOBinder(this, target, framebuffer);
        return ogluCheckFramebufferStatus_(target);
    }
}
void DSAFuncs::ogluGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params) const
{
    CALL_EXISTS(ogluGetNamedFramebufferAttachmentParameteriv_, framebuffer, attachment, pname, params)
    {
        const auto backup = FBOBinder(this, framebuffer);
        ogluGetFramebufferAttachmentParameteriv_(GL_READ_FRAMEBUFFER, attachment, pname, params);
    }
}


void DSAFuncs::ogluSetObjectLabel(GLenum type, GLuint id, std::u16string_view name) const
{
    if (ogluObjectLabel_)
    {
        const auto str = common::strchset::to_u8string(name, common::str::Charset::UTF16LE);
        ogluObjectLabel_(type, id,
            static_cast<GLsizei>(std::min<size_t>(str.size(), MaxLabelLen)),
            reinterpret_cast<const GLchar*>(str.c_str()));
    }
}
void DSAFuncs::ogluSetObjectLabel(GLsync sync, std::u16string_view name) const
{
    if (ogluObjectPtrLabel_)
    {
        const auto str = common::strchset::to_u8string(name, common::str::Charset::UTF16LE);
        ogluObjectPtrLabel_(sync,
            static_cast<GLsizei>(std::min<size_t>(str.size(), MaxLabelLen)),
            reinterpret_cast<const GLchar*>(str.c_str()));
    }
}


void DSAFuncs::ogluClearDepth(GLclampd d) const
{
    CALL_EXISTS(ogluClearDepth_, d)
    CALL_EXISTS(ogluClearDepthf_, static_cast<GLclampf>(d))
    {
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"unsupported textarget with calling ClearDepth");
    }
}


common::container::FrozenDenseSet<std::string_view> DSAFuncs::GetExtensions() const
{
    if (ogluGetStringi)
    {
        GLint count;
        ogluGetIntegerv(GL_NUM_EXTENSIONS, &count);
        std::vector<std::string_view> exts;
        exts.reserve(count);
        for (GLint i = 0; i < count; i++)
        {
            const GLubyte* ext = ogluGetStringi(GL_EXTENSIONS, i);
            exts.emplace_back(reinterpret_cast<const char*>(ext));
        }
        return exts;
    }
    else
    {
        const GLubyte* exts = ogluGetString(GL_EXTENSIONS);
        return common::str::Split(reinterpret_cast<const char*>(exts), ' ', false);
    }
}
std::optional<std::string_view> DSAFuncs::GetError() const
{
    const auto err = ogluGetError();
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




}