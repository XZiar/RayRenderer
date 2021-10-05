#include "oglPch.h"
#include "GLFuncWrapper.h"
#include "oglUtil.h"
#include "oglException.h"
#include "oglContext.h"
#include "oglBuffer.h"

#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>


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
    using DCType = HDC;
#elif COMMON_OS_UNIX
#   include <dlfcn.h>
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
    using DCType = Display*;
#else
#   error "unknown os"
#endif


namespace oglu
{
[[maybe_unused]] constexpr auto PlatFuncsSize = sizeof(PlatFuncs);
[[maybe_unused]] constexpr auto CtxFuncsSize = sizeof(CtxFuncs);

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
            const auto [it, ret] = Map.emplace(key, std::make_unique<T>(key));
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
thread_local const CtxFuncs* CtxFunc = nullptr;

static auto& GetPlatFuncsMap()
{
    static ResourceKeeper<PlatFuncs> PlatFuncMap;
    return PlatFuncMap;
}
static auto& GetCtxFuncsMap()
{
    static ResourceKeeper<CtxFuncs> CtxFuncMap;
    return CtxFuncMap;
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
static void PrepareCtxFuncs(void* hRC)
{
    if (hRC == nullptr)
        CtxFunc = nullptr;
    else
    {
        auto& rmap = GetCtxFuncsMap();
        CtxFunc = rmap.FindOrCreate(hRC);
    }
}

#if COMMON_OS_WIN
constexpr PIXELFORMATDESCRIPTOR PixFmtDesc =    // pfd Tells Windows How We Want Things To Be
{
    sizeof(PIXELFORMATDESCRIPTOR),              // Size Of This Pixel Format Descriptor
    1,                                          // Version Number
    PFD_DRAW_TO_WINDOW/*Support Window*/ | PFD_SUPPORT_OPENGL/*Support OpenGL*/ | PFD_DOUBLEBUFFER/*Support Double Buffering*/ | PFD_GENERIC_ACCELERATED,
    PFD_TYPE_RGBA,                              // Request An RGBA Format
    32,                                         // Select Our Color Depth
    0, 0, 0, 0, 0, 0,                           // Color Bits Ignored
    0, 0,                                       // No Alpha Buffer, Shift Bit Ignored
    0, 0, 0, 0, 0,                              // No Accumulation Buffer, Accumulation Bits Ignored
    24,                                         // 24Bit Z-Buffer (Depth Buffer) 
    8,                                          // 8Bit Stencil Buffer
    0,                                          // No Auxiliary Buffer
    PFD_MAIN_PLANE,                             // Main Drawing Layer
    0,                                          // Reserved
    0, 0, 0                                     // Layer Masks Ignored
};
void oglUtil::SetPixFormat(void* hdc_)
{
    const auto hdc = reinterpret_cast<HDC>(hdc_);
    const int pixFormat = ChoosePixelFormat(hdc, &PixFmtDesc);
    SetPixelFormat(hdc, pixFormat, &PixFmtDesc);
}
#elif COMMON_OS_LINUX
constexpr int VisualAttrs[] =
{
    GLX_X_RENDERABLE,   1,
    GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
    GLX_RENDER_TYPE,    GLX_RGBA_BIT,
    GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
    GLX_RED_SIZE,       8,
    GLX_GREEN_SIZE,     8,
    GLX_BLUE_SIZE,      8,
    GLX_ALPHA_SIZE,     8,
    GLX_DEPTH_SIZE,     24,
    GLX_STENCIL_SIZE,   8,
    GLX_DOUBLEBUFFER,   1,
    0
};
constexpr int VisualAttrs2[] =
{
    GLX_X_RENDERABLE,   1,
    GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
    GLX_RENDER_TYPE,    GLX_RGBA_BIT,
    GLX_DRAWABLE_TYPE,  GLX_PIXMAP_BIT,
    GLX_RED_SIZE,       8,
    GLX_GREEN_SIZE,     8,
    GLX_BLUE_SIZE,      8,
    GLX_ALPHA_SIZE,     8,
    GLX_DEPTH_SIZE,     24,
    GLX_STENCIL_SIZE,   8,
    GLX_DOUBLEBUFFER,   1,
    0
};
std::optional<GLContextInfo> oglUtil::GetBasicContextInfo(void* display, bool useOffscreen)
{
    GLContextInfo info;
    info.DeviceContext = display;
    const auto disp = reinterpret_cast<Display*>(display);
    int fbCount = 0;
    const auto configs = glXChooseFBConfig(disp, DefaultScreen(disp), useOffscreen ? VisualAttrs2 : VisualAttrs, &fbCount);
    if (!configs || fbCount == 0)
        return {};
    if (0 != glXGetFBConfigAttrib(disp, configs[0], GLX_VISUAL_ID, &info.VisualId))
        return {};
    info.FBConfigs = configs;
    info.IsWindowDrawable = !useOffscreen;
    return info;
}
void oglUtil::InitDrawable(GLContextInfo& info, uint32_t host)
{
    const auto display = reinterpret_cast<Display*>(info.DeviceContext);
    const auto config  = reinterpret_cast<GLXFBConfig*>(info.FBConfigs)[0];
    if (info.IsWindowDrawable)
    {
        const auto glxwindow = glXCreateWindow(display, config, host, nullptr);
        info.Drawable = glxwindow;
    }
    else
    {
        XVisualInfo* vi = glXGetVisualFromFBConfig(display, config);
        const auto glxpixmap = glXCreateGLXPixmap(display, vi, host);
        info.Drawable = glxpixmap;
    }
}
void oglUtil::DestroyDrawable(GLContextInfo& info)
{
    const auto display = reinterpret_cast<Display*>(info.DeviceContext);
    if (info.IsWindowDrawable)
    {
        glXDestroyWindow(display, info.Drawable);
    }
    else
    {
        glXDestroyGLXPixmap(display, info.Drawable);
    }
    XFree(info.FBConfigs);
}
#endif

oglContext oglContext_::InitContext(const GLContextInfo& info)
{
    constexpr uint32_t VERSIONS[] = { 46,45,44,43,42,41,40,33,32,31,30 };
    const auto oldCtx = Refresh();
    PreparePlatFuncs(info.DeviceContext); // ensure PlatFuncs exists
    void* ctx = nullptr;
    if (LatestVersion == 0) // perform update
    {
        for (const auto ver : VERSIONS)
        {
            if (ctx = PlatFuncs::CreateNewContext(info, ver); ctx && PlatFuncs::MakeGLContextCurrent(info, ctx))
            {
                const auto verStr = common::str::to_u16string(
                    reinterpret_cast<const char*>(glGetString(GL_VERSION)), common::str::Encoding::UTF8);
                const auto vendor = common::str::to_u16string(
                    reinterpret_cast<const char*>(glGetString(GL_VENDOR)), common::str::Encoding::UTF8);
                int32_t major = 0, minor = 0;
                glGetIntegerv(GL_MAJOR_VERSION, &major);
                glGetIntegerv(GL_MINOR_VERSION, &minor);
                const uint32_t realVer = major * 10 + minor;
                oglLog().info(u"Latest GL version [{}] from [{}]\n", verStr, vendor);
                common::UpdateAtomicMaximum(LatestVersion, realVer);
                break;
            }
        }
    }
    else
    {
        ctx = PlatFuncs::CreateNewContext(info, LatestVersion.load());
    }
    if (!ctx)
    {
#if COMMON_OS_WIN
        oglLog().error(u"failed to init context on HDC[{}], error: {}\n", info.DeviceContext, PlatFuncs::GetSystemError());
#elif COMMON_OS_LINUX
        oglLog().error(u"failed to init context on Display[{}] Drawable[{}], error: {}\n",
            info.DeviceContext, info.Drawable, PlatFuncs::GetSystemError());
#endif
        return {};
    }
#if COMMON_OS_WIN
    oglContext newCtx(new oglContext_(std::make_shared<detail::SharedContextCore>(), info.DeviceContext, ctx));
#elif COMMON_OS_LINUX
    oglContext newCtx(new oglContext_(std::make_shared<detail::SharedContextCore>(), info.DeviceContext, ctx, info.Drawable));
#endif
    newCtx->Init(true);
    PushToMap(newCtx);
    newCtx->UnloadContext();
    if (oldCtx)
        oldCtx->UseContext();
    return newCtx;
}


#if COMMON_OS_UNIX
static int TmpXErrorHandler(Display* disp, XErrorEvent* evt)
{
    thread_local std::string txtBuf;
    txtBuf.resize(1024, '\0');
    XGetErrorText(disp, evt->error_code, txtBuf.data(), 1024); // return value undocumented, cannot rely on that
    txtBuf.resize(std::char_traits<char>::length(txtBuf.data()));
    oglLog().warning(u"X11 report an error with code[{}][{}]:\t{}\n", evt->error_code, evt->minor_code,
        common::str::to_u16string(txtBuf, common::str::Encoding::UTF8));
    return 0;
}
#endif


static void ShowQuerySuc (const std::string_view tarName, const std::string_view ext, void* ptr)
{
    oglLog().verbose(FMT_STRING(u"Func [{}] uses [gl...{{{}}}] ({:p})\n"), tarName, ext, (void*)ptr);
}
static void ShowQueryFail(const std::string_view tarName)
{
    oglLog().warning(FMT_STRING(u"Func [{}] not found\n"), tarName);
}
static void ShowQueryFall(const std::string_view tarName)
{
    oglLog().warning(FMT_STRING(u"Func [{}] fallback to default\n"), tarName);
}

//template<typename T, std::size_T N, std::size_t ...Ns>
//std::array<T, N> make_array_impl(
//    std::initializer_list<T> t,
//    std::index_sequence<Ns...>)
//{
//    return std::array<T, N>{ *(t.begin() + Ns) ... };
//}
//
//template<typename T, std::size_t N>
//std::array<T, N> make_array(std::initializer_list<T> t) {
//    if (N > t.size())
//        throw std::out_of_range("that's crazy!");
//    return make_array_impl<T, N>(t, std::make_index_sequence<N>());
//}
//
//template<std::size_t... Idxes>
//static constexpr auto CreateSVArray_(const std::initializer_list<std::string_view> txts, std::index_sequence<Idxes...>)
//{
//    return std::array<std::string_view, sizeof...(Idxes)>{ *(t.begin() + Idxes)... };
//}
//static constexpr auto CreateSVArray(const std::initializer_list<std::string_view> txts)
//{
//    return CreateSVArray_(txts, std::make_index_sequence<txts.size()>());
//}

template<typename T, size_t N>
static void QueryFunc(T& target, const std::string_view tarName, const std::pair<bool, bool> shouldPrint, 
    const std::array<std::string_view, N> names, const std::array<std::string_view, N> suffixes)
{
    const auto [printSuc, printFail] = shouldPrint;
    size_t idx = 0;
    for (const auto& name : names)
    {
#if COMMON_OS_WIN
        const auto ptr = wglGetProcAddress(name.data());
#elif COMMON_OS_DARWIN
        const auto ptr = NSGLGetProcAddress(name.data());
#else
        const auto ptr = glXGetProcAddress(reinterpret_cast<const GLubyte*>(name.data()));
#endif
        if (ptr)
        {
            target = reinterpret_cast<T>(ptr);
            if (printSuc)
                ShowQuerySuc(tarName, suffixes[idx], (void*)ptr);
            return;
        }
        idx++;
    }
    target = nullptr;
    if (printFail)
        ShowQueryFail(tarName);
}

template<typename T>
static void QueryPlainFunc(T& target, const std::string_view tarName,
    const std::pair<bool, bool> shouldPrint, const std::string_view name, void* fallback)
{
    const auto [printSuc, printFail] = shouldPrint;
#if COMMON_OS_WIN
    const auto ptr = wglGetProcAddress(name.data());
#elif COMMON_OS_DARWIN
    const auto ptr = NSGLGetProcAddress(name.data());
#else
    const auto ptr = glXGetProcAddress(reinterpret_cast<const GLubyte*>(name.data()));
#endif
    if (ptr)
    {
        target = reinterpret_cast<T>(ptr);
        if (printSuc)
            ShowQuerySuc(tarName, "", (void*)ptr);
    }
    else
    {
        target = reinterpret_cast<T>(fallback);
        if (printFail)
            ShowQueryFall(tarName);
    }
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
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"no avaliable implementation for wglGetExtensionsString");
    return common::str::Split(exts, ' ', false);
}
#else
common::container::FrozenDenseSet<std::string_view> PlatFuncs::GetExtensions(void* dpy, int screen) const
{
    const char* exts = glXQueryExtensionsString((Display*)dpy, screen);
    return common::str::Split(exts, ' ', false);
}
#endif

PlatFuncs::PlatFuncs(void* target)
{
    const auto shouldPrint = GetFuncShouldPrint();

#define WITH_SUFFIX(r, name, i, sfx)    BOOST_PP_COMMA_IF(i) std::string_view(name sfx)
#define WITH_SUFFIXS(name, ...) std::array{ BOOST_PP_SEQ_FOR_EACH_I(WITH_SUFFIX, name, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) }
#define QUERY_FUNC(name, ...)  QueryFunc(name,          STRINGIZE(name), shouldPrint, WITH_SUFFIXS(#name, __VA_ARGS__), {__VA_ARGS__})
#define QUERY_FUNC_(name, ...) QueryFunc(PPCAT(name,_), STRINGIZE(name), shouldPrint, WITH_SUFFIXS(#name, __VA_ARGS__), {__VA_ARGS__})
    
    //constexpr std::array<std::string_view, 2> kk{ "xx", "yy" };
    //constexpr auto kkk = CreateSVArray("xx", "yy");

#if COMMON_OS_WIN
    const auto hdc = reinterpret_cast<HDC>(target);
    HGLRC tmpHrc = nullptr;
    if (!wglGetCurrentContext())
    { // create temp hrc since wgl requires a context to use wglGetProcAddress
        tmpHrc = wglCreateContext(hdc);
        wglMakeCurrent(hdc, tmpHrc);
    }
    QUERY_FUNC_(wglGetExtensionsStringARB,  "");
    QUERY_FUNC_(wglGetExtensionsStringEXT,  "");
    Extensions = GetExtensions(hdc);
    QUERY_FUNC_(wglCreateContextAttribsARB, "");
    if (tmpHrc)
    {
        wglMakeCurrent(hdc, nullptr);
        wglDeleteContext(tmpHrc);
    }
#else
    const auto display = reinterpret_cast<Display*>(target);
    QUERY_FUNC_(glXGetCurrentDisplay,       "");
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


#if COMMON_OS_UNIX
struct X11Initor
{
    X11Initor()
    {
        XInitThreads();
        XSetErrorHandler(&TmpXErrorHandler);
    }
};
#endif

void PlatFuncs::InJectRenderDoc(const common::fs::path& dllPath)
{
#if COMMON_OS_WIN
    if (common::fs::exists(dllPath))
        LoadLibrary(dllPath.c_str());
    else
        LoadLibrary(L"renderdoc.dll");
#else
    if (common::fs::exists(dllPath))
        dlopen(dllPath.c_str(), RTLD_LAZY);
    else
        dlopen("renderdoc.dll", RTLD_LAZY);
#endif
}
void PlatFuncs::InitEnvironment()
{
#if COMMON_OS_WIN

    HWND tmpWND = CreateWindow(
        L"Static", L"Fake Window",          // window class, title
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN,  // style
        0, 0,                               // position x, y
        1, 1,                               // width, height
        NULL, NULL,                         // parent window, menu
        nullptr, NULL);                     // instance, param
    HDC tmpDC = GetDC(tmpWND);
    const int PixelFormat = ChoosePixelFormat(tmpDC, &PixFmtDesc);
    SetPixelFormat(tmpDC, PixelFormat, &PixFmtDesc);
    HGLRC tmpRC = wglCreateContext(tmpDC);
    wglMakeCurrent(tmpDC, tmpRC);

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(tmpRC);
    DeleteDC(tmpDC);
    DestroyWindow(tmpWND);

#else
    
    XInitThreads();
    XSetErrorHandler(&TmpXErrorHandler);
    const char* const disp = getenv("DISPLAY");
    Display* display = XOpenDisplay(disp ? disp : ":0.0");
    /* open display */
    if (!display)
    {
        oglLog().error(u"Failed to open display\n");
        return;
    }

    /* get framebuffer configs, any is usable (might want to add proper attribs) */
    int fbcount = 0;
    ::GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display), VisualAttrs, &fbcount);
    if (!fbc)
    {
        oglLog().error(u"Failed to get FBConfig\n");
        return;
    }
    XVisualInfo* vi = glXGetVisualFromFBConfig(display, fbc[0]);
    const auto tmpRC = glXCreateContext(display, vi, 0, GL_TRUE);
    Window win = DefaultRootWindow(display);
    glXMakeCurrent(display, win, tmpRC);

