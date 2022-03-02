#include "oglPch.h"
#include "oglUtil.h"

#if !__has_include(<GL/glx.h>)
#   pragma message("Missing GLX header, skip GLX Loader")
#else
#   undef APIENTRY
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


namespace oglu
{
using namespace std::string_view_literals;


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


DEFINE_FUNC(XFree,                     FreeXObject);
DEFINE_FUNC(XSetErrorHandler,          SetErrorHandler);
DEFINE_FUNC(XGetErrorText,             GetErrorText);
DEFINE_FUNC(glXQueryVersion,           QueryVersion);
DEFINE_FUNC(glXChooseFBConfig,         ChooseFBConfig);
DEFINE_FUNC(glXGetFBConfigAttrib,      GetFBConfigAttrib);
DEFINE_FUNC(glXGetVisualFromFBConfig,  GetVisualFromFBConfig);
DEFINE_FUNC(glXCreateWindow,           CreateWindow);
DEFINE_FUNC(glXCreateGLXPixmap,        CreateGLXPixmap);
DEFINE_FUNC(glXDestroyWindow,          DestroyWindow);
DEFINE_FUNC(glXDestroyGLXPixmap,       DestroyGLXPixmap);
DEFINE_FUNC(glXMakeCurrent,            MakeCurrent);
DEFINE_FUNC(glXDestroyContext,         DestroyContext);
DEFINE_FUNC(glXSwapBuffers,            SwapBuffers);
DEFINE_FUNC(glXQueryExtensionsString,  QueryExtensionsString);
DEFINE_FUNC(glXGetCurrentDisplay,      GetCurrentDisplay);
DEFINE_FUNC(glXGetCurrentDrawable,     GetCurrentDrawable);
DEFINE_FUNC(glXGetCurrentContext,      GetCurrentContext);
DEFINE_FUNC(glXGetProcAddress,         GetProcAddress);

class GLXLoader_;
thread_local const GLXLoader_* CurLoader = nullptr;

class GLXLoader_ final : public GLXLoader
{
private:
    static int CreateContextXErrorHandler(Display* disp, XErrorEvent* evt)
    {
        std::string txtBuf;
        txtBuf.resize(1024, '\0');
        CurLoader->GetErrorText(disp, evt->error_code, txtBuf.data(), 1024); // return value undocumented, cannot rely on that
        txtBuf.resize(std::char_traits<char>::length(txtBuf.data()));
             if (txtBuf == "GLXBadFBConfig") oglLog().warning(u"Failed for FBConfig or unsupported version\n");
        else if (txtBuf == "GLXBadContext")  oglLog().warning(u"Failed for invalid RenderContext\n");
        else if (txtBuf == "BadMatch")       oglLog().warning(u"Failed for different server\n");
        else oglLog().warning(u"Failed for X11 error with code[{}][{}]:\t{}\n", evt->error_code, evt->minor_code,
            common::str::to_u16string(txtBuf, common::str::Encoding::UTF8));
        return 0;
    }

