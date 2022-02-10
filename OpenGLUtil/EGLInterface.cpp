#include "oglPch.h"
#include "oglUtil.h"


#undef APIENTRY
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "angle-Headers/eglext_angle.h"
#pragma message("Compiling OpenGLUtil with egl-ext[" STRINGIZE(EGL_EGLEXT_VERSION) "]")
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)


namespace oglu
{
using namespace std::string_view_literals;


#define DEFINE_FUNC2(type, func, name) using T_P##name = type; static constexpr auto N_##name = #func ""sv

DEFINE_FUNC(eglGetError,               GetError);
DEFINE_FUNC2(PFNEGLQUERYDEVICESEXTPROC, eglQueryDevicesEXT,        QueryDevicesEXT);
DEFINE_FUNC(eglGetDisplay,             GetDisplay);
DEFINE_FUNC(eglGetPlatformDisplay,     GetPlatformDisplay);
DEFINE_FUNC2(PFNEGLGETPLATFORMDISPLAYEXTPROC, eglGetPlatformDisplayEXT,  GetPlatformDisplayEXT);
DEFINE_FUNC(eglInitialize,             Initialize);
DEFINE_FUNC(eglQueryString,            QueryString);
DEFINE_FUNC(eglGetProcAddress,         GetProcAddress);
DEFINE_FUNC(eglChooseConfig,           ChooseConfig);
DEFINE_FUNC(eglGetConfigs,             GetConfigs);
DEFINE_FUNC(eglGetConfigAttrib,        GetConfigAttrib);
DEFINE_FUNC(eglCreateWindowSurface,    CreateWindowSurface);
DEFINE_FUNC(eglCreatePixmapSurface,    CreatePixmapSurface);
DEFINE_FUNC(eglCreatePbufferSurface,   CreatePbufferSurface);
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



template<typename T, typename E>
struct EnumBitfield
{
    static_assert(std::is_enum_v<E>);
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>);
    T Value = 0;
    constexpr EnumBitfield operator+(E val) const noexcept
    {
        return { static_cast<T>((1u << common::enum_cast(val)) | Value) };
    }
    constexpr EnumBitfield operator-(E val) const noexcept
    {
        return { static_cast<T>(~(1u << common::enum_cast(val)) & Value) };
    }
    constexpr bool operator()(E val) const noexcept
    {
        return (Value & static_cast<T>(1u << common::enum_cast(val))) != 0;
    }
    constexpr EnumBitfield& operator+=(E val) noexcept
    {
        Value = operator+(val).Value;
        return *this;
    }
    constexpr EnumBitfield& operator-=(E val) noexcept
    {
        Value = operator-(val).Value;
        return *this;
    }
    constexpr EnumBitfield& Add(E val) noexcept
    {
        return operator+=(val);
    }
    constexpr EnumBitfield& Remove(E val) noexcept
    {
        return operator-=(val);
    }
};


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
        bool IsOffscreen;
        EGLHost(EGLLoader_& loader, EGLDisplay dc, bool isOffscreen) : EGLLoader::EGLHost(loader), DeviceContext(dc), IsOffscreen(isOffscreen)
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
            SupportSRGBFrameBuffer = Extensions.Has("EGL_KHR_gl_colorspace");
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
                //Surface = loader.CreatePixmapSurface(DeviceContext, Config, (EGLNativePixmapType)surface, nullptr);
                Surface = loader.CreatePbufferSurface(DeviceContext, Config, nullptr);
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
    DECLARE_FUNC(QueryDevicesEXT);
    DECLARE_FUNC(GetDisplay);
    DECLARE_FUNC(GetPlatformDisplay);
    DECLARE_FUNC(GetPlatformDisplayEXT);
    DECLARE_FUNC(Initialize);
    DECLARE_FUNC(QueryString);
    DECLARE_FUNC(GetProcAddress);
    DECLARE_FUNC(ChooseConfig);
    DECLARE_FUNC(GetConfigs);
    DECLARE_FUNC(GetConfigAttrib);
    DECLARE_FUNC(CreateWindowSurface);
    DECLARE_FUNC(CreatePixmapSurface);
    DECLARE_FUNC(CreatePbufferSurface);
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
    common::container::FrozenDenseSet<std::string_view> Extensions;
    EnumBitfield<uint16_t, AngleBackend> BackendMask;
#if COMMON_OS_DARWIN && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    EGLType Type = EGLType::EAGL;
#else
    EGLType Type = EGLType::Unknown;
#endif
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
        TrLd_FUNC(EGL, QueryDevicesEXT);
        LOAD_FUNC(EGL, GetDisplay);
        TrLd_FUNC(EGL, GetPlatformDisplay);
        TrLd_FUNC(EGL, GetPlatformDisplayEXT);
        LOAD_FUNC(EGL, Initialize);
        LOAD_FUNC(EGL, QueryString);
        LOAD_FUNC(EGL, GetProcAddress);
        LOAD_FUNC(EGL, ChooseConfig);
        LOAD_FUNC(EGL, GetConfigs);
        LOAD_FUNC(EGL, GetConfigAttrib);
        LOAD_FUNC(EGL, CreateWindowSurface);
        LOAD_FUNC(EGL, CreatePixmapSurface);
        LOAD_FUNC(EGL, CreatePbufferSurface);
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
        const auto exts = QueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
        if (exts)
        {
            Extensions = common::str::Split(exts, ' ', false);
            std::string extNames;
            for (const auto ext : Extensions)
                extNames.append(ext).push_back('\n');
            oglLog().debug(u"Loaded EGL with client exts:\n{}", extNames);
            if (Extensions.Has("EGL_KHR_platform_android") || Extensions.Has("EGL_ANDROID_GLES_layers"sv))
                Type = EGLType::ANDROID;
            if (Extensions.Has("EGL_ANGLE_platform_angle"sv))
                Type = EGLType::ANGLE;
            else if (Extensions.Has("EGL_MESA_platform_gbm"sv) || Extensions.Has("EGL_MESA_platform_surfaceless"sv))
                Type = EGLType::MESA;

            if (Extensions.Has("EGL_ANGLE_platform_angle_d3d"sv))
                BackendMask.Add(AngleBackend::D3D9).Add(AngleBackend::D3D11);
            if (Extensions.Has("EGL_ANGLE_platform_angle_d3d11on12"sv))
                BackendMask.Add(AngleBackend::D3D11on12);
            if (Extensions.Has("EGL_ANGLE_platform_angle_opengl"sv))
                BackendMask.Add(AngleBackend::GL).Add(AngleBackend::GLES);
            if (Extensions.Has("EGL_ANGLE_platform_angle_vulkan"sv))
                BackendMask.Add(AngleBackend::Vulkan);
            if (Extensions.Has("EGL_ANGLE_platform_angle_metal"sv))
                BackendMask.Add(AngleBackend::Metal);
            /*if (Extensions.Has("EGL_EXT_device_base"sv) || (Extensions.Has("EGL_EXT_device_enumeration"sv) && Extensions.Has("EGL_EXT_device_query"sv)))
            {
                std::array<EGLDeviceEXT, 16> devices = { nullptr };
                EGLint count = 0;
                if (EGL_TRUE == QueryDevicesEXT(devices.size(), devices.data(), &count))
                {
                }
            }*/
        }
    }
    ~EGLLoader_() final {}
private:
    EGLType GetType() const noexcept final { return Type; }
    bool CheckSupport(AngleBackend backend) const noexcept final
    {
        if (backend == AngleBackend::Any)
            return BackendMask.Value != 0;
        return BackendMask(backend);
    }

    std::string_view Name() const noexcept final { return LoaderName; }
    std::u16string Description() const noexcept final
    {
        std::u16string ret;
        switch (Type)
        {
        case EGLType::ANDROID: ret = u"Android"; break;
        case EGLType::ANGLE:   ret = u"Angle";   break;
        case EGLType::EAGL:    ret = u"EAGL";    break;
        case EGLType::MESA:    ret = u"MESA";    break;
        default: break;
        }
        return ret;
    }
    std::u16string_view GetCurErrStr() const noexcept { return GetErrorString(GetError()); }

    /*void Init() override
    { }*/
    std::shared_ptr<EGLLoader::EGLHost> CreateFromDisplay(EGLDisplay display, bool useOffscreen)
    {
        if (!display)
            COMMON_THROWEX(common::BaseException, fmt::format(u"Unable to get EGL Display: [{}]"sv, GetCurErrStr()));
        auto host = std::make_shared<EGLHost>(*this, display, useOffscreen);
        detail::AttribList cfgAttrib(EGL_NONE);
        cfgAttrib.Set(EGL_SURFACE_TYPE, useOffscreen ? EGL_PBUFFER_BIT : EGL_WINDOW_BIT);
        cfgAttrib.Set(EGL_RED_SIZE,     8);
        cfgAttrib.Set(EGL_GREEN_SIZE,   8);
        cfgAttrib.Set(EGL_BLUE_SIZE,    8);
        cfgAttrib.Set(EGL_ALPHA_SIZE,   8);
        cfgAttrib.Set(EGL_STENCIL_SIZE, 8);
        cfgAttrib.Set(EGL_DEPTH_SIZE,   24);
        EGLint rdBit = 0;
        if (host->SupportDesktop) rdBit |= EGL_OPENGL_BIT;
        if (host->SupportES)      rdBit |= EGL_OPENGL_ES2_BIT;
        if (host->Version >= 15)  rdBit |= EGL_OPENGL_ES3_BIT;
        cfgAttrib.Set(EGL_RENDERABLE_TYPE, rdBit);
        int cfgCount = 0;
        if (EGL_TRUE != ChooseConfig(display, cfgAttrib.Data(), &host->Config, 1, &cfgCount))
            COMMON_THROWEX(common::BaseException, fmt::format(u"Unable to choose EGL Config: [{}]"sv, GetCurErrStr()));
        if (cfgCount == 0)
        {
            EGLint cfgTotal = 0;
            if (EGL_TRUE != GetConfigs(display, nullptr, 0, &cfgTotal))
                COMMON_THROWEX(common::BaseException, fmt::format(u"Unable to get EGL Configs: [{}]"sv, GetCurErrStr()));
            std::u16string result;
            std::vector<EGLConfig> configs; 
            configs.resize(cfgTotal);
            GetConfigs(display, configs.data(), cfgTotal, &cfgTotal);
            for (EGLint i = 0; i < cfgTotal; ++i)
            {
                const auto& cfg = configs[i];
                APPEND_FMT(result, u"cfg[{}]: "sv, i);
                for (const auto& [k, v] : cfgAttrib)
                {
                    EGLint v2 = 0;
                    if (EGL_TRUE == GetConfigAttrib(display, cfg, k, &v2))
                    {
                        const auto match = (k == EGL_RENDERABLE_TYPE || k == EGL_SURFACE_TYPE) ? (v & v2) == v : v == v2;
                        if (!match)
                            APPEND_FMT(result, u"[{:04X}]:[{:04X}]vs[{:04X}], "sv, k, v, v2);
                    }
                }
                result.append(u"\n");
            }
            oglLog().debug(u"Totally [{}] configs avaliable:\n{}\n", cfgTotal, result);
            COMMON_THROWEX(common::BaseException, u"Unable to choose EGL Config"sv);
        }
        if (EGL_TRUE != GetConfigAttrib(display, host->Config, EGL_NATIVE_VISUAL_ID, &host->VisualId))
            COMMON_THROWEX(common::BaseException, fmt::format(u"Unable to get EGL VisualId: [{}]"sv, GetCurErrStr()));
        return host;
    }

    std::shared_ptr<EGLLoader::EGLHost> CreateHost(NativeDisplay display, bool useOffscreen) final
    {
        EGLDisplay disp = GetDisplay((EGLNativeDisplayType)(display));
        if (!disp && useOffscreen && display)
            disp = GetDisplay(EGL_DEFAULT_DISPLAY);
        return CreateFromDisplay(disp, useOffscreen);
    }

    bool CheckSupportPlatformDisplay()
    {
        if (GetPlatformDisplay || GetPlatformDisplayEXT)
            return true;
        oglLog().warning(u"EGL Loader does not support GetPlatformDisplay.\n");
        return false;
    }
    template<typename F>
    EGLDisplay GetPlatformDisplayCombine(EGLenum platform, void* display, F&& fillAttrib) const
    {
        if (GetPlatformDisplay)
        {
            detail::AttribList<EGLAttrib> dpyAttrib(EGL_NONE);
            fillAttrib(dpyAttrib);
            return GetPlatformDisplay(platform, display, dpyAttrib.Data());
        }
        else
        {
            detail::AttribList<EGLint> dpyAttrib(EGL_NONE);
            fillAttrib(dpyAttrib);
            return GetPlatformDisplayEXT(platform, display, dpyAttrib.Data());
        }
    }
    std::shared_ptr<EGLLoader::EGLHost> CreateHostFromXcb(void* connection, std::optional<int32_t> screen, bool useOffscreen) final
    {
        if (CheckSupportPlatformDisplay())
        {
            if (Extensions.Has("EGL_MESA_platform_xcb"sv))
            {
                EGLDisplay disp = GetPlatformDisplayCombine(EGL_PLATFORM_XCB_EXT, connection, [&](auto& list)
                    {
                        if (screen)
                            list.Set(EGL_PLATFORM_XCB_SCREEN_EXT, screen.value());
                    });
                return CreateFromDisplay(disp, useOffscreen);
            }
            oglLog().warning(u"EGL Loader does not support create from xcb.\n");
        }
        return {};
    }
    std::shared_ptr<EGLLoader::EGLHost> CreateHostFromX11(void* disp, std::optional<int32_t> screen, bool useOffscreen) final
    {
        if (CheckSupportPlatformDisplay())
        {
            if (Extensions.Has("EGL_EXT_platform_x11"sv) || Extensions.Has("EGL_KHR_platform_x11"sv))
            {
                EGLDisplay display = GetPlatformDisplayCombine(EGL_PLATFORM_X11_EXT, disp, [&](auto& list)
                    {
                        if (screen)
                            list.Set(EGL_PLATFORM_X11_SCREEN_EXT, screen.value());
                    });
                return CreateFromDisplay(display, useOffscreen);
            }
            oglLog().warning(u"EGL Loader does not support create from x11.\n");
        }
        return {};
    }
    std::shared_ptr<EGLLoader::EGLHost> CreateHostFromAngle(NativeDisplay disp, AngleBackend backend, bool useOffscreen) final
    {
        if (!BackendMask(backend))
            oglLog().warning(u"EGL Loader does not support create from angle.\n");
        else if (CheckSupportPlatformDisplay())
        {
            const int32_t plat = [&]()
            {
                switch (backend)
                {
                case AngleBackend::D3D9:        return EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE;
                case AngleBackend::D3D11:
                case AngleBackend::D3D11on12:   return EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE;
                case AngleBackend::GL:          return EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE;
                case AngleBackend::GLES:        return EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE;
                case AngleBackend::Vulkan:
                case AngleBackend::SwiftShader: return EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE;
                case AngleBackend::Metal:       return EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE;
                default:                        return 0;
                }
            }();
            EGLDisplay display = GetPlatformDisplayCombine(EGL_PLATFORM_ANGLE_ANGLE, disp, [&](auto& list)
                {
                    if (plat != 0)
                        list.Set(EGL_PLATFORM_ANGLE_TYPE_ANGLE, plat);
                    if (backend == AngleBackend::D3D11on12)
                        list.Set(EGL_PLATFORM_ANGLE_D3D11ON12_ANGLE, EGL_TRUE);
                    else if (backend == AngleBackend::SwiftShader)
                        list.Set(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_DEVICE_TYPE_SWIFTSHADER_ANGLE);
                    if (backend == AngleBackend::D3D11 || backend == AngleBackend::D3D11on12)
                        list.Set(EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_FALSE);
                });
            return CreateFromDisplay(display, useOffscreen);
        }
        return {};
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
    CASE_ERR(BAD_CONFIG);
    CASE_ERR(BAD_CONTEXT);
    CASE_ERR(BAD_CURRENT_SURFACE);
    CASE_ERR(BAD_DISPLAY);
    CASE_ERR(BAD_MATCH);
    CASE_ERR(BAD_NATIVE_PIXMAP);
    CASE_ERR(BAD_NATIVE_WINDOW);
    CASE_ERR(BAD_PARAMETER);
    CASE_ERR(BAD_SURFACE);
    CASE_ERR(CONTEXT_LOST);
    CASE_ERR(BAD_STREAM_KHR);
    CASE_ERR(BAD_STATE_KHR);
    CASE_ERR(BAD_DEVICE_EXT);
    CASE_ERR(BAD_OUTPUT_LAYER_EXT);
    CASE_ERR(BAD_OUTPUT_PORT_EXT);
#undef CASE_ERR
    default: return u"Unknwon"sv;
    }
}


std::u16string_view EGLLoader::GetAngleBackendName(AngleBackend backend) noexcept
{
    switch (backend)
    {
#define CASE_BE(name) case AngleBackend::name: return u"" #name ""sv
    CASE_BE(D3D9);
    CASE_BE(D3D11);
    CASE_BE(D3D11on12);
    CASE_BE(GL);
    CASE_BE(GLES);
    CASE_BE(Vulkan);
    CASE_BE(SwiftShader);
    CASE_BE(Metal);
#undef CASE_BE
    default: return u"Any"sv;
    }
}


}