    glXMakeCurrent(nullptr, win, nullptr);
    glXDestroyContext(display, tmpRC);

#endif
}

void* PlatFuncs::GetCurrentDeviceContext()
{
#if COMMON_OS_WIN
    const auto hDC = wglGetCurrentDC();
#else
    static X11Initor initor;
    const auto hDC = glXGetCurrentDisplay();
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
    PrepareCtxFuncs(hRC);
    return hRC;
}
void  PlatFuncs::DeleteGLContext([[maybe_unused]] void* hDC, void* hRC)
{
#if COMMON_OS_WIN
    wglDeleteContext((HGLRC)hRC);
#else
    glXDestroyContext((Display*)hDC, (GLXContext)hRC);
#endif
    GetCtxFuncsMap().Remove(hRC);
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
        PrepareCtxFuncs(hRC);
    }
    return ret;
}
bool PlatFuncs::MakeGLContextCurrent(const GLContextInfo& info, void* hRC)
{
#if COMMON_OS_WIN
    return MakeGLContextCurrent(info.DeviceContext, hRC);
#else
    return MakeGLContextCurrent(info.DeviceContext, info.Drawable, hRC);
#endif
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
void* PlatFuncs::CreateNewContext(const GLContextInfo& info, const uint32_t version)
{
    if (PlatFunc == nullptr)
        return nullptr;
    const auto target = reinterpret_cast<DCType>(info.DeviceContext);
    const auto ctxAttrb = GenerateContextAttrib(version, true);
#if COMMON_OS_WIN
    return PlatFunc->wglCreateContextAttribsARB_(target, nullptr, ctxAttrb.data());
#elif COMMON_OS_LINUX
    return PlatFunc->glXCreateContextAttribsARB_(target,
        reinterpret_cast<GLXFBConfig*>(info.FBConfigs)[0], nullptr, true, ctxAttrb.data());
#endif
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
        int num_fbc = 0;
        const auto fbc = glXChooseFBConfig(
            (Display*)prevCtx->Hdc, DefaultScreen((Display*)prevCtx->Hdc), VisualAttrs, &num_fbc);
        const auto newHrc = PlatFunc->glXCreateContextAttribsARB_(
            prevCtx->Hdc, fbc[0], isShared ? prevCtx->Hrc : nullptr, true, attribs);
        return newHrc;
#endif
    }
}
void PlatFuncs::SwapBuffer(const oglContext_& ctx)
{
#if COMMON_OS_WIN
    SwapBuffers(reinterpret_cast<HDC>(ctx.Hdc));
#elif COMMON_OS_LINUX
    glXSwapBuffers(reinterpret_cast<Display*>(ctx.Hdc), ctx.DRW);
#endif
}



CtxFuncs::CtxFuncs(void*)
{
    const auto shouldPrint = GetFuncShouldPrint();

#define WITH_SUFFIX(r, name, i, sfx)    BOOST_PP_COMMA_IF(i) std::string_view("gl" name sfx)
#define WITH_SUFFIXS(name, ...) std::array{ BOOST_PP_SEQ_FOR_EACH_I(WITH_SUFFIX, name, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) }
#define QUERY_FUNC(name, ...)   QueryFunc(PPCAT(oglu, name),          STRINGIZE(name), shouldPrint, WITH_SUFFIXS(#name, __VA_ARGS__), {__VA_ARGS__})
#define QUERY_FUNC_(name, ...)  QueryFunc(PPCAT(PPCAT(oglu, name),_), STRINGIZE(name), shouldPrint, WITH_SUFFIXS(#name, __VA_ARGS__), {__VA_ARGS__})
#define PLAIN_FUNC(name)   QueryPlainFunc(PPCAT(oglu, name),          STRINGIZE(name), shouldPrint, STRINGIZE(PPCAT(gl, name)), (void*)&PPCAT(gl, name))
#define PLAIN_FUNC_(name)  QueryPlainFunc(PPCAT(PPCAT(oglu, name),_), STRINGIZE(name), shouldPrint, STRINGIZE(PPCAT(gl, name)), (void*)&PPCAT(gl, name))

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
    PLAIN_FUNC (DrawArrays);
    PLAIN_FUNC (DrawElements);
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
    PLAIN_FUNC (GenTextures);
    QUERY_FUNC_(CreateTextures,                 "");
    PLAIN_FUNC (DeleteTextures);
    QUERY_FUNC (ActiveTexture,                  "", "ARB");
    PLAIN_FUNC (BindTexture);
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
    QUERY_FUNC (GetTextureHandle,               "ARB", "NV");
    QUERY_FUNC (MakeTextureHandleResident,      "ARB", "NV");
    QUERY_FUNC (MakeTextureHandleNonResident,   "ARB", "NV");
    QUERY_FUNC (GetImageHandle,                 "ARB", "NV");
    QUERY_FUNC (MakeImageHandleResident,        "ARB", "NV");
    QUERY_FUNC (MakeImageHandleNonResident,     "ARB", "NV");
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
    QUERY_FUNC_(GenFramebuffers,                            "", "EXT");
    QUERY_FUNC_(CreateFramebuffers,                         "", "EXT");
    QUERY_FUNC (DeleteFramebuffers,                         "", "EXT");
    QUERY_FUNC_(BindFramebuffer,                            "", "EXT");
    QUERY_FUNC_(BlitNamedFramebuffer,                       "");
    QUERY_FUNC_(BlitFramebuffer,                            "", "EXT");
    QUERY_FUNC (DrawBuffers,                                "", "ARB", "ATI");
    QUERY_FUNC_(InvalidateNamedFramebufferData,             "");
    QUERY_FUNC_(InvalidateFramebuffer,                      "");
    QUERY_FUNC_(DiscardFramebufferEXT,                      "");
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
    PLAIN_FUNC (Clear);
    PLAIN_FUNC (ClearColor);
    PLAIN_FUNC_(ClearDepth);
    QUERY_FUNC_(ClearDepthf,                                "");
    PLAIN_FUNC (ClearStencil);
    QUERY_FUNC_(ClearNamedFramebufferiv,                    "");
    QUERY_FUNC_(ClearBufferiv,                              "");
    QUERY_FUNC_(ClearNamedFramebufferuiv,                   "");
    QUERY_FUNC_(ClearBufferuiv,                             "");
    QUERY_FUNC_(ClearNamedFramebufferfv,                    "");
    QUERY_FUNC_(ClearBufferfv,                              "");
    QUERY_FUNC_(ClearNamedFramebufferfi,                    "");
    QUERY_FUNC_(ClearBufferfi,                              "");

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
    QUERY_FUNC (ProgramUniformHandleui64,           "ARB", "NV");

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
    QUERY_FUNC_(LabelObjectEXT,         "");
    QUERY_FUNC_(ObjectPtrLabel,         "");
    QUERY_FUNC_(PushDebugGroup,         "");
    QUERY_FUNC_(PopDebugGroup,          "");
    QUERY_FUNC_(PushGroupMarkerEXT,     "");
    QUERY_FUNC_(PopGroupMarkerEXT,      "");

    //others
    PLAIN_FUNC (GetError);
    PLAIN_FUNC (GetFloatv);
    PLAIN_FUNC (GetIntegerv);
    PLAIN_FUNC (GetString);
    QUERY_FUNC (GetStringi,             "");
    PLAIN_FUNC (IsEnabled);
    PLAIN_FUNC (Enable);
    PLAIN_FUNC (Disable);
    PLAIN_FUNC (Finish);
    PLAIN_FUNC (Flush);
    PLAIN_FUNC (DepthFunc);
    PLAIN_FUNC (CullFace);
    PLAIN_FUNC (FrontFace);
    PLAIN_FUNC (Viewport);
    QUERY_FUNC (ViewportArrayv,         "", "NV");
    QUERY_FUNC (ViewportIndexedf,       "", "NV");
    QUERY_FUNC (ViewportIndexedfv,      "", "NV");
    QUERY_FUNC (ClipControl,            "", "EXT");
    QUERY_FUNC (MemoryBarrier,          "", "EXT");


    Extensions = GetExtensions();
    {
        VendorString = common::str::to_u16string(
            reinterpret_cast<const char*>(ogluGetString(GL_VENDOR)), common::str::Encoding::UTF8);
        VersionString = common::str::to_u16string(
            reinterpret_cast<const char*>(ogluGetString(GL_VERSION)), common::str::Encoding::UTF8);
        int32_t major = 0, minor = 0;
        ogluGetIntegerv(GL_MAJOR_VERSION, &major);
        ogluGetIntegerv(GL_MINOR_VERSION, &minor);
        Version = major * 10 + minor;
    }
    SupportDebug            = ogluDebugMessageCallback != nullptr;
    SupportSRGB             = PlatFunc->SupportSRGB && (Extensions.Has("GL_ARB_framebuffer_sRGB") || Extensions.Has("GL_EXT_framebuffer_sRGB"));
    SupportClipControl      = ogluClipControl != nullptr;
    SupportGeometryShader   = Version >= 33 || Extensions.Has("GL_ARB_geometry_shader4");
    SupportComputeShader    = Extensions.Has("GL_ARB_compute_shader");
    SupportTessShader       = Extensions.Has("GL_ARB_tessellation_shader");
    SupportBindlessTexture  = Extensions.Has("GL_ARB_bindless_texture") || Extensions.Has("GL_NV_bindless_texture");
    SupportImageLoadStore   = Extensions.Has("GL_ARB_shader_image_load_store") || Extensions.Has("GL_EXT_shader_image_load_store");
    SupportSubroutine       = Extensions.Has("GL_ARB_shader_subroutine");
    SupportIndirectDraw     = Extensions.Has("GL_ARB_draw_indirect");
    SupportBaseInstance     = Extensions.Has("GL_ARB_base_instance") || Extensions.Has("GL_EXT_base_instance");
    SupportVSMultiLayer     = Extensions.Has("GL_ARB_shader_viewport_layer_array") || Extensions.Has("GL_AMD_vertex_shader_layer");
    
    ogluGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS,         &MaxUBOUnits);
    ogluGetIntegerv(GL_MAX_IMAGE_UNITS,                     &MaxImageUnits);
    ogluGetIntegerv(GL_MAX_COLOR_ATTACHMENTS,               &MaxColorAttachment);
    ogluGetIntegerv(GL_MAX_DRAW_BUFFERS,                    &MaxDrawBuffers);
    ogluGetIntegerv(GL_MAX_LABEL_LENGTH,                    &MaxLabelLen);
    ogluGetIntegerv(GL_MAX_DEBUG_MESSAGE_LENGTH,            &MaxMessageLen);
    if (SupportImageLoadStore)
        ogluGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &MaxTextureUnits);

