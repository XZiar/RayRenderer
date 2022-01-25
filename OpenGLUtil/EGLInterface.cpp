#include "oglPch.h"
#include "oglUtil.h"
#include "SystemCommon/DynamicLibrary.h"


#undef APIENTRY
#include "EGL/egl.h"
#include "EGL/eglext.h"
#pragma message("Compiling OpenGLUtil with egl-ext[" STRINGIZE(EGL_EGLEXT_VERSION) "]")


namespace oglu
{
using namespace std::string_view_literals;


constexpr EGLint VisualAttrs[] =
{
    EGL_SURFACE_TYPE,   EGL_WINDOW_BIT | EGL_PIXMAP_BIT,
    EGL_RED_SIZE,       8,
    EGL_GREEN_SIZE,     8,
    EGL_BLUE_SIZE,      8,
    EGL_ALPHA_SIZE,     8,
    EGL_DEPTH_SIZE,     24,
    EGL_STENCIL_SIZE,   8,
    EGL_NONE
};


#define PREPARE_FUNC(func, name) using P##name = decltype(&func); static constexpr auto N##name = #func ""sv
#define PREPARE_FUNC2(type, func, name) using P##name = type; static constexpr auto N##name = #func ""sv

PREPARE_FUNC(eglGetError,               GetError);
PREPARE_FUNC(eglGetDisplay,             GetDisplay);
PREPARE_FUNC(eglInitialize,             Initialize);
PREPARE_FUNC(eglQueryString,            QueryString);
PREPARE_FUNC(eglGetProcAddress,         GetProcAddress);
PREPARE_FUNC(eglChooseConfig,           ChooseConfig);
PREPARE_FUNC(eglGetConfigAttrib,        GetConfigAttrib);
PREPARE_FUNC(eglCreateWindowSurface,    CreateWindowSurface);
PREPARE_FUNC(eglCreatePixmapSurface,    CreatePixmapSurface);
PREPARE_FUNC(eglBindAPI,                BindAPI);
PREPARE_FUNC(eglCreateContext,          CreateContext);
PREPARE_FUNC(eglMakeCurrent,            MakeCurrent);
PREPARE_FUNC(eglQuerySurface,           QuerySurface);
PREPARE_FUNC(eglGetCurrentDisplay,      GetCurrentDisplay);
PREPARE_FUNC(eglGetCurrentContext,      GetCurrentContext);
PREPARE_FUNC(eglGetCurrentSurface,      GetCurrentSurface);
PREPARE_FUNC(eglDestroyContext,         DestroyContext);
PREPARE_FUNC(eglDestroySurface,         DestroySurface);
PREPARE_FUNC(eglTerminate,              Terminate);
PREPARE_FUNC(eglSwapBuffers,            SwapBuffers);

#undef PREPARE_FUNC2
#undef PREPARE_FUNC

class EGLLoader_ : public EGLLoader
{
private:
    static constexpr std::u16string_view GetErrorString(EGLint err) noexcept;
    struct EGLHost final : public EGLLoader::EGLHost
    {
        friend EGLLoader_;
        EGLDisplay DeviceContext;
        std::u16string Vendor;
        std::u16string VersionString;
        EGLConfig Config = nullptr;
        EGLSurface Surface = nullptr;
        EGLint VisualId = 0;
        uint16_t Version = 0;
        bool IsOffscreen = false;
        EGLHost(EGLLoader_& loader, EGLDisplay dc) : EGLLoader::EGLHost(loader), DeviceContext(dc)
        {
            int verMajor = 0, verMinor = 0;
            if (loader.Initialize(DeviceContext, &verMajor, &verMinor) != EGL_TRUE)
                COMMON_THROWEX(common::BaseException, fmt::format(u"Unable to initialize EGL: [{}]"sv, loader.GetCurErrStr()));
            Version = gsl::narrow_cast<uint16_t>(verMajor * 10 + verMinor);
            
            const auto clientAPIs = loader.QueryString(DeviceContext, EGL_CLIENT_APIS);
            oglLog().verbose(u"EGL Support API: [{}]\n", clientAPIs);
            for (const auto api : common::str::SplitStream(clientAPIs, ' ', false))
            {
                if (api == "OpenGL_ES") SupportES = true;
                else if (api == "OpenGL") SupportDesktop = true;
            }

            const auto vendor = loader.QueryString(DeviceContext, EGL_VENDOR);
            Vendor = common::str::to_u16string(vendor, common::str::Encoding::UTF8);

            const auto version = loader.QueryString(DeviceContext, EGL_VERSION);
            VersionString = common::str::to_u16string(version, common::str::Encoding::UTF8);

            const auto exts = loader.QueryString(DeviceContext, EGL_EXTENSIONS);
            Extensions = common::str::Split(exts, ' ', false);
            SupportSRGB = Extensions.Has("EGL_KHR_gl_colorspace");
            SupportFlushControl = Extensions.Has("EGL_KHR_context_flush_control");
        }
        ~EGLHost() final
        {
            auto& loader = static_cast<EGLLoader_&>(Loader);
            if (Surface)
                loader.DestroySurface(DeviceContext, Surface);
            loader.Terminate(DeviceContext);
        }
        bool MakeGLContextCurrent_(void* hRC) const final
        {
            return static_cast<EGLLoader_&>(Loader).MakeCurrent(DeviceContext, Surface, Surface, (EGLContext)hRC);
        }
        void DeleteGLContext(void* hRC) const final
        {
            static_cast<EGLLoader_&>(Loader).DestroyContext(DeviceContext, (EGLContext)hRC);
        }
        void SwapBuffer() const final
        {
            static_cast<EGLLoader_&>(Loader).SwapBuffers(DeviceContext, Surface);
        }
        void ReportFailure(std::u16string_view action) const final
        {
            oglLog().error(u"Failed to {} on Display[{}] {}[{}], error: {}\n"sv, action, (void*)DeviceContext,
                IsOffscreen ? u"Pixmap"sv : u"Window"sv, (void*)Surface,
                (int32_t)errno);
        }
        void TemporalInsideContext(void* hRC, const std::function<void(void* hRC)>& func) const final
        {
            auto& loader = static_cast<const EGLLoader_&>(Loader);
            const auto oldHdc   = loader.GetCurrentDisplay();
            const auto oldSurfR = loader.GetCurrentSurface(EGL_READ);
            const auto oldSurfW = loader.GetCurrentSurface(EGL_DRAW);
            const auto oldHrc   = loader.GetCurrentContext();
            if (loader.MakeCurrent(DeviceContext, Surface, Surface, (EGLContext)hRC))
            {
                func(hRC);
                loader.MakeCurrent(oldHdc, oldSurfW, oldSurfR, oldHrc);
            }
        }
        const int& GetVisualId() const noexcept final { return VisualId; }
        uint32_t GetVersion() const noexcept final { return Version; }
        void* GetDeviceContext() const noexcept final { return DeviceContext; }
    };
    common::DynLib LibEGL;
#define PREPARE_FUNC(name) P##name name = nullptr
    PREPARE_FUNC(GetError);
    PREPARE_FUNC(GetDisplay);
    PREPARE_FUNC(Initialize);
    PREPARE_FUNC(QueryString);
    PREPARE_FUNC(GetProcAddress);
    PREPARE_FUNC(ChooseConfig);
    PREPARE_FUNC(GetConfigAttrib);
    PREPARE_FUNC(CreateWindowSurface);
    PREPARE_FUNC(CreatePixmapSurface);
    PREPARE_FUNC(BindAPI);
    PREPARE_FUNC(CreateContext);
    PREPARE_FUNC(MakeCurrent);
    PREPARE_FUNC(QuerySurface);
    PREPARE_FUNC(GetCurrentDisplay);
    PREPARE_FUNC(GetCurrentContext);
    PREPARE_FUNC(GetCurrentSurface);
    PREPARE_FUNC(DestroyContext);
    PREPARE_FUNC(DestroySurface);
    PREPARE_FUNC(Terminate);
    PREPARE_FUNC(SwapBuffers);
#undef PREPARE_FUNC
public:
    EGLLoader_() : 
#if COMMON_OS_WIN
        LibEGL("libEGL.dll")
#elif COMMON_OS_DARWIN
        LibEGL("libEGL.dylib")
#else
        LibEGL("libEGL.so")
#endif
    {
#define PREPARE_FUNC(lib, name) name = Lib##lib.GetFunction<P##name>(N##name)
        PREPARE_FUNC(EGL, GetError);
        PREPARE_FUNC(EGL, GetDisplay);
        PREPARE_FUNC(EGL, Initialize);
        PREPARE_FUNC(EGL, QueryString);
        PREPARE_FUNC(EGL, GetProcAddress);
        PREPARE_FUNC(EGL, ChooseConfig);
        PREPARE_FUNC(EGL, GetConfigAttrib);
        PREPARE_FUNC(EGL, CreateWindowSurface);
        PREPARE_FUNC(EGL, CreatePixmapSurface);
        PREPARE_FUNC(EGL, BindAPI);
        PREPARE_FUNC(EGL, CreateContext);
        PREPARE_FUNC(EGL, MakeCurrent);
        PREPARE_FUNC(EGL, QuerySurface);
        PREPARE_FUNC(EGL, GetCurrentDisplay);
        PREPARE_FUNC(EGL, GetCurrentContext);
        PREPARE_FUNC(EGL, GetCurrentSurface);
        PREPARE_FUNC(EGL, DestroyContext);
        PREPARE_FUNC(EGL, DestroySurface);
        PREPARE_FUNC(EGL, Terminate);
        PREPARE_FUNC(EGL, SwapBuffers);
#undef PREPARE_FUNC
    }
    ~EGLLoader_() final {}
private:
    std::string_view Name() const noexcept final { return "EGL"sv; }
    std::u16string_view GetCurErrStr() const noexcept { return GetErrorString(GetError()); }

