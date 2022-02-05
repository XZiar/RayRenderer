#include "oglPch.h"
#include "oglUtil.h"


#undef APIENTRY
#include "EGL/egl.h"
#include "EGL/eglext.h"
#pragma message("Compiling OpenGLUtil with egl-ext[" STRINGIZE(EGL_EGLEXT_VERSION) "]")


namespace oglu
{
using namespace std::string_view_literals;



DEFINE_FUNC(eglGetError,               GetError);
DEFINE_FUNC(eglGetDisplay,             GetDisplay);
DEFINE_FUNC(eglInitialize,             Initialize);
DEFINE_FUNC(eglQueryString,            QueryString);
DEFINE_FUNC(eglGetProcAddress,         GetProcAddress);
DEFINE_FUNC(eglChooseConfig,           ChooseConfig);
DEFINE_FUNC(eglGetConfigAttrib,        GetConfigAttrib);
DEFINE_FUNC(eglCreateWindowSurface,    CreateWindowSurface);
DEFINE_FUNC(eglCreatePixmapSurface,    CreatePixmapSurface);
DEFINE_FUNC(eglBindAPI,                BindAPI);
DEFINE_FUNC(eglCreateContext,          CreateContext);
DEFINE_FUNC(eglMakeCurrent,            MakeCurrent);
DEFINE_FUNC(eglQuerySurface,           QuerySurface);
DEFINE_FUNC(eglGetCurrentDisplay,      GetCurrentDisplay);
DEFINE_FUNC(eglGetCurrentContext,      GetCurrentContext);
DEFINE_FUNC(eglGetCurrentSurface,      GetCurrentSurface);
DEFINE_FUNC(eglDestroyContext,         DestroyContext);
DEFINE_FUNC(eglDestroySurface,         DestroySurface);
DEFINE_FUNC(eglTerminate,              Terminate);
DEFINE_FUNC(eglSwapBuffers,            SwapBuffers);


class EGLLoader_ final : public EGLLoader
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
        void InitSurface(uintptr_t surface) final
        {
            auto& loader = static_cast<EGLLoader_&>(Loader);
            if (IsOffscreen)
            {
                Surface = loader.CreatePixmapSurface(DeviceContext, Config, (EGLNativePixmapType)surface, nullptr);
            }
            else
            {
                Surface = loader.CreateWindowSurface(DeviceContext, Config, (EGLNativeWindowType)surface, nullptr);
            }
        }
        void* CreateContext_(const CreateInfo& cinfo, void* sharedCtx) noexcept final
        {
            auto& loader = static_cast<EGLLoader_&>(Loader);
            loader.BindAPI(cinfo.Type == GLType::Desktop ? EGL_OPENGL_API : EGL_OPENGL_ES_API);
            detail::AttribList attrib(EGL_NONE);
            EGLint ctxFlags = 0;
            if (cinfo.DebugContext)
            {
                if (Version >= 15)
                    attrib.Set(EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE);
                else if (Version >= 14)
                    ctxFlags |= EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
            }
            if (ctxFlags != 0 && Extensions.Has("EGL_KHR_create_context"))
                attrib.Set(EGL_CONTEXT_FLAGS_KHR, ctxFlags);
            if (cinfo.Version != 0)
            {
                attrib.Set(EGL_CONTEXT_MAJOR_VERSION, static_cast<int32_t>(cinfo.Version / 10));
                attrib.Set(EGL_CONTEXT_MINOR_VERSION, static_cast<int32_t>(cinfo.Version % 10));
            }
            if (cinfo.FlushWhenSwapContext && SupportFlushControl)
            {
                attrib.Set(EGL_CONTEXT_RELEASE_BEHAVIOR_KHR, cinfo.FlushWhenSwapContext.value() ?
                    EGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_KHR : EGL_CONTEXT_RELEASE_BEHAVIOR_NONE_KHR);
            }
            return loader.CreateContext(DeviceContext, Config, reinterpret_cast<EGLContext>(sharedCtx), attrib.Data());
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
                IsOffscreen ? u"Pixmap"sv : u"Window"sv, (void*)Surface, static_cast<EGLLoader_&>(Loader).GetCurErrStr());
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
        void* LoadFunction(std::string_view name) const noexcept final
        {
            return reinterpret_cast<void*>(static_cast<EGLLoader_&>(Loader).GetProcAddress(name.data()));
        }
        void* GetBasicFunctions(size_t, std::string_view) const noexcept final
        {
            return nullptr;
        }
    };
    common::DynLib LibEGL;
    DECLARE_FUNC(GetError);
    DECLARE_FUNC(GetDisplay);
    DECLARE_FUNC(Initialize);
    DECLARE_FUNC(QueryString);
    DECLARE_FUNC(GetProcAddress);
    DECLARE_FUNC(ChooseConfig);
    DECLARE_FUNC(GetConfigAttrib);
    DECLARE_FUNC(CreateWindowSurface);
    DECLARE_FUNC(CreatePixmapSurface);
    DECLARE_FUNC(BindAPI);
    DECLARE_FUNC(CreateContext);
    DECLARE_FUNC(MakeCurrent);
    DECLARE_FUNC(QuerySurface);
    DECLARE_FUNC(GetCurrentDisplay);
    DECLARE_FUNC(GetCurrentContext);
    DECLARE_FUNC(GetCurrentSurface);
    DECLARE_FUNC(DestroyContext);
    DECLARE_FUNC(DestroySurface);
    DECLARE_FUNC(Terminate);
    DECLARE_FUNC(SwapBuffers);