#undef WITH_SUFFIX
#undef WITH_SUFFIXS
#undef QUERY_FUNC
#undef QUERY_FUNC_
#undef PLAIN_FUNC
}


#define CALL_EXISTS(func, ...) if (func) { return func(__VA_ARGS__); }


void CtxFuncs::ogluNamedBufferStorage(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) const
{
    CALL_EXISTS(ogluNamedBufferStorage_, buffer, size, data, flags)
    {
        ogluBindBuffer(target, buffer);
        ogluBufferStorage_(target, size, data, flags);
    }
}
void CtxFuncs::ogluNamedBufferData(GLenum target, GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) const
{
    CALL_EXISTS(ogluNamedBufferData_, buffer, size, data, usage)
    {
        ogluBindBuffer(target, buffer);
        ogluBufferData_(target, size, data, usage);
    }
}
void CtxFuncs::ogluNamedBufferSubData(GLenum target, GLuint buffer, GLintptr offset, GLsizeiptr size, const void* data) const
{
    CALL_EXISTS(ogluNamedBufferSubData_, buffer, offset, size, data)
    {
        ogluBindBuffer(target, buffer);
        ogluBufferSubData_(target, offset, size, data);
    }
}
void* CtxFuncs::ogluMapNamedBuffer(GLenum target, GLuint buffer, GLenum access) const
{
    CALL_EXISTS(ogluMapNamedBuffer_, buffer, access)
    {
        ogluBindBuffer(target, buffer);
        return ogluMapBuffer_(target, access);
    }
}
GLboolean CtxFuncs::ogluUnmapNamedBuffer(GLenum target, GLuint buffer) const
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
    const CtxFuncs& DSA;
    bool Changed;
    VAOBinder(const CtxFuncs* dsa, GLuint newVAO) noexcept :
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
void CtxFuncs::RefreshVAOState() const
{
    const auto lock = DataLock.LockScope();
    ogluGetIntegerv(GL_VERTEX_ARRAY_BINDING, reinterpret_cast<GLint*>(&VAO));
}
void CtxFuncs::ogluBindVertexArray(GLuint vaobj) const
{
    const auto lock = DataLock.LockScope();
    ogluBindVertexArray_(vaobj);
    VAO = vaobj;
}
void CtxFuncs::ogluEnableVertexArrayAttrib(GLuint vaobj, GLuint index) const
{
    CALL_EXISTS(ogluEnableVertexArrayAttrib_, vaobj, index)
    {
        ogluBindVertexArray_(vaobj); // ensure be in binding
        ogluEnableVertexAttribArray_(index);
    }
}
void CtxFuncs::ogluVertexAttribPointer(GLuint index, GLint size, GLenum type, bool normalized, GLsizei stride, size_t offset) const
{
    const auto pointer = reinterpret_cast<const void*>(uintptr_t(offset));
    ogluVertexAttribPointer_(index, size, type, normalized ? GL_TRUE : GL_FALSE, stride, pointer);
}
void CtxFuncs::ogluVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const
{
    const auto pointer = reinterpret_cast<const void*>(uintptr_t(offset));
    ogluVertexAttribIPointer_(index, size, type, stride, pointer);
}
void CtxFuncs::ogluVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, size_t offset) const
{
    const auto pointer = reinterpret_cast<const void*>(uintptr_t(offset));
    ogluVertexAttribLPointer_(index, size, type, stride, pointer);
}


