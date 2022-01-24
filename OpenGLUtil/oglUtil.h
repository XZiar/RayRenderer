#pragma once
#include "oglRely.h"

#if COMMON_OS_WIN
#define OGLU_OPTIMUS_ENABLE_NV extern "C" { _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001; }
#else
#define OGLU_OPTIMUS_ENABLE_NV 
#endif

namespace oglu
{

class oglLoader
{
    friend oglContext_;
    friend CtxFuncs;
    friend GLHost_;
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> Impl;
    //virtual void Init() = 0;
    [[nodiscard]] virtual void* CreateContext(const GLHost& host, const CreateInfo& cinfo, void* sharedCtx) noexcept = 0;
    [[nodiscard]] virtual void* GetFunction_(std::string_view name) const noexcept = 0;
    OGLUAPI [[nodiscard]] std::shared_ptr<oglContext_> CreateContext_(const GLHost& host, CreateInfo cinfo, const oglContext_* sharedCtx) noexcept;
    void FillCurrentBaseInfo(ContextBaseInfo& info) const;
    static void LogError(const common::BaseException& be) noexcept;
    static oglLoader* RegisterLoader(std::unique_ptr<oglLoader> loader) noexcept;
protected:
    oglLoader();
    template<typename T>
    [[nodiscard]] T GetFunction(std::string_view name) const noexcept
    {
        return reinterpret_cast<T>(GetFunction_(name));
    }
    template<typename T>
    static T* RegisterLoader() noexcept
    {
        static_assert(std::is_base_of_v<oglLoader, T>);
        try
        {
            return static_cast<T*>(RegisterLoader(std::make_unique<T>()));
        }
        catch (const common::BaseException& be)
        {
            LogError(be);
        }
        return nullptr;
    }
public:

    virtual ~oglLoader();
    [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
    //void InitEnvironment();
    [[nodiscard]] std::shared_ptr<oglContext_> CreateContext(const GLHost& host, const CreateInfo& cinfo) noexcept
    {
        return CreateContext_(host, cinfo, {});
    }

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
    struct WGLHost : public GLHost_
    {
    protected:
        using GLHost_::GLHost_;
    public:
    };
    virtual std::shared_ptr<WGLHost> CreateHost(void* hdc) = 0;
};

class GLXLoader : public oglLoader
{
public:
    struct GLXHost : public GLHost_
    {
    protected:
        using GLHost_::GLHost_;
    public:
        virtual const int& GetVisualId() const noexcept = 0;
    };
    virtual std::shared_ptr<GLXHost> CreateHost(void* display, int32_t screen, bool useOffscreen = false) = 0;
    virtual void InitDrawable(GLXHost& host, uint32_t drawable) const = 0;
};


class oglUtil
{
private:
public:
    OGLUAPI static void InJectRenderDoc(const common::fs::path& dllPath);
    OGLUAPI static std::optional<std::string_view> GetError();
    OGLUAPI static common::PromiseResult<void> SyncGL();
    OGLUAPI static common::PromiseResult<void> ForceSyncGL();
};


}