    struct GLXHost final : public GLXLoader::GLXHost
    {
        friend GLXLoader_;
        //using GLHost::GetFunction;
        Display* DeviceContext;
        GLXFBConfig* FBConfigs = nullptr;
        PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = nullptr;
        PFNGLXQUERYRENDERERINTEGERMESAPROC glXQueryRendererIntegerMESA = nullptr;
        PFNGLXQUERYRENDERERSTRINGMESAPROC glXQueryRendererStringMESA = nullptr;
        uint32_t Version = 0;
        int VisualId = 0;
        GLXDrawable Drawable = 0;
        bool IsWindowDrawable = false;
        GLXHost(GLXLoader_& loader, Display* dc, int32_t screen) noexcept : GLXLoader::GLXHost(loader), DeviceContext(dc)
        {
            SupportDesktop = true;
            int verMajor = 0, verMinor = 0;
            loader.QueryVersion(dc, &verMajor, &verMinor);
            Version = verMajor * 10 + verMinor;
            glXCreateContextAttribsARB  = GetFunction<PFNGLXCREATECONTEXTATTRIBSARBPROC> ("glXCreateContextAttribsARB");
            glXQueryRendererIntegerMESA = GetFunction<PFNGLXQUERYRENDERERINTEGERMESAPROC>("glXQueryRendererIntegerMESA");
            glXQueryRendererStringMESA  = GetFunction<PFNGLXQUERYRENDERERSTRINGMESAPROC> ("glXQueryRendererStringMESA");
            const char* exts = loader.QueryExtensionsString(DeviceContext, screen);
            Extensions = common::str::Split(exts, ' ', false);
            SupportES = Extensions.Has("GLX_EXT_create_context_es2_profile");
            SupportSRGBFrameBuffer = Extensions.Has("GLX_ARB_framebuffer_sRGB") || Extensions.Has("GLX_EXT_framebuffer_sRGB");
            SupportFlushControl = Extensions.Has("GLX_ARB_context_flush_control");
            if (Extensions.Has("GLX_MESA_query_renderer") && glXQueryRendererIntegerMESA)
            {
                uint32_t mesaVer[3] = { 0 };
                glXQueryRendererIntegerMESA(DeviceContext, screen, 0, GLX_RENDERER_VERSION_MESA, mesaVer);
                glXQueryRendererIntegerMESA(DeviceContext, screen, 0, GLX_RENDERER_VENDOR_ID_MESA, &VendorId);
                glXQueryRendererIntegerMESA(DeviceContext, screen, 0, GLX_RENDERER_DEVICE_ID_MESA, &DeviceId);
                oglLog().verbose(u"Create host on MESA[{}.{}.{}], device VID[{:#010x}] DID[{:#010x}].\n", 
                    mesaVer[0], mesaVer[1], mesaVer[2], VendorId, DeviceId);
                CommonDev = xcomp::LocateDevice(nullptr, nullptr, nullptr, &VendorId, &DeviceId, {});
            }
        }
        ~GLXHost() final
        {
            auto& loader = static_cast<GLXLoader_&>(Loader);
            if (Drawable)
            {
                if (IsWindowDrawable)
                    loader.DestroyWindow(DeviceContext, Drawable);
                else
                    loader.DestroyGLXPixmap(DeviceContext, Drawable);
            }
            if (FBConfigs)
                loader.FreeXObject(FBConfigs);
        }
        void InitDrawable(uint32_t drawable) final
        {
            auto& loader = static_cast<GLXLoader_&>(Loader);
            const auto& config = FBConfigs[0];
            if (IsWindowDrawable)
            {
                const auto glxwindow = loader.CreateWindow(DeviceContext, config, drawable, nullptr);
                Drawable = glxwindow;
            }
            else
            {
                XVisualInfo* vi = loader.GetVisualFromFBConfig(DeviceContext, config);
                const auto glxpixmap = loader.CreateGLXPixmap(DeviceContext, vi, drawable);
                Drawable = glxpixmap;
            }
        }
        void* CreateContext_(const CreateInfo& cinfo, void* sharedCtx) noexcept final
        {
            detail::AttribList attrib;
            auto ctxProfile = 0;
            if (cinfo.Type == GLType::ES)
                ctxProfile |= GLX_CONTEXT_ES2_PROFILE_BIT_EXT;
            else
                ctxProfile |= GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
            attrib.Set(GLX_CONTEXT_PROFILE_MASK_ARB, ctxProfile);
            if (cinfo.DebugContext)
                attrib.Set(GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB);
            if (cinfo.Version != 0)
            {
                attrib.Set(GLX_CONTEXT_MAJOR_VERSION_ARB, static_cast<int32_t>(cinfo.Version / 10));
                attrib.Set(GLX_CONTEXT_MINOR_VERSION_ARB, static_cast<int32_t>(cinfo.Version % 10));
            }
            if (cinfo.FlushWhenSwapContext && SupportFlushControl)
            {
                attrib.Set(GLX_CONTEXT_RELEASE_BEHAVIOR_ARB, cinfo.FlushWhenSwapContext.value() ?
                    GLX_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB : GLX_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB);
            }
            return CreateContextAttribs(reinterpret_cast<GLXContext>(sharedCtx), attrib.Data());
        }
        bool MakeGLContextCurrent_(void* hRC) const final
        {
            return static_cast<GLXLoader_&>(Loader).MakeCurrent(DeviceContext, Drawable, (GLXContext)hRC);
        }
        void DeleteGLContext(void* hRC) const final
        {
            static_cast<GLXLoader_&>(Loader).DestroyContext(DeviceContext, (GLXContext)hRC);
        }
        void SwapBuffer() const final
        {
            static_cast<GLXLoader_&>(Loader).SwapBuffers(DeviceContext, Drawable);
        }
        void ReportFailure(std::u16string_view action) const final
        {
            oglLog().error(u"Failed to {} on Display[{}] {}[{}], error: {}\n"sv, action, (void*)DeviceContext,
                Drawable == 0 ? u"Drawable"sv : (IsWindowDrawable ? u"Window"sv : u"Pixmap"sv), Drawable,
                (int32_t)errno);
        }
        void TemporalInsideContext(void* hRC, const std::function<void(void* hRC)>& func) const final
        {
            auto& loader = static_cast<const GLXLoader_&>(Loader);
            const auto oldHdc = loader.GetCurrentDisplay();
            const auto oldDrw = loader.GetCurrentDrawable();
            const auto oldHrc = loader.GetCurrentContext();
            if (loader.MakeCurrent(DeviceContext, Drawable, (GLXContext)hRC))
            {
                func(hRC);
                loader.MakeCurrent(oldHdc, oldDrw, oldHrc);
            }
        }
        const int& GetVisualId() const noexcept final { return VisualId; }
        uint32_t GetVersion() const noexcept final { return Version; }
        void* GetDeviceContext() const noexcept final { return DeviceContext; }
        GLXContext CreateContextAttribs(GLXContext share_context, const int32_t* attrib_list)
        {
            const auto& loader = static_cast<const GLXLoader_&>(Loader);
            CurLoader = &loader;
            const auto oldHandler = loader.SetErrorHandler(&CreateContextXErrorHandler);
            const auto ret = glXCreateContextAttribsARB(DeviceContext, FBConfigs[0], share_context, 1/*True*/, attrib_list);
            loader.SetErrorHandler(oldHandler);
            CurLoader = nullptr;
            return ret;
        }
        void* LoadFunction(std::string_view name) const noexcept final
        {
            return reinterpret_cast<void*>(static_cast<GLXLoader_&>(Loader).GetProcAddress(reinterpret_cast<const GLubyte*>(name.data())));
        }
        void* GetBasicFunctions(size_t, std::string_view) const noexcept final
        {
            return nullptr;
        }
    };
    common::DynLib LibX11, LibGLX;
    DECLARE_FUNC(FreeXObject);
    DECLARE_FUNC(SetErrorHandler);
    DECLARE_FUNC(GetErrorText);
    DECLARE_FUNC(QueryVersion);
    DECLARE_FUNC(ChooseFBConfig);
    DECLARE_FUNC(GetFBConfigAttrib);
    DECLARE_FUNC(GetVisualFromFBConfig);
    DECLARE_FUNC(CreateWindow);
    DECLARE_FUNC(CreateGLXPixmap);
    DECLARE_FUNC(DestroyWindow);
    DECLARE_FUNC(DestroyGLXPixmap);
    DECLARE_FUNC(MakeCurrent);
    DECLARE_FUNC(DestroyContext);
    DECLARE_FUNC(SwapBuffers);
    DECLARE_FUNC(QueryExtensionsString);
    DECLARE_FUNC(GetCurrentDisplay);
    DECLARE_FUNC(GetCurrentDrawable);
    DECLARE_FUNC(GetCurrentContext);
    DECLARE_FUNC(GetProcAddress);
public:
    static constexpr std::string_view LoaderName = "GLX"sv;
    GLXLoader_() : LibX11("libX11.so"), LibGLX("libGLX.so")
    {
        LOAD_FUNC(X11, FreeXObject);
        LOAD_FUNC(X11, SetErrorHandler);
        LOAD_FUNC(X11, GetErrorText);
        LOAD_FUNC(GLX, QueryVersion);
        LOAD_FUNC(GLX, ChooseFBConfig);
        LOAD_FUNC(GLX, GetFBConfigAttrib);
        LOAD_FUNC(GLX, GetVisualFromFBConfig);
        LOAD_FUNC(GLX, CreateWindow);
        LOAD_FUNC(GLX, CreateGLXPixmap);
        LOAD_FUNC(GLX, DestroyWindow);
        LOAD_FUNC(GLX, DestroyGLXPixmap);
        LOAD_FUNC(GLX, MakeCurrent);
        LOAD_FUNC(GLX, DestroyContext);
        LOAD_FUNC(GLX, SwapBuffers);
        LOAD_FUNC(GLX, QueryExtensionsString);
        LOAD_FUNC(GLX, GetCurrentDisplay);
        LOAD_FUNC(GLX, GetCurrentDrawable);
        LOAD_FUNC(GLX, GetCurrentContext);
        LOAD_FUNC(GLX, GetProcAddress);
    }
    ~GLXLoader_() final {}
private:
    std::string_view Name() const noexcept final { return LoaderName; }
    std::u16string Description() const noexcept final
    {
        return {};
    }

    /*void Init() override
    { }*/
    std::shared_ptr<GLXLoader::GLXHost> CreateHost(void* display, int32_t screen, bool useOffscreen) final
    {
        const auto disp = reinterpret_cast<Display*>(display);
        auto host = std::make_shared<GLXHost>(*this, disp, screen);
        int fbCount = 0;
        host->FBConfigs = ChooseFBConfig(disp, screen, useOffscreen ? VisualAttrs2 : VisualAttrs, &fbCount);
        if (!host->FBConfigs || fbCount == 0)
            return {};
        if (0 != GetFBConfigAttrib(disp, host->FBConfigs[0], GLX_VISUAL_ID, &host->VisualId))
            return {};
        host->IsWindowDrawable = !useOffscreen;
        return host;
    }

    static inline const auto Dummy = detail::RegisterLoader<GLXLoader_>();
};


}

#endif