void CtxFuncs::ogluDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, uint32_t instancecount, uint32_t baseinstance) const
{
    if (baseinstance != 0)
        CALL_EXISTS(ogluDrawArraysInstancedBaseInstance_, mode, first, count, static_cast<GLsizei>(instancecount), baseinstance)
    CALL_EXISTS(ogluDrawArraysInstanced_, mode, first, count, static_cast<GLsizei>(instancecount)) // baseInstance ignored
    {
        for (uint32_t i = 0; i < instancecount; i++)
        {
            ogluDrawArrays(mode, first, count);
        }
    }
}
void CtxFuncs::ogluDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, uint32_t instancecount, uint32_t baseinstance) const
{
    if (baseinstance != 0)
        CALL_EXISTS(ogluDrawElementsInstancedBaseInstance_, mode, count, type, indices, static_cast<GLsizei>(instancecount), baseinstance)
    CALL_EXISTS(ogluDrawElementsInstanced_, mode, count, type, indices, static_cast<GLsizei>(instancecount)) // baseInstance ignored
    {
        for (uint32_t i = 0; i < instancecount; i++)
        {
            ogluDrawElements(mode, count, type, indices);
        }
    }
}
void CtxFuncs::ogluMultiDrawArraysIndirect(GLenum mode, const oglIndirectBuffer_& indirect, GLint offset, GLsizei drawcount) const
{
    if (ogluMultiDrawArraysIndirect_)
    {
        ogluBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect.BufferID); //IBO not included in VAO
        const auto pointer = reinterpret_cast<const void*>(uintptr_t(sizeof(oglIndirectBuffer_::DrawArraysIndirectCommand) * offset));
        ogluMultiDrawArraysIndirect_(mode, pointer, drawcount, 0);
    }
    else if (ogluDrawArraysInstancedBaseInstance_)
    {
        const auto& cmd = indirect.GetArrayCommands();
        for (GLsizei i = offset; i < drawcount; i++)
        {
            ogluDrawArraysInstancedBaseInstance_(mode, cmd[i].first, cmd[i].count, cmd[i].instanceCount, cmd[i].baseInstance);
        }
    }
    else if (ogluDrawArraysInstanced_)
    {
        const auto& cmd = indirect.GetArrayCommands();
        for (GLsizei i = offset; i < drawcount; i++)
        {
            ogluDrawArraysInstanced_(mode, cmd[i].first, cmd[i].count, cmd[i].instanceCount); // baseInstance ignored
        }
    }
    else
    {
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"no avaliable implementation for MultiDrawArraysIndirect");
    }
}
void CtxFuncs::ogluMultiDrawElementsIndirect(GLenum mode, GLenum type, const oglIndirectBuffer_& indirect, GLint offset, GLsizei drawcount) const
{
    if (ogluMultiDrawElementsIndirect_)
    {
        ogluBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect.BufferID); //IBO not included in VAO
        const auto pointer = reinterpret_cast<const void*>(uintptr_t(sizeof(oglIndirectBuffer_::DrawArraysIndirectCommand) * offset));
        ogluMultiDrawElementsIndirect_(mode, type, pointer, drawcount, 0);
    }
    else if (ogluDrawElementsInstancedBaseVertexBaseInstance_)
    {
        const auto& cmd = indirect.GetElementCommands();
        for (GLsizei i = offset; i < drawcount; i++)
        {
            const auto pointer = reinterpret_cast<const void*>(uintptr_t(cmd[i].firstIndex));
            ogluDrawElementsInstancedBaseVertexBaseInstance_(mode, cmd[i].count, type, pointer, cmd[i].instanceCount, cmd[i].baseVertex, cmd[i].baseInstance);
        }
    }
    else if (ogluDrawElementsInstancedBaseInstance_)
    {
        const auto& cmd = indirect.GetElementCommands();
        for (GLsizei i = offset; i < drawcount; i++)
        {
            const auto pointer = reinterpret_cast<const void*>(uintptr_t(cmd[i].firstIndex));
            ogluDrawElementsInstancedBaseInstance_(mode, cmd[i].count, type, pointer, cmd[i].instanceCount, cmd[i].baseInstance); // baseInstance ignored
        }
    }
    else if (ogluDrawElementsInstanced_)
    {
        const auto& cmd = indirect.GetElementCommands();
        for (GLsizei i = offset; i < drawcount; i++)
        {
            const auto pointer = reinterpret_cast<const void*>(uintptr_t(cmd[i].firstIndex));
            ogluDrawElementsInstanced_(mode, cmd[i].count, type, pointer, cmd[i].instanceCount); // baseInstance & baseVertex ignored
        }
    }
    else
    {
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"no avaliable implementation for MultiDrawElementsIndirect");
    }
}


