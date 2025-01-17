#include "oglPch.h"
#include "oglUtil.h"
#include "oglFBO.h"
#include "SystemCommon/StringDetect.h"

#undef APIENTRY
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "angle-Headers/eglext_angle.h"
#pragma message("Compiling OpenGLUtil with egl-ext[" STRINGIZE(EGL_EGLEXT_VERSION) "]")


namespace oglu
{
MAKE_ENABLER_IMPL(oglAHBRenderBuffer_)
using namespace std::string_view_literals;


#define DEFINE_FUNC2(type, func, name) using T_P##name = type; static constexpr auto N_##name = #func ""sv

DEFINE_FUNC(eglGetError,               GetError);
DEFINE_FUNC(eglGetDisplay,             GetDisplay);
DEFINE_FUNC(eglGetPlatformDisplay,     GetPlatformDisplay);
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
DEFINE_FUNC(eglCreateImage,            CreateImage);
DEFINE_FUNC(eglDestroyImage,           DestroyImage);
DEFINE_FUNC2(PFNEGLQUERYDEVICESEXTPROC,         eglQueryDevicesEXT,         QueryDevicesEXT);
DEFINE_FUNC2(PFNEGLQUERYDEVICESTRINGEXTPROC,    eglQueryDeviceStringEXT,    QueryDeviceStringEXT);
DEFINE_FUNC2(PFNEGLGETPLATFORMDISPLAYEXTPROC,   eglGetPlatformDisplayEXT,   GetPlatformDisplayEXT);
DEFINE_FUNC2(PFNEGLQUERYDISPLAYATTRIBEXTPROC,   eglQueryDisplayAttribEXT,   QueryDisplayAttribEXT);
DEFINE_FUNC2(PFNEGLQUERYDEVICEBINARYEXTPROC,    eglQueryDeviceBinaryEXT,    QueryDeviceBinaryEXT);
DEFINE_FUNC2(PFNEGLQUERYDEVICEATTRIBEXTPROC,    eglQueryDeviceAttribEXT,    QueryDeviceAttribEXT);
DEFINE_FUNC2(PFNEGLCREATEIMAGEKHRPROC,          eglCreateImageKHR,          CreateImageKHR);
DEFINE_FUNC2(PFNEGLDESTROYIMAGEKHRPROC,         eglDestroyImageKHR,         DestroyImageKHR);
DEFINE_FUNC2(PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC, eglGetNativeClientBufferANDROID, GetNativeClientBufferANDROID);


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
    friend oglEGLImage_;
private:
    struct FBConfig
    {
        EGLConfig Config = nullptr;
        EGLint VisualId = 0;
        constexpr explicit operator bool() const noexcept { return Config != nullptr; }
    };
    static constexpr std::u16string_view GetErrorString(EGLint err) noexcept;
    struct EGLHost final : public EGLLoader::EGLHost
    {
        friend EGLLoader_;
        friend oglEGLImage_;
        EGLDisplay DeviceContext;
        const EGLLoader::DeviceHolder* RefDevice = nullptr;
        EGLLoader::DeviceHolder SelfDevice;
        std::u16string Vendor;
        std::u16string VersionString;
        FBConfig CommonConfig, GLConfig, GLESConfig;
        EGLSurface Surface = nullptr;
        uint16_t Version = 0;
        bool IsOffscreen;
        EGLHost(EGLLoader_& loader, EGLDisplay dc, bool isOffscreen) : EGLLoader::EGLHost(loader), DeviceContext(dc), IsOffscreen(isOffscreen)
        {
            int verMajor = 0, verMinor = 0;
            if (loader.Initialize(DeviceContext, &verMajor, &verMinor) != EGL_TRUE)
                COMMON_THROWEX(common::BaseException, FMTSTR2(u"Unable to initialize EGL: [{}]"sv, loader.GetCurErrStr()));
            Version = gsl::narrow_cast<uint16_t>(verMajor * 10 + verMinor);
            
            const auto clientAPIs = loader.QueryString(DeviceContext, EGL_CLIENT_APIS);
            oglLog().Verbose(u"EGL Support API: [{}]\n", clientAPIs);
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

            if (loader.QueryDisplayAttribEXT)
            {
                EGLAttrib result = 0;
                if (EGL_TRUE == loader.QueryDisplayAttribEXT(DeviceContext, EGL_DEVICE_EXT, &result))
                {
                    SelfDevice.Cookie = reinterpret_cast<EGLDeviceEXT>(result);
                    for (const auto& dev : loader.GetDeviceList())
                    {
                        if (dev.Cookie == SelfDevice.Cookie)
                        {
                            RefDevice = &dev;
                            break;
                        }
                    }
                    if (RefDevice == nullptr)
                    {
                        loader.InitDevice(SelfDevice);
                    }
                }
            }

        }
        ~EGLHost() final
        {
            auto& loader = static_cast<EGLLoader_&>(Loader);
            if (Surface)
                loader.DestroySurface(DeviceContext, Surface);
            loader.Terminate(DeviceContext);
        }
        const FBConfig& GetConfig(std::optional<GLType> type) const
        {
            if (CommonConfig)
                return CommonConfig;
            if (type)
                return *type == GLType::Desktop ? GLConfig : GLESConfig;
            return GLConfig ? GLConfig : GLESConfig;
        }
        void InitSurface(uintptr_t surface) final
        {
            auto& loader = static_cast<EGLLoader_&>(Loader);
            if (IsOffscreen)
            {
                //Surface = loader.CreatePixmapSurface(DeviceContext, Config, (EGLNativePixmapType)surface, nullptr);
                Surface = loader.CreatePbufferSurface(DeviceContext, GetConfig({}).Config, nullptr);
            }
            else
            {
                Surface = loader.CreateWindowSurface(DeviceContext, GetConfig({}).Config, (EGLNativeWindowType)surface, nullptr);
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
            return loader.CreateContext(DeviceContext, GetConfig(cinfo.Type).Config, reinterpret_cast<EGLContext>(sharedCtx), attrib.Data());
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
            oglLog().Error(u"Failed to {} on Display[{}] {}[{}], error: {}\n"sv, action, (void*)DeviceContext,
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
        const int& GetVisualId() const noexcept final { return GetConfig({}).VisualId; }
        uint32_t GetVersion() const noexcept final { return Version; }
        const xcomp::CommonDeviceInfo* GetCommonDevice() const noexcept final 
        { 
            const auto dev = GetDeviceInfo();
            return dev ? dev->XCompDevice : nullptr;
        }
        void* GetDeviceContext() const noexcept final { return DeviceContext; }
        const DeviceHolder* GetDeviceInfo() const noexcept final
        {
            if (RefDevice) return RefDevice;
            if (SelfDevice.Cookie) return &SelfDevice;
            return nullptr;
        }
        std::shared_ptr<oglEGLImage_> CreateImageFromAndroidHWBuffer(void* ahb) const noexcept final
        {
            if (Extensions.Has("EGL_ANDROID_get_native_client_buffer"))
            {
                auto& loader = static_cast<EGLLoader_&>(Loader);
                Ensures(loader.GetNativeClientBufferANDROID && (loader.CreateImage || loader.CreateImageKHR));
                // Create EGLImages for the buffers
                const auto cbuf = loader.GetNativeClientBufferANDROID(reinterpret_cast<const AHardwareBuffer*>(ahb));
                EGLImage img = nullptr;
                if (loader.CreateImage)
                {
                    EGLAttrib imageAttribs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
                    img = loader.CreateImage(DeviceContext, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, cbuf, imageAttribs);
                }
                else
                {
                    EGLint imageAttribs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
                    loader.CreateImageKHR(DeviceContext, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, cbuf, imageAttribs);
                }
                if (img != EGL_NO_IMAGE_KHR)
                {
                    return std::make_shared<oglEGLImage_>(shared_from_this(), img);
                }
            }
            else
                oglLog().Warning(u"EGL Host does not support create image from Android Hardware Buffer.\n");
            return {};
        }
        std::shared_ptr<oglAHBRenderBuffer_> CreateRBOFromAndroidHWBuffer(void* ahb, uint32_t w, uint32_t h) const noexcept final
        {
            auto image = CreateImageFromAndroidHWBuffer(ahb);
            if (image)
            {
                return MAKE_ENABLER_SHARED(oglAHBRenderBuffer_, (std::move(image), w, h));
            }
            return {};
        }
        void DestroyImage(EGLImage image) const noexcept
        {
            auto& loader = static_cast<EGLLoader_&>(Loader);
            if (loader.DestroyImage)
            {
                loader.DestroyImage(DeviceContext, image);
            }
            else
            {
                loader.DestroyImageKHR(DeviceContext, image);
            }
        }
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
    const xcomp::CommonDeviceContainer& XCompDevs;
    DECLARE_FUNC(GetError);
    DECLARE_FUNC(GetDisplay);
    DECLARE_FUNC(GetPlatformDisplay);
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
    DECLARE_FUNC(CreateImage);
    DECLARE_FUNC(DestroyImage);
    DECLARE_FUNC(QueryDevicesEXT);
    DECLARE_FUNC(QueryDeviceStringEXT);
    DECLARE_FUNC(GetPlatformDisplayEXT);
    DECLARE_FUNC(QueryDisplayAttribEXT);
    DECLARE_FUNC(QueryDeviceBinaryEXT);
    DECLARE_FUNC(QueryDeviceAttribEXT);
    DECLARE_FUNC(CreateImageKHR);
    DECLARE_FUNC(DestroyImageKHR);
    DECLARE_FUNC(GetNativeClientBufferANDROID);
    common::container::FrozenDenseSet<std::string_view> Extensions;
    EnumBitfield<uint16_t, AngleBackend> BackendMask;
    EGLType Type = EGLType::Unknown;
public:
    static constexpr std::string_view LoaderName = "EGL"sv;
    EGLLoader_(const common::fs::path& dllpath) : LibEGL(dllpath), XCompDevs(xcomp::ProbeDevice())
    {
        GetLoadedDlls().emplace_back(&LibEGL);
        LOAD_FUNC(EGL, GetError);
        LOAD_FUNC(EGL, GetDisplay);
        TrLd_FUNC(EGL, GetPlatformDisplay);
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
        TrLd_FUNC(EGL, CreateImage);
        TrLd_FUNC(EGL, DestroyImage);

#define LdEGLFUNC(lib, name) name = reinterpret_cast<T_P##name>(GetProcAddress(N_##name.data())); if (!name) TrLd_FUNC(lib, name)
        LdEGLFUNC(EGL, QueryDevicesEXT);
        LdEGLFUNC(EGL, QueryDeviceStringEXT);
        LdEGLFUNC(EGL, GetPlatformDisplayEXT);
        LdEGLFUNC(EGL, QueryDisplayAttribEXT);
        LdEGLFUNC(EGL, QueryDeviceBinaryEXT);
        LdEGLFUNC(EGL, QueryDeviceAttribEXT);
        LdEGLFUNC(EGL, CreateImageKHR);
        LdEGLFUNC(EGL, DestroyImageKHR);
        LdEGLFUNC(EGL, GetNativeClientBufferANDROID);
#undef LdEGLFUNC
        if (const auto exts = QueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS); exts)
        {
            Extensions = common::str::Split(exts, ' ', false);
            std::string extNames;
            for (const auto ext : Extensions)
                extNames.append(ext).push_back('\n');
            oglLog().Debug(FmtString(u"Loaded EGL({}) with client exts:\n{}"), dllpath.u16string(), extNames);
            if (Extensions.Has("EGL_KHR_platform_android") || Extensions.Has("EGL_ANDROID_GLES_layers"sv))
                Type = EGLType::ANDROID;
            else if (Extensions.Has("EGL_ANGLE_platform_angle"sv))
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
        }
        if (QueryDevicesEXT)
        {
            std::vector<EGLDeviceEXT> devices;
            if (EGLint devCount = 0; EGL_TRUE == QueryDevicesEXT(0, nullptr, &devCount))
            {
                devices.resize(devCount);
                if (EGL_TRUE == QueryDevicesEXT(devCount, devices.data(), &devCount))
                {
                    Devices = std::unique_ptr<DeviceHolder[]>(new DeviceHolder[devCount]);
                    DeviceCount = devCount;

                    for (uint32_t i = 0; i < DeviceCount; ++i)
                    {
                        Devices[i].Cookie = devices[i];
                        InitDevice(Devices[i]);
                    }
                }
            }
        }
    }
    void InitDevice(DeviceHolder& device) const noexcept
    {
        std::optional<std::array<std::byte, 16>> uuid;
        std::u16string devPath16;
        if (QueryDeviceStringEXT)
        {
            if (const auto renderer = QueryDeviceStringEXT(device.Cookie, EGL_RENDERER_EXT); renderer)
            {
                device.Name = common::str::to_u16string(renderer);
            }
            if (const auto exts = QueryDeviceStringEXT(device.Cookie, EGL_EXTENSIONS); exts)
            {
                device.Extensions = common::str::Split(exts, ' ', false);
            }
            // device.Extension.Has("EGL_EXT_device_drm"sv)
            if (const auto devPath = QueryDeviceStringEXT(device.Cookie, EGL_DRM_DEVICE_FILE_EXT); devPath)
            {
                devPath16 = common::str::to_u16string(devPath);
            }
        }
        if (QueryDeviceBinaryEXT)
        {
            if (EGLint nameSize = 0; EGL_TRUE == QueryDeviceBinaryEXT(device.Cookie, EGL_DRIVER_NAME_EXT, 0, nullptr, &nameSize))
            {
                std::string tmp(nameSize, '\0');
                if (EGL_TRUE == QueryDeviceBinaryEXT(device.Cookie, EGL_DRIVER_NAME_EXT, nameSize, tmp.data(), &nameSize))
                {
                    device.Name = common::str::to_u16string(tmp.data());
                }
            }
            std::array<std::byte, 16> uuid_ = {};
            EGLint uuidSize = 0;
            if (EGL_TRUE == QueryDeviceBinaryEXT(device.Cookie, EGL_DEVICE_UUID_EXT, 16, uuid_.data(), &uuidSize) && uuidSize == 16)
                uuid.emplace(uuid_);
        }
        if (QueryDeviceAttribEXT)
        {
#if COMMON_OS_WIN
            if (!device.XCompDevice && device.Extensions.Has("EGL_ANGLE_device_d3d"sv))
            {
                if (const auto d3dLocator = dynamic_cast<const xcomp::D3DDeviceLocator*>(&XCompDevs); d3dLocator)
                {
                    EGLAttrib d3dHandle = 0;
                    if (EGL_TRUE != QueryDeviceAttribEXT(device.Cookie, EGL_D3D11_DEVICE_ANGLE, &d3dHandle))
                        QueryDeviceAttribEXT(device.Cookie, EGL_D3D9_DEVICE_ANGLE, &d3dHandle);
                    if (d3dHandle)
                        device.XCompDevice = d3dLocator->LocateExactDevice(reinterpret_cast<void*>(d3dHandle));
                }
            }
#endif
        }
        if (!device.XCompDevice)
            device.XCompDevice = XCompDevs.LocateExactDevice(nullptr, uuid ? &*uuid : nullptr, nullptr, devPath16);
        if (!device.XCompDevice && !device.Name.empty())
            device.XCompDevice = XCompDevs.TryLocateDevice(nullptr, nullptr, device.Name);
        else if (device.XCompDevice && device.Name.empty())
            device.Name = device.XCompDevice->Name;
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
    bool SupportCreateFromDevice() const noexcept final
    {
        return Extensions.Has("EGL_EXT_platform_device"sv) && CheckSupportPlatformDisplay();
    }

    std::string_view Name() const noexcept final { return LoaderName; }
    std::u16string Description() const noexcept final
    {
        std::u16string ret;
        switch (Type)
        {
        case EGLType::ANDROID: ret = u"Android"; break;
        case EGLType::ANGLE:   ret = u"Angle";   break;
        case EGLType::MESA:    ret = u"MESA";    break;
        default: break;
        }
        return ret;
    }
    std::u16string_view GetCurErrStr() const noexcept { return GetErrorString(GetError()); }

    /*void Init() override
    { }*/

    bool TryChooseConfig(FBConfig& ret, EGLDisplay display, const detail::AttribList<int32_t>& cfgAttrib) const
    {
        common::str::Formatter<char16_t> fmt16{};
        int cfgCount = 0;
        if (EGL_TRUE != ChooseConfig(display, cfgAttrib.Data(), &ret.Config, 1, &cfgCount))
            COMMON_THROWEX(common::BaseException, fmt16.FormatStatic(FmtString(u"Unable to choose EGL Config: [{}]"sv), GetCurErrStr()));
        if (cfgCount == 0)
        {
            ret.Config = nullptr;
            return false;
        }
        if (EGL_TRUE != GetConfigAttrib(display, ret.Config, EGL_NATIVE_VISUAL_ID, &ret.VisualId))
            COMMON_THROWEX(common::BaseException, fmt16.FormatStatic(FmtString(u"Unable to get EGL VisualId: [{}]"sv), GetCurErrStr()));
        return true;
    }

    std::shared_ptr<EGLLoader::EGLHost> CreateFromDisplay(EGLDisplay display, bool useOffscreen)
    {
        common::str::Formatter<char16_t> fmt16{};
        if (!display)
            COMMON_THROWEX(common::BaseException, fmt16.FormatStatic(FmtString(u"Unable to get EGL Display: [{}]"sv), GetCurErrStr()));
        auto host = std::make_shared<EGLHost>(*this, display, useOffscreen);
        detail::AttribList cfgAttrib(EGL_NONE);
        cfgAttrib.Set(EGL_SURFACE_TYPE, useOffscreen ? EGL_PBUFFER_BIT : EGL_WINDOW_BIT);
        cfgAttrib.Set(EGL_RED_SIZE,     8);
        cfgAttrib.Set(EGL_GREEN_SIZE,   8);
        cfgAttrib.Set(EGL_BLUE_SIZE,    8);
        cfgAttrib.Set(EGL_ALPHA_SIZE,   8);
        cfgAttrib.Set(EGL_STENCIL_SIZE, 8);
        cfgAttrib.Set(EGL_DEPTH_SIZE,   24);

        EGLint renderableGL = 0, renderableGLES = 0;
        if (host->SupportDesktop) renderableGL   |= EGL_OPENGL_BIT;
        if (host->SupportES)      renderableGLES |= EGL_OPENGL_ES2_BIT;
        if (host->Version >= 15)  renderableGLES |= EGL_OPENGL_ES3_BIT;
        Ensures(renderableGL || renderableGLES);
        if (renderableGL && renderableGLES) // try both support type
        {
            cfgAttrib.Set(EGL_RENDERABLE_TYPE, renderableGL | renderableGLES);
            TryChooseConfig(host->CommonConfig, display, cfgAttrib);
        }
        if (renderableGL)
        {
            cfgAttrib.Set(EGL_RENDERABLE_TYPE, renderableGL);
            TryChooseConfig(host->GLConfig, display, cfgAttrib);
        }
        if (renderableGLES)
        {
            cfgAttrib.Set(EGL_RENDERABLE_TYPE, renderableGLES);
            TryChooseConfig(host->GLESConfig, display, cfgAttrib);
        }
        if (renderableGL && renderableGLES && !host->CommonConfig)
        {
            if (host->GLConfig && host->GLESConfig)
            {
                if (host->GLConfig.VisualId != host->GLESConfig.VisualId)
                    oglLog().Warning(u"GL&GLES with different visaul id: [{}} vs [{}].\n", host->GLConfig.VisualId, host->GLESConfig.VisualId);
            }
            else
            {
                if (host->SupportDesktop && !host->GLConfig)
                {
                    host->SupportDesktop = false;
                    oglLog().Warning(u"Disable GL support due to no compatible config.\n");
                }
                if (host->SupportES && !host->GLESConfig)
                {
                    host->SupportES = false;
                    oglLog().Warning(u"Disable GLES support due to no compatible config.\n");
                }
            }
        }
        if (!host->CommonConfig && !host->GLConfig && !host->GLESConfig)
        {
            EGLint cfgTotal = 0;
            if (EGL_TRUE != GetConfigs(display, nullptr, 0, &cfgTotal))
                COMMON_THROWEX(common::BaseException, fmt16.FormatStatic(FmtString(u"Unable to get EGL Configs: [{}]"sv), GetCurErrStr()));
            std::u16string result;
            std::vector<EGLConfig> configs; 
            configs.resize(cfgTotal);
            GetConfigs(display, configs.data(), cfgTotal, &cfgTotal);
            cfgAttrib.Set(EGL_RENDERABLE_TYPE, renderableGLES);
            for (EGLint i = 0; i < cfgTotal; ++i)
            {
                const auto& cfg = configs[i];
                fmt16.FormatToStatic(result, FmtString(u"cfg[{}]: "sv), i);
                for (const auto [k, v] : cfgAttrib)
                {
                    EGLint v2 = 0;
                    if (EGL_TRUE == GetConfigAttrib(display, cfg, k, &v2))
                    {
                        if (k == EGL_RENDERABLE_TYPE)
                        {
                            fmt16.FormatToStatic(result, FmtString(u"[RdType]:[{:04X}|{:04X}]vs[{:04X}], "sv), renderableGL, renderableGLES, v2);
                        }
                        else
                        {
                            const auto match = (k == EGL_RENDERABLE_TYPE || k == EGL_SURFACE_TYPE) ? (v & v2) == v : v == v2;
                            if (!match)
                                fmt16.FormatToStatic(result, FmtString(u"[{:04X}]:[{:04X}]vs[{:04X}], "sv), k, v, v2);
                        }
                    }
                }
                result.append(u"\n");
            }
            oglLog().Debug(u"Totally [{}] configs avaliable:\n{}\n", cfgTotal, result);
            COMMON_THROWEX(common::BaseException, u"Unable to choose EGL Config"sv);
        }
        return host;
    }

    bool CheckSupportPlatformDisplay() const noexcept
    {
        if (GetPlatformDisplay || GetPlatformDisplayEXT)
            return true;
        oglLog().Warning(u"EGL Loader does not support GetPlatformDisplay.\n");
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

    std::shared_ptr<EGLLoader::EGLHost> CreateHost(NativeDisplay display, bool useOffscreen) final
    {
        EGLDisplay disp = GetDisplay((EGLNativeDisplayType)(display));
        if (!disp && useOffscreen)
            disp = GetDisplay(EGL_DEFAULT_DISPLAY);
        if (!disp && useOffscreen && CheckSupportPlatformDisplay() && Extensions.Has("EGL_MESA_platform_surfaceless"sv))
            disp = GetPlatformDisplayCombine(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, [&](auto&) { }); 
        return CreateFromDisplay(disp, useOffscreen);
    }
    std::shared_ptr<EGLLoader::EGLHost> CreateHostFromXcb(void* connection, std::optional<int32_t> screen, bool useOffscreen) final
    {
        if (CheckSupportPlatformDisplay())
        {
            if (Extensions.Has("EGL_EXT_platform_xcb"sv) || Extensions.Has("EGL_MESA_platform_xcb"sv))
            {
                EGLDisplay disp = GetPlatformDisplayCombine(EGL_PLATFORM_XCB_EXT, connection, [&](auto& list)
                    {
                        if (screen)
                            list.Set(EGL_PLATFORM_XCB_SCREEN_EXT, screen.value());
                    });
                return CreateFromDisplay(disp, useOffscreen);
            }
            oglLog().Warning(u"EGL Loader does not support create from xcb.\n");
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
            oglLog().Warning(u"EGL Loader does not support create from x11.\n");
        }
        return {};
    }
    std::shared_ptr<EGLLoader::EGLHost> CreateHostFromWayland(void* disp, bool useOffscreen) final
    {
        if (CheckSupportPlatformDisplay())
        {
            if (Extensions.Has("EGL_EXT_platform_wayland"sv) || Extensions.Has("EGL_KHR_platform_wayland"sv))
            {
                EGLDisplay display = GetPlatformDisplayCombine(EGL_PLATFORM_WAYLAND_EXT, disp, [&](auto&){});
                return CreateFromDisplay(display, useOffscreen);
            }
            oglLog().Warning(u"EGL Loader does not support create from wayland.\n");
        }
        return {};
    }
    std::shared_ptr<EGLLoader::EGLHost> CreateHostFromAngle(void* disp, AngleBackend backend, bool useOffscreen) final
    {
        if (!BackendMask(backend))
            oglLog().Warning(u"EGL Loader does not support create from angle.\n");
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
    std::shared_ptr<EGLLoader::EGLHost> CreateHostFromAndroid(bool useOffscreen) final
    {
        if (!Extensions.Has("EGL_KHR_platform_android"sv) || !CheckSupportPlatformDisplay())
        {
            oglLog().Warning(u"EGL Loader does not support create from android.\n");
            return {};
        }
        EGLDisplay display = GetPlatformDisplayCombine(EGL_PLATFORM_ANDROID_KHR, EGL_DEFAULT_DISPLAY, [&](auto&) { });
        return CreateFromDisplay(display, useOffscreen);
    }
    std::shared_ptr<EGLLoader::EGLHost> CreateHostFromDevice(const EGLLoader::DeviceHolder& device) final
    {
        if (!SupportCreateFromDevice())
        {
            oglLog().Warning(u"EGL Loader does not support create from device.\n");
            return {};
        }
        if (const auto ptr = &device; ptr < Devices.get() || ptr - Devices.get() >= DeviceCount)
        {
            oglLog().Warning(u"Device not belongs to this loader.\n");
            return {};
        }
        EGLDisplay display = GetPlatformDisplayCombine(EGL_PLATFORM_DEVICE_EXT, device.Cookie, [&](auto&) {});
        return CreateFromDisplay(display, true);
    }
    std::shared_ptr<EGLLoader::EGLHost> CreateHostSurfaceless() final
    {
        if (!CheckSupportPlatformDisplay())
        {
            oglLog().Warning(u"EGL Loader does not support create from platform.\n");
            return {};
        }
        if (Extensions.Has("EGL_MESA_platform_surfaceless"sv))
        {
            EGLDisplay display = GetPlatformDisplayCombine(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, [&](auto&) {});
            return CreateFromDisplay(display, true);
        }
        if (Extensions.Has("EGL_KHR_platform_android"sv))
        {
            return CreateHostFromAndroid(true);
        }
        return {};
    }

    static std::vector<const common::DynLib*>& GetLoadedDlls() noexcept
    {
        static std::vector<const common::DynLib*> container;
        return container;
    }

    static inline const auto Dummy = []() 
        {
            std::vector<common::fs::path> dllpaths;
#if COMMON_OS_WIN
            dllpaths.emplace_back("libEGL.dll");
#elif COMMON_OS_DARWIN
            dllpaths.emplace_back("libEGL.dylib");
#else
            dllpaths.emplace_back("libEGL.so");
            dllpaths.emplace_back("libEGL.so.1");
#endif
#if COMMON_OS_ANDROID
#   if COMMON_OSBIT == 64
            dllpaths.emplace_back("/system/lib64/libEGL.so");
#   else
            dllpaths.emplace_back("/system/lib/libEGL.so");
#   endif
#endif
            const auto envstrs = common::GetEnvVar("OGLU_EGL_PATH");
            const auto envpaths = common::str::to_u16string(envstrs, common::str::DetectEncoding(envstrs));
            for (const common::fs::path p : common::str::SplitStream(envpaths, u';', false))
            {
                std::error_code ec;
                if (p.is_absolute() && common::fs::exists(p, ec))
                {
                    dllpaths.emplace_back(p);
                }
            }
            for (const auto& p : dllpaths)
            {
                detail::RegisterLoader(EGLLoader_::LoaderName, [=]() -> std::unique_ptr<oglLoader> 
                    {
                        if (const auto dll = common::DynLib::FindLoaded(p); dll)
                        {
                            for (const auto target : GetLoadedDlls())
                            {
                                if (*target == dll) // already loaded
                                    return {};
                            }
                        }
                        return std::make_unique<EGLLoader_>(p);
                    });
            }
            return true;
        }();
};


oglEGLImage_::~oglEGLImage_()
{
    static_cast<const EGLLoader_::EGLHost*>(Host.get())->DestroyImage(Image);
}


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
