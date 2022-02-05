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
    std::atomic_uint16_t VersionDesktop = 0, VersionES = 0;
    bool SupportDesktop : 1;
    bool SupportES : 1;
    bool SupportSRGB : 1;
    bool SupportFlushControl : 1;
    GLHost(oglLoader& loader) noexcept : Loader(loader),
        SupportDesktop(false), SupportES(false), SupportSRGB(false), SupportFlushControl(false) {}
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
    struct EGLHost : public GLHost
    {
    protected:
        using GLHost::GLHost;
    public:
        virtual void InitSurface(uintptr_t surface) = 0;
        virtual const int& GetVisualId() const noexcept = 0;
    };
    template<typename T>
    std::enable_if_t<std::is_pointer_v<T>, std::shared_ptr<EGLHost>> CreateHost(const T& display, bool useOffscreen = false)
    {
        return CreateHost_(reinterpret_cast<uintptr_t>(display), useOffscreen);
    }
    template<typename T>
    std::enable_if_t<std::is_integral_v<T>, std::shared_ptr<EGLHost>> CreateHost(const T& display, bool useOffscreen = false)
    {
        return CreateHost_(static_cast<uintptr_t>(display), useOffscreen);
    }
    std::shared_ptr<EGLHost> CreateHost(bool useOffscreen = false)
    {
        return CreateHost_(0, useOffscreen);
    }
private:
    virtual std::shared_ptr<EGLHost> CreateHost_(uintptr_t display, bool useOffscreen) = 0;
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