void CtxFuncs::ogluCreateTextures(GLenum target, GLsizei n, GLuint* textures) const
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
void CtxFuncs::ogluBindTextureUnit(GLuint unit, GLuint texture, GLenum target) const
{
    CALL_EXISTS(ogluBindTextureUnit_,                   unit,         texture)
    CALL_EXISTS(ogluBindMultiTextureEXT_, GL_TEXTURE0 + unit, target, texture)
    {
        ogluActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(target, texture);
    }
}
void CtxFuncs::ogluTextureBuffer(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer) const
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
void CtxFuncs::ogluGenerateTextureMipmap(GLuint texture, GLenum target) const
{
    CALL_EXISTS(ogluGenerateTextureMipmap_,    texture)
    CALL_EXISTS(ogluGenerateTextureMipmapEXT_, texture, target)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluGenerateMipmap_(target);
    }
}
void CtxFuncs::ogluTextureParameteri(GLuint texture, GLenum target, GLenum pname, GLint param) const
{
    CALL_EXISTS(ogluTextureParameteri_,    texture,         pname, param)
    CALL_EXISTS(ogluTextureParameteriEXT_, texture, target, pname, param)
    {
        glBindTexture(target, texture);
        glTexParameteri(target, pname, param);
    }
}
void CtxFuncs::ogluTextureSubImage1D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(ogluTextureSubImage1D_,    texture,         level, xoffset, width, format, type, pixels)
    CALL_EXISTS(ogluTextureSubImage1DEXT_, texture, target, level, xoffset, width, format, type, pixels)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
    }
}
void CtxFuncs::ogluTextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(ogluTextureSubImage2D_,    texture,         level, xoffset, yoffset, width, height, format, type, pixels)
    CALL_EXISTS(ogluTextureSubImage2DEXT_, texture, target, level, xoffset, yoffset, width, height, format, type, pixels)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
    }
}
void CtxFuncs::ogluTextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(ogluTextureSubImage3D_,    texture,         level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels)
    CALL_EXISTS(ogluTextureSubImage3DEXT_, texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluTexSubImage3D_(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    }
}
void CtxFuncs::ogluTextureImage1D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(ogluTextureImage1DEXT_, texture, target, level, internalformat, width, border, format, type, pixels)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
    }
}
void CtxFuncs::ogluTextureImage2D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(ogluTextureImage2DEXT_, texture, target, level, internalformat, width, height, border, format, type, pixels)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    }
}
void CtxFuncs::ogluTextureImage3D(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) const
{
    CALL_EXISTS(ogluTextureImage3DEXT_, texture, target, level, internalformat, width, height, depth, border, format, type, pixels)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluTexImage3D_(target, level, internalformat, width, height, depth, border, format, type, pixels);
    }
}
void CtxFuncs::ogluCompressedTextureSubImage1D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(ogluCompressedTextureSubImage1D_,    texture,         level, xoffset, width, format, imageSize, data)
    CALL_EXISTS(ogluCompressedTextureSubImage1DEXT_, texture, target, level, xoffset, width, format, imageSize, data)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluCompressedTexSubImage1D_(target, level, xoffset, width, format, imageSize, data);
    }
}
void CtxFuncs::ogluCompressedTextureSubImage2D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(ogluCompressedTextureSubImage2D_,    texture,         level, xoffset, yoffset, width, height, format, imageSize, data)
    CALL_EXISTS(ogluCompressedTextureSubImage2DEXT_, texture, target, level, xoffset, yoffset, width, height, format, imageSize, data)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluCompressedTexSubImage2D_(target, level, xoffset, yoffset, width, height, format, imageSize, data);
    }
}
void CtxFuncs::ogluCompressedTextureSubImage3D(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(ogluCompressedTextureSubImage3D_,    texture,         level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data)
    CALL_EXISTS(ogluCompressedTextureSubImage3DEXT_, texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluCompressedTexSubImage3D_(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    }
}
void CtxFuncs::ogluCompressedTextureImage1D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(ogluCompressedTextureImage1DEXT_, texture, target, level, internalformat, width, border, imageSize, data)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluCompressedTexImage1D_(target, level, internalformat, width, border, imageSize, data);
    }
}
void CtxFuncs::ogluCompressedTextureImage2D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(ogluCompressedTextureImage2DEXT_, texture, target, level, internalformat, width, height, border, imageSize, data)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluCompressedTexImage2D_(target, level, internalformat, width, height, border, imageSize, data);
    }
}
void CtxFuncs::ogluCompressedTextureImage3D(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) const
{
    CALL_EXISTS(ogluCompressedTextureImage3DEXT_, texture, target, level, internalformat, width, height, depth, border, imageSize, data)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluCompressedTexImage3D_(target, level, internalformat, width, height, depth, border, imageSize, data);
    }
}
void CtxFuncs::ogluTextureStorage1D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) const
{
    CALL_EXISTS(ogluTextureStorage1D_,    texture,         levels, internalformat, width)
    CALL_EXISTS(ogluTextureStorage1DEXT_, texture, target, levels, internalformat, width)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluTexStorage1D_(target, levels, internalformat, width);
    }
}
void CtxFuncs::ogluTextureStorage2D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) const
{
    CALL_EXISTS(ogluTextureStorage2D_,    texture,         levels, internalformat, width, height)
    CALL_EXISTS(ogluTextureStorage2DEXT_, texture, target, levels, internalformat, width, height)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluTexStorage2D_(target, levels, internalformat, width, height);
    }
}
void CtxFuncs::ogluTextureStorage3D(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) const
{
    CALL_EXISTS(ogluTextureStorage3D_,    texture,         levels, internalformat, width, height, depth)
    CALL_EXISTS(ogluTextureStorage3DEXT_, texture, target, levels, internalformat, width, height, depth)
    {
        ogluActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        ogluTexStorage3D_(target, levels, internalformat, width, height, depth);
    }
}
void CtxFuncs::ogluGetTextureLevelParameteriv(GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params) const
{
    CALL_EXISTS(ogluGetTextureLevelParameteriv_,    texture,         level, pname, params)
    CALL_EXISTS(ogluGetTextureLevelParameterivEXT_, texture, target, level, pname, params)
    {
        glBindTexture(target, texture);
        glGetTexLevelParameteriv(target, level, pname, params);
    }
}
void CtxFuncs::ogluGetTextureImage(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, size_t bufSize, void* pixels) const
{
    CALL_EXISTS(ogluGetTextureImage_,    texture,         level, format, type, bufSize > INT32_MAX ? INT32_MAX : static_cast<GLsizei>(bufSize), pixels)
    CALL_EXISTS(ogluGetTextureImageEXT_, texture, target, level, format, type, pixels)
    {
        glBindTexture(target, texture);
        glGetTexImage(target, level, format, type, pixels);
    }
}
void CtxFuncs::ogluGetCompressedTextureImage(GLuint texture, GLenum target, GLint level, size_t bufSize, void* img) const
{
    CALL_EXISTS(ogluGetCompressedTextureImage_,    texture,         level, bufSize > INT32_MAX ? INT32_MAX : static_cast<GLsizei>(bufSize), img)
    CALL_EXISTS(ogluGetCompressedTextureImageEXT_, texture, target, level, img)
    {
        glBindTexture(target, texture);
        ogluGetCompressedTexImage_(target, level, img);
    }
}


void CtxFuncs::ogluCreateRenderbuffers(GLsizei n, GLuint* renderbuffers) const
{
    CALL_EXISTS(ogluCreateRenderbuffers_, n, renderbuffers)
    {
        ogluGenRenderbuffers_(n, renderbuffers);
        for (GLsizei i = 0; i < n; ++i)
            ogluBindRenderbuffer_(GL_RENDERBUFFER, renderbuffers[i]);
    }
}
void CtxFuncs::ogluNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) const
{
    CALL_EXISTS(ogluNamedRenderbufferStorage_, renderbuffer, internalformat, width, height)
    {
        ogluBindRenderbuffer_(GL_RENDERBUFFER, renderbuffer);
        ogluRenderbufferStorage_(GL_RENDERBUFFER, internalformat, width, height);
    }
}
void CtxFuncs::ogluNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) const
{
    CALL_EXISTS(ogluNamedRenderbufferStorageMultisample_, renderbuffer, samples, internalformat, width, height)
    {
        ogluBindRenderbuffer_(GL_RENDERBUFFER, renderbuffer);
        ogluRenderbufferStorageMultisample_(GL_RENDERBUFFER, samples, internalformat, width, height);
    }
}
void CtxFuncs::ogluNamedRenderbufferStorageMultisampleCoverage(GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) const
{
    CALL_EXISTS(ogluNamedRenderbufferStorageMultisampleCoverageEXT_, renderbuffer, coverageSamples, colorSamples, internalformat, width, height)
    if (ogluRenderbufferStorageMultisampleCoverageNV_)
    {
        ogluBindRenderbuffer_(GL_RENDERBUFFER, renderbuffer);
        ogluRenderbufferStorageMultisampleCoverageNV_(GL_RENDERBUFFER, coverageSamples, colorSamples, internalformat, width, height);
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
            ChangeRead = true, DSA.ogluBindFramebuffer_(GL_READ_FRAMEBUFFER, newFBO.first);
        if (DSA.DrawFBO != newFBO.second)
            ChangeDraw = true, DSA.ogluBindFramebuffer_(GL_DRAW_FRAMEBUFFER, newFBO.second);
    }
    FBOBinder(const CtxFuncs* dsa, const GLenum target, const GLuint newFBO) noexcept :
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
void CtxFuncs::RefreshFBOState() const
{
    const auto lock = DataLock.LockScope();
    ogluGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&ReadFBO));
    ogluGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&DrawFBO));
}
void CtxFuncs::ogluCreateFramebuffers(GLsizei n, GLuint* framebuffers) const
{
    CALL_EXISTS(ogluCreateFramebuffers_, n, framebuffers)
    {
        ogluGenFramebuffers_(n, framebuffers);
        const auto backup = FBOBinder(this);
        for (GLsizei i = 0; i < n; ++i)
            ogluBindFramebuffer_(GL_READ_FRAMEBUFFER, framebuffers[i]);
    }
}
void CtxFuncs::ogluBindFramebuffer(GLenum target, GLuint framebuffer) const
{
    const auto lock = DataLock.LockScope();
    ogluBindFramebuffer_(target, framebuffer);
    if (target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
        ReadFBO = framebuffer;
    if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
        DrawFBO = framebuffer;
}
void CtxFuncs::ogluBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) const
{
    CALL_EXISTS(ogluBlitNamedFramebuffer_, readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
    {
        const auto backup = FBOBinder(this, { readFramebuffer, drawFramebuffer });
        ogluBlitFramebuffer_(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    }
}
void CtxFuncs::ogluInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum* attachments) const
{
    CALL_EXISTS(ogluInvalidateNamedFramebufferData_, framebuffer, numAttachments, attachments)
    const auto invalidator = ogluInvalidateFramebuffer_ ? ogluInvalidateFramebuffer_ : ogluDiscardFramebufferEXT_;
    if (invalidator)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        invalidator(GL_DRAW_FRAMEBUFFER, numAttachments, attachments);
    }
}
void CtxFuncs::ogluNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) const
{
    CALL_EXISTS(ogluNamedFramebufferRenderbuffer_, framebuffer, attachment, renderbuffertarget, renderbuffer)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        ogluFramebufferRenderbuffer_(GL_DRAW_FRAMEBUFFER, attachment, renderbuffertarget, renderbuffer);
    }
}
void CtxFuncs::ogluNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) const
{
    CALL_EXISTS(ogluNamedFramebufferTexture_, framebuffer, attachment, texture, level)
    if (ogluFramebufferTexture_)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        ogluFramebufferTexture_(GL_DRAW_FRAMEBUFFER, attachment, texture, level);
    }
    else
    {
        switch (textarget)
        {
        case GL_TEXTURE_1D:
            CALL_EXISTS(ogluNamedFramebufferTexture1DEXT_, framebuffer, attachment, textarget, texture, level)
            {
                const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
                ogluFramebufferTexture1D_(GL_DRAW_FRAMEBUFFER, attachment, textarget, texture, level);
            }
            break;
        case GL_TEXTURE_2D:
            CALL_EXISTS(ogluNamedFramebufferTexture2DEXT_, framebuffer, attachment, textarget, texture, level)
            {
                const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
                ogluFramebufferTexture2D_(GL_DRAW_FRAMEBUFFER, attachment, textarget, texture, level);
            }
            break;
        default:
            COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"unsupported textarget with calling FramebufferTexture");
        }
    }
}
void CtxFuncs::ogluNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer) const
{
    CALL_EXISTS(ogluNamedFramebufferTextureLayer_, framebuffer, attachment, texture, level, layer)
    if (ogluFramebufferTextureLayer_)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        ogluFramebufferTextureLayer_(GL_DRAW_FRAMEBUFFER, attachment, texture, level, layer);
    }
    else
    {
        switch (textarget)
        {
        case GL_TEXTURE_3D:
            CALL_EXISTS(ogluNamedFramebufferTexture3DEXT_, framebuffer, attachment, textarget, texture, level, layer)
            {
                const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
                ogluFramebufferTexture3D_(GL_DRAW_FRAMEBUFFER, attachment, textarget, texture, level, layer);
            }
            break;
        default:
            COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"unsupported textarget with calling FramebufferTextureLayer");
        }
    }
}
GLenum CtxFuncs::ogluCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) const
{
    CALL_EXISTS(ogluCheckNamedFramebufferStatus_, framebuffer, target)
    {
        const auto backup = FBOBinder(this, target, framebuffer);
        return ogluCheckFramebufferStatus_(target);
    }
}
void CtxFuncs::ogluGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params) const
{
    CALL_EXISTS(ogluGetNamedFramebufferAttachmentParameteriv_, framebuffer, attachment, pname, params)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        ogluGetFramebufferAttachmentParameteriv_(GL_DRAW_FRAMEBUFFER, attachment, pname, params);
    }
}
void CtxFuncs::ogluClearDepth(GLclampd d) const
{
    CALL_EXISTS(ogluClearDepth_, d)
    CALL_EXISTS(ogluClearDepthf_, static_cast<GLclampf>(d))
    {
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"unsupported textarget with calling ClearDepth");
    }
}
void CtxFuncs::ogluClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value) const
{
    CALL_EXISTS(ogluClearNamedFramebufferiv_, framebuffer, buffer, drawbuffer, value)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        ogluClearBufferiv_(buffer, drawbuffer, value);
    }
}
void CtxFuncs::ogluClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value) const
{
    CALL_EXISTS(ogluClearNamedFramebufferuiv_, framebuffer, buffer, drawbuffer, value)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        ogluClearBufferuiv_(buffer, drawbuffer, value);
    }
}
void CtxFuncs::ogluClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value) const
{
    CALL_EXISTS(ogluClearNamedFramebufferfv_, framebuffer, buffer, drawbuffer, value)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        ogluClearBufferfv_(buffer, drawbuffer, value);
    }
}
void CtxFuncs::ogluClearNamedFramebufferDepthStencil(GLuint framebuffer, GLfloat depth, GLint stencil) const
{
    CALL_EXISTS(ogluClearNamedFramebufferfi_, framebuffer, GL_DEPTH_STENCIL, 0, depth, stencil)
    {
        const auto backup = FBOBinder(this, GL_DRAW_FRAMEBUFFER, framebuffer);
        ogluClearBufferfi_(GL_DEPTH_STENCIL, 0, depth, stencil);
    }
}


void CtxFuncs::ogluSetObjectLabel(GLenum identifier, GLuint id, std::u16string_view name) const
{
    if (ogluObjectLabel_)
    {
        const auto str = common::str::to_u8string(name, common::str::Encoding::UTF16LE);
        ogluObjectLabel_(identifier, id,
            static_cast<GLsizei>(std::min<size_t>(str.size(), MaxLabelLen)),
            reinterpret_cast<const GLchar*>(str.c_str()));
    }
    else if (ogluLabelObjectEXT_)
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
        ogluLabelObjectEXT_(type, id,
            static_cast<GLsizei>(str.size()),
            reinterpret_cast<const GLchar*>(str.c_str()));
    }
}
void CtxFuncs::ogluSetObjectLabel(GLsync sync, std::u16string_view name) const
{
    if (ogluObjectPtrLabel_)
    {
        const auto str = common::str::to_u8string(name, common::str::Encoding::UTF16LE);
        ogluObjectPtrLabel_(sync,
            static_cast<GLsizei>(std::min<size_t>(str.size(), MaxLabelLen)),
            reinterpret_cast<const GLchar*>(str.c_str()));
    }
}
void CtxFuncs::ogluPushDebugGroup(GLenum source, GLuint id, std::u16string_view message) const
{
    if (ogluPushDebugGroup_)
    {
        const auto str = common::str::to_u8string(message, common::str::Encoding::UTF16LE);
        ogluPushDebugGroup_(source, id,
            static_cast<GLsizei>(std::min<size_t>(str.size(), MaxMessageLen)),
            reinterpret_cast<const GLchar*>(str.c_str()));
    }
    else if (ogluPushGroupMarkerEXT_)
    {
        const auto str = common::str::to_u8string(message, common::str::Encoding::UTF16LE);
        ogluPushGroupMarkerEXT_(0, reinterpret_cast<const GLchar*>(str.c_str()));
    }
}
void CtxFuncs::ogluPopDebugGroup() const
{
    if (ogluPopDebugGroup_)
        ogluPopDebugGroup_();
    else if (ogluPopGroupMarkerEXT_)
        ogluPopGroupMarkerEXT_();
}


common::container::FrozenDenseSet<std::string_view> CtxFuncs::GetExtensions() const
{
    if (ogluGetStringi)
    {
        GLint count;
        ogluGetIntegerv(GL_NUM_EXTENSIONS, &count);
        std::vector<std::string_view> exts;
        exts.reserve(count);
        for (GLint i = 0; i < count; i++)
        {
            exts.emplace_back(ogluGetStringi(GL_EXTENSIONS, i));
        }
        return exts;
    }
    else
    {
        const GLubyte* exts = ogluGetString(GL_EXTENSIONS);
        return common::str::Split(reinterpret_cast<const char*>(exts), ' ', false);
    }
}
std::optional<std::string_view> CtxFuncs::GetError() const
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