    /*void Init() override
    { }*/
    std::shared_ptr<EGLLoader::EGLHost> CreateHost(void* display, bool useOffscreen) final
    {
        EGLDisplay disp = GetDisplay(reinterpret_cast<EGLNativeDisplayType>(display));
        if (!disp)
            disp = GetDisplay(EGL_DEFAULT_DISPLAY);
        if (!disp)
            COMMON_THROWEX(common::BaseException, fmt::format(u"Unable to get EGL Display: [{}]"sv, GetCurErrStr()));
        auto host = std::make_shared<EGLHost>(*this, disp);
        int cfgCount = 0;
        if (EGL_TRUE != ChooseConfig(disp, VisualAttrs, &host->Config, 1, &cfgCount) || cfgCount == 0)
            COMMON_THROWEX(common::BaseException, fmt::format(u"Unable to choose EGL Config: [{}]"sv, GetCurErrStr()));
        if (EGL_TRUE != GetConfigAttrib(disp, host->Config, EGL_NATIVE_VISUAL_ID, &host->VisualId))
            COMMON_THROWEX(common::BaseException, fmt::format(u"Unable to get EGL VisualId: [{}]"sv, GetCurErrStr()));
        host->IsOffscreen = useOffscreen;
        return host;
    }
    void InitSurface(EGLLoader::EGLHost& host_, uintptr_t surface) const override
    {
        auto& host = static_cast<EGLHost&>(host_);
        if (host.IsOffscreen)
        {
            host.Surface = CreatePixmapSurface(host.DeviceContext, host.Config, (EGLNativePixmapType)surface, nullptr);
        }
        else
        {
            host.Surface = CreateWindowSurface(host.DeviceContext, host.Config, (EGLNativeWindowType)surface, nullptr);
        }
    }
    void* CreateContext_(const GLHost& host_, const CreateInfo& cinfo, void* sharedCtx) noexcept final
    {
        auto host = std::static_pointer_cast<EGLHost>(host_);
        BindAPI(cinfo.Type == GLType::Desktop ? EGL_OPENGL_API : EGL_OPENGL_ES_API);
        detail::AttribList attrib(EGL_NONE);
        if (cinfo.DebugContext)
            attrib.Set(EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE);
        if (cinfo.Version != 0)
        {
            attrib.Set(EGL_CONTEXT_MAJOR_VERSION, static_cast<int32_t>(cinfo.Version / 10));
            attrib.Set(EGL_CONTEXT_MINOR_VERSION, static_cast<int32_t>(cinfo.Version % 10));
        }
        if (cinfo.FlushWhenSwapContext && host->SupportFlushControl)
        {
            attrib.Set(EGL_CONTEXT_RELEASE_BEHAVIOR_KHR, cinfo.FlushWhenSwapContext.value() ?
                EGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_KHR : EGL_CONTEXT_RELEASE_BEHAVIOR_NONE_KHR);
        }
        return this->CreateContext(host->DeviceContext, host->Config, reinterpret_cast<EGLContext>(sharedCtx), attrib.Data());
    }
    void* GetFunction_(std::string_view name) const noexcept final
    {
        return reinterpret_cast<void*>(GetProcAddress(name.data()));
    }

    static inline const auto Singleton = oglLoader::RegisterLoader<EGLLoader_>();
};


inline constexpr std::u16string_view EGLLoader_::GetErrorString(EGLint err) noexcept
{
    switch (err)
    {
#define CASE_ERR(name) case EGL_##name: return u"" #name ""sv
    CASE_ERR(SUCCESS);
    CASE_ERR(NOT_INITIALIZED);
    CASE_ERR(BAD_ACCESS);
    CASE_ERR(BAD_ALLOC);
    CASE_ERR(BAD_ATTRIBUTE);
    CASE_ERR(BAD_CONTEXT);
    CASE_ERR(BAD_CONFIG);
    CASE_ERR(BAD_CURRENT_SURFACE);
    CASE_ERR(BAD_DISPLAY);
    CASE_ERR(BAD_SURFACE);
    CASE_ERR(BAD_MATCH);
    CASE_ERR(BAD_PARAMETER);
    CASE_ERR(BAD_NATIVE_PIXMAP);
    CASE_ERR(BAD_NATIVE_WINDOW);
    CASE_ERR(CONTEXT_LOST);
    default: return u"Unknwon"sv;
    }
}


}