public:
    static constexpr std::string_view LoaderName = "EGL"sv;
    EGLLoader_() :
#if COMMON_OS_WIN
        LibEGL("libEGL.dll")
#elif COMMON_OS_DARWIN
        LibEGL("libEGL.dylib")
#else
        LibEGL("libEGL.so")
#endif
    {
        LOAD_FUNC(EGL, GetError);
        LOAD_FUNC(EGL, GetDisplay);
        LOAD_FUNC(EGL, Initialize);
        LOAD_FUNC(EGL, QueryString);
        LOAD_FUNC(EGL, GetProcAddress);
        LOAD_FUNC(EGL, ChooseConfig);
        LOAD_FUNC(EGL, GetConfigAttrib);
        LOAD_FUNC(EGL, CreateWindowSurface);
        LOAD_FUNC(EGL, CreatePixmapSurface);
        LOAD_FUNC(EGL, BindAPI);
        LOAD_FUNC(EGL, CreateContext);
        LOAD_FUNC(EGL, MakeCurrent);
        LOAD_FUNC(EGL, QuerySurface);
        LOAD_FUNC(EGL, GetCurrentDisplay);
        LOAD_FUNC(EGL, GetCurrentContext);
        LOAD_FUNC(EGL, GetCurrentSurface);
        LOAD_FUNC(EGL, DestroyContext);
        LOAD_FUNC(EGL, DestroySurface);
        LOAD_FUNC(EGL, Terminate);
        LOAD_FUNC(EGL, SwapBuffers);
    }
    ~EGLLoader_() final {}
private:
    std::string_view Name() const noexcept final { return LoaderName; }
    std::u16string_view GetCurErrStr() const noexcept { return GetErrorString(GetError()); }

    /*void Init() override
    { }*/
    std::shared_ptr<EGLLoader::EGLHost> CreateHost_(uintptr_t display, bool useOffscreen) final
    {
        EGLDisplay disp = GetDisplay((EGLNativeDisplayType)(display));
        if (!disp && useOffscreen && display)
            disp = GetDisplay(EGL_DEFAULT_DISPLAY);
        if (!disp)
            COMMON_THROWEX(common::BaseException, fmt::format(u"Unable to get EGL Display: [{}]"sv, GetCurErrStr()));
        auto host = std::make_shared<EGLHost>(*this, disp);
        detail::AttribList cfgAttrib(EGL_NONE);
        cfgAttrib.Set(EGL_SURFACE_TYPE, useOffscreen ? EGL_PIXMAP_BIT : EGL_WINDOW_BIT);
        cfgAttrib.Set(EGL_RED_SIZE,     8);
        cfgAttrib.Set(EGL_GREEN_SIZE,   8);
        cfgAttrib.Set(EGL_BLUE_SIZE,    8);
        cfgAttrib.Set(EGL_ALPHA_SIZE,   8);
        cfgAttrib.Set(EGL_STENCIL_SIZE, 8);
        cfgAttrib.Set(EGL_DEPTH_SIZE,   24);
        EGLint cfmBit = 0;
        if (host->SupportDesktop) cfmBit |= EGL_OPENGL_BIT;
        if (host->SupportES)      cfmBit |= EGL_OPENGL_ES2_BIT;
        if (host->Version >= 15)  cfmBit |= EGL_OPENGL_ES3_BIT;
        cfgAttrib.Set(EGL_CONFORMANT, cfmBit);
        int cfgCount = 0;
        if (EGL_TRUE != ChooseConfig(disp, cfgAttrib.Data(), &host->Config, 1, &cfgCount) || cfgCount == 0)
            COMMON_THROWEX(common::BaseException, fmt::format(u"Unable to choose EGL Config: [{}]"sv, GetCurErrStr()));
        if (EGL_TRUE != GetConfigAttrib(disp, host->Config, EGL_NATIVE_VISUAL_ID, &host->VisualId))
            COMMON_THROWEX(common::BaseException, fmt::format(u"Unable to get EGL VisualId: [{}]"sv, GetCurErrStr()));
        host->IsOffscreen = useOffscreen;
        return host;
    }

    static inline const auto Dummy = detail::RegisterLoader<EGLLoader_>();
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
