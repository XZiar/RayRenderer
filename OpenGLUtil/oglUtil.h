#pragma once
#include "oglRely.h"

#if COMMON_OS_WIN
#define OGLU_OPTIMUS_ENABLE_NV extern "C" { _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001; }
#else
#define OGLU_OPTIMUS_ENABLE_NV 
#endif

namespace oglu
{

struct CreateInfo
{
    uint32_t Version = 0;
    GLType Type = GLType::Desktop;
    bool PrintFuncLoadSuccess = false;
    bool PrintFuncLoadFail = false;
    bool DebugContext = true;
    std::optional<bool> FramebufferSRGB;
    std::optional<bool> FlushWhenSwapContext;
};


class GLHost : public std::enable_shared_from_this<GLHost>
{
    friend oglLoader;
    friend oglContext_;
    friend CtxFuncs;
private:
    [[nodiscard]] virtual void* LoadFunction(std::string_view name) const noexcept = 0;
    [[nodiscard]] virtual void* GetBasicFunctions(size_t idx, std::string_view name) const noexcept = 0;
    [[nodiscard]] virtual void* CreateContext_(const CreateInfo& cinfo, void* sharedCtx) noexcept = 0;
    [[nodiscard]] virtual bool MakeGLContextCurrent_(void* hRC) const = 0;
    virtual void DeleteGLContext(void* hRC) const = 0;
    virtual void SwapBuffer() const = 0;
    virtual void ReportFailure(std::u16string_view action) const = 0;
    virtual void TemporalInsideContext(void* hRC, const std::function<void(void* hRC)>& func) const = 0;
    [[nodiscard]] bool MakeGLContextCurrent(void* hRC) const;
    [[nodiscard]] std::optional<ContextBaseInfo> FillBaseInfo(void* hRC) const;
    [[nodiscard]] void* GetFunction(std::string_view name) const noexcept;
    OGLUAPI [[nodiscard]] std::shared_ptr<oglContext_> CreateContext(CreateInfo cinfo, const oglContext_* sharedCtx);
protected:
    oglLoader& Loader;
    common::container::FrozenDenseSet<std::string_view> Extensions;
    const xcomp::CommonDeviceInfo* CommonDev = nullptr;
    uint32_t VendorId = 0, DeviceId = 0;
    std::atomic_uint16_t VersionDesktop = 0, VersionES = 0;
    bool SupportDesktop : 1;
    bool SupportES : 1;
    bool SupportSRGBFrameBuffer : 1;
    bool SupportFlushControl : 1;
    GLHost(oglLoader& loader) noexcept : Loader(loader),
        SupportDesktop(false), SupportES(false), SupportSRGBFrameBuffer(false), SupportFlushControl(false) {}
    template<typename T>
    [[nodiscard]] T GetFunction(std::string_view name) const noexcept
    {
        return reinterpret_cast<T>(GetFunction(name));
    }
public:
    virtual ~GLHost() = 0;
    [[nodiscard]] virtual void* GetDeviceContext() const noexcept = 0;
    [[nodiscard]] virtual uint32_t GetVersion() const noexcept = 0;
    [[nodiscard]] constexpr const common::container::FrozenDenseSet<std::string_view>& GetExtensions() const noexcept { return Extensions; }
    [[nodiscard]] constexpr bool CheckSupport(GLType type) const noexcept
    {
        switch (type)
        {
        case GLType::Desktop: return SupportDesktop;
        case GLType::ES:      return SupportES;
        default:              return false;
        }
    }
    [[nodiscard]] constexpr const xcomp::CommonDeviceInfo* GetCommonDevice() const noexcept
    {
        return CommonDev;
    }
    [[nodiscard]] std::shared_ptr<oglContext_> CreateContext(const CreateInfo& cinfo)
    {
        return CreateContext(cinfo, nullptr);
    }
};

class oglLoader
{
    friend oglContext_;
    friend CtxFuncs;
    friend GLHost;
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> Impl;
    //virtual void Init() = 0;
protected:
    oglLoader();
public:

    virtual ~oglLoader();
    [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
    [[nodiscard]] virtual std::u16string Description() const noexcept = 0;
    //void InitEnvironment();

    [[nodiscard]] OGLUAPI static common::span<const std::unique_ptr<oglLoader>> GetLoaders() noexcept;
    template<typename T>
    [[nodiscard]] static T* GetLoader() noexcept
    {
        static_assert(std::is_base_of_v<oglLoader, T>);
        for (const auto& loader : GetLoaders())
        {
            if (const auto ld = dynamic_cast<T*>(loader.get()); ld)
                return ld;
        }
        return nullptr;
    }
    [[nodiscard]] static oglLoader* GetLoader(std::string_view name) noexcept
    {
        for (const auto& loader : GetLoaders())
        {
            if (loader->Name() == name)
                return loader.get();
        }
        return nullptr;
    }
};

class WGLLoader : public oglLoader
{
public:
    struct WGLHost : public GLHost
    {
    protected:
        using GLHost::GLHost;
    public:
    };
    virtual std::shared_ptr<WGLHost> CreateHost(void* hdc) = 0;
};

class GLXLoader : public oglLoader
{
public:
    struct GLXHost : public GLHost
    {
    protected:
        using GLHost::GLHost;
    public:
        virtual void InitDrawable(uint32_t drawable) = 0;
        virtual const int& GetVisualId() const noexcept = 0;
    };
    virtual std::shared_ptr<GLXHost> CreateHost(void* display, int32_t screen, bool useOffscreen = false) = 0;
};

class EGLLoader : public oglLoader
{
public:
    enum class EGLType : uint8_t { Unknown, ANDROID, MESA, ANGLE };
    enum class AngleBackend : uint8_t { Any, D3D9, D3D11, D3D11on12, GL, GLES, Vulkan, SwiftShader, Metal };
    [[nodiscard]] OGLUAPI static std::u16string_view GetAngleBackendName(AngleBackend backend) noexcept;
    struct EGLHost : public GLHost
    {
    protected:
        using GLHost::GLHost;
    public:
        virtual void InitSurface(uintptr_t surface) = 0;
        virtual const int& GetVisualId() const noexcept = 0;
    };
#if COMMON_OS_DARWIN
    using NativeDisplay = int;
#else
    using NativeDisplay = void*;
#endif
    [[nodiscard]] virtual EGLType GetType() const noexcept = 0;
    [[nodiscard]] virtual bool CheckSupport(AngleBackend backend) const noexcept = 0;
    [[nodiscard]] std::shared_ptr<EGLHost> CreateHost(bool useOffscreen = false)
    {
        return CreateHost(static_cast<NativeDisplay>(0), useOffscreen);
    }
    [[nodiscard]] virtual std::shared_ptr<EGLHost> CreateHost(NativeDisplay display, bool useOffscreen) = 0;
    [[nodiscard]] virtual std::shared_ptr<EGLHost> CreateHostFromXcb(void* connection, std::optional<int32_t> screen, bool useOffscreen) = 0;
    [[nodiscard]] virtual std::shared_ptr<EGLHost> CreateHostFromX11(void* display, std::optional<int32_t> screen, bool useOffscreen) = 0;
    [[nodiscard]] virtual std::shared_ptr<EGLHost> CreateHostFromAngle(void* display, AngleBackend backend, bool useOffscreen) = 0;
    [[nodiscard]] virtual std::shared_ptr<EGLHost> CreateHostFromAndroid(bool useOffscreen) = 0;
};

class EAGLLoader : public oglLoader
{
public:
    struct EAGLHost : public GLHost
    {
    protected:
        using GLHost::GLHost;
    public:
        virtual void BindDrawable(oglContext_& ctx, void* caeaglLayer) = 0;
    };
    [[nodiscard]] virtual std::shared_ptr<EAGLHost> CreateHost(bool useOffscreen) = 0;
 };


class oglUtil
{
private:
public:
    OGLUAPI static void InJectRenderDoc(const common::fs::path& dllPath);
    OGLUAPI static std::optional<std::string_view> GetError();
    OGLUAPI static common::PromiseResult<void> SyncGL();
};


}
