#include "oglPch.h"
#include "oglUtil.h"
#include "SystemCommon/DynamicLibrary.h"


#undef APIENTRY
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include "GL/glxext.h"
//fucking X11 defines some terrible macro
#undef Success
#undef Always
#undef None
#undef Bool
#undef Int
#pragma message("Compiling OpenGLUtil with glx-ext[" STRINGIZE(GLX_GLXEXT_VERSION) "]")


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


#define PREPARE_FUNC(func, name) using P##name = decltype(&func); static constexpr auto N##name = #func ""sv
#define PREPARE_FUNC2(type, func, name) using P##name = type; static constexpr auto N##name = #func ""sv

PREPARE_FUNC(XFree,                     FreeXObject);
PREPARE_FUNC(XSetErrorHandler,          SetErrorHandler);
PREPARE_FUNC(XGetErrorText,             GetErrorText);
PREPARE_FUNC(glXChooseFBConfig,         ChooseFBConfig);
PREPARE_FUNC(glXGetFBConfigAttrib,      GetFBConfigAttrib);
PREPARE_FUNC(glXGetVisualFromFBConfig,  GetVisualFromFBConfig);
PREPARE_FUNC(glXCreateWindow,           CreateWindow);
PREPARE_FUNC(glXCreateGLXPixmap,        CreateGLXPixmap);
PREPARE_FUNC(glXDestroyWindow,          DestroyWindow);
PREPARE_FUNC(glXDestroyGLXPixmap,       DestroyGLXPixmap);
PREPARE_FUNC(glXMakeCurrent,            MakeCurrent);
PREPARE_FUNC(glXDestroyContext,         DestroyContext);
PREPARE_FUNC(glXSwapBuffers,            SwapBuffers);
PREPARE_FUNC(glXQueryExtensionsString,  QueryExtensionsString);
PREPARE_FUNC(glXGetCurrentDisplay,      GetCurrentDisplay);
PREPARE_FUNC(glXGetCurrentDrawable,     GetCurrentDrawable);
PREPARE_FUNC(glXGetCurrentContext,      GetCurrentContext);
PREPARE_FUNC(glXGetProcAddress,         GetProcAddress);

#undef PREPARE_FUNC2
#undef PREPARE_FUNC

class GLXLoader_ : public GLXLoader
{
private:
    static int CreateContextXErrorHandler(Display* disp, XErrorEvent* evt)
    {
        std::string txtBuf;
        txtBuf.resize(1024, '\0');
        Singleton->GetErrorText(disp, evt->error_code, txtBuf.data(), 1024); // return value undocumented, cannot rely on that
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
        Display* DeviceContext;
        GLXFBConfig* FBConfigs = nullptr;
        PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = nullptr;
        int VisualId = 0;
        GLXDrawable Drawable = 0;
        bool IsWindowDrawable = false;
        GLXHost(GLXLoader_& loader, Display* dc, int32_t screen) noexcept : GLXLoader::GLXHost(loader), DeviceContext(dc)
        {
            glXCreateContextAttribsARB = loader.GetFunction<PFNGLXCREATECONTEXTATTRIBSARBPROC>("glXCreateContextAttribsARB");
            const char* exts = loader.QueryExtensionsString(DeviceContext, screen);
            Extensions = common::str::Split(exts, ' ', false);
            SupportSRGB = Extensions.Has("GLX_ARB_framebuffer_sRGB") || Extensions.Has("GLX_EXT_framebuffer_sRGB");
            SupportFlushControl = Extensions.Has("GLX_ARB_context_flush_control");
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
        const int& GetVisualId() const noexcept final { return VisualId; }
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
        void* GetDeviceContext() const noexcept final { return DeviceContext; }
        GLXContext CreateContextAttribs(GLXContext share_context, const int32_t* attrib_list)
        {
            const auto& loader = static_cast<const GLXLoader_&>(Loader);
            const auto oldHandler = loader.SetErrorHandler(&CreateContextXErrorHandler);
            const auto ret = glXCreateContextAttribsARB(DeviceContext, FBConfigs[0], share_context, 1/*True*/, attrib_list);
            loader.SetErrorHandler(oldHandler);
            return ret;
        }
    };
    common::DynLib LibX11, LibGLX;
#define PREPARE_FUNC(name) P##name name = nullptr
    PREPARE_FUNC(FreeXObject);
    PREPARE_FUNC(SetErrorHandler);
    PREPARE_FUNC(GetErrorText);
    PREPARE_FUNC(ChooseFBConfig);
    PREPARE_FUNC(GetFBConfigAttrib);
    PREPARE_FUNC(GetVisualFromFBConfig);
    PREPARE_FUNC(CreateWindow);
    PREPARE_FUNC(CreateGLXPixmap);
    PREPARE_FUNC(DestroyWindow);
    PREPARE_FUNC(DestroyGLXPixmap);
    PREPARE_FUNC(MakeCurrent);
    PREPARE_FUNC(DestroyContext);
    PREPARE_FUNC(SwapBuffers);
    PREPARE_FUNC(QueryExtensionsString);
    PREPARE_FUNC(GetCurrentDisplay);
    PREPARE_FUNC(GetCurrentDrawable);
    PREPARE_FUNC(GetCurrentContext);
    PREPARE_FUNC(GetProcAddress);
#undef PREPARE_FUNC
public:
    GLXLoader_() : LibX11("libX11.so"), LibGLX("libGLX.so")
    {
#define PREPARE_FUNC(lib, name) name = Lib##lib.GetFunction<P##name>(N##name)
        PREPARE_FUNC(X11, FreeXObject);
        PREPARE_FUNC(X11, SetErrorHandler);
        PREPARE_FUNC(X11, GetErrorText);
        PREPARE_FUNC(GLX, ChooseFBConfig);
        PREPARE_FUNC(GLX, GetFBConfigAttrib);
        PREPARE_FUNC(GLX, GetVisualFromFBConfig);
        PREPARE_FUNC(GLX, CreateWindow);
        PREPARE_FUNC(GLX, CreateGLXPixmap);
        PREPARE_FUNC(GLX, DestroyWindow);
        PREPARE_FUNC(GLX, DestroyGLXPixmap);
        PREPARE_FUNC(GLX, MakeCurrent);
        PREPARE_FUNC(GLX, DestroyContext);
        PREPARE_FUNC(GLX, SwapBuffers);
        PREPARE_FUNC(GLX, QueryExtensionsString);
        PREPARE_FUNC(GLX, GetCurrentDisplay);
        PREPARE_FUNC(GLX, GetCurrentDrawable);
        PREPARE_FUNC(GLX, GetCurrentContext);
        PREPARE_FUNC(GLX, GetProcAddress);
#undef PREPARE_FUNC
    }
    ~GLXLoader_() final {}
private:
    std::string_view Name() const noexcept final { return "GLX"sv; }

    /*void Init() override
    { }*/
    std::shared_ptr<GLXLoader::GLXHost> CreateHost(void* display, int32_t screen, bool useOffscreen) final
    {
        const auto disp = reinterpret_cast<Display*>(display);
        auto host = std::make_shared<GLXHost>(*this, disp, screen);
        int fbCount = 0;
        const auto configs = ChooseFBConfig(disp, screen, useOffscreen ? VisualAttrs2 : VisualAttrs, &fbCount);
        if (!configs || fbCount == 0)
            return {};
        if (0 != GetFBConfigAttrib(disp, configs[0], GLX_VISUAL_ID, &host->VisualId))
            return {};
        host->FBConfigs = configs;
        host->IsWindowDrawable = !useOffscreen;
        return host;
    }
    void InitDrawable(GLXLoader::GLXHost& host_, uint32_t drawable) const override
    {
        auto& host = static_cast<GLXHost&>(host_);
        const auto& display = host.DeviceContext;
        const auto& config  = host.FBConfigs[0];
        if (host.IsWindowDrawable)
        {
            const auto glxwindow = CreateWindow(display, config, drawable, nullptr);
            host.Drawable = glxwindow;
        }
        else
        {
            XVisualInfo* vi = GetVisualFromFBConfig(display, config);
            const auto glxpixmap = CreateGLXPixmap(display, vi, drawable);
            host.Drawable = glxpixmap;
        }
    }
    void* CreateContext(const GLHost& host_, const CreateInfo& cinfo, void* sharedCtx) noexcept final
    {
        auto host = std::static_pointer_cast<GLXHost>(host_);
        detail::AttribList attrib;
        attrib.Set(GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB);
        if (cinfo.DebugContext)
            attrib.Set(GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB);
        if (cinfo.Version != 0)
        {
            attrib.Set(GLX_CONTEXT_MAJOR_VERSION_ARB, static_cast<int32_t>(cinfo.Version / 10));
            attrib.Set(GLX_CONTEXT_MINOR_VERSION_ARB, static_cast<int32_t>(cinfo.Version % 10));
        }
        if (cinfo.FlushWhenSwapContext && false)
        {
            attrib.Set(GLX_CONTEXT_RELEASE_BEHAVIOR_ARB, GLX_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB);
        }
        return host->CreateContextAttribs(reinterpret_cast<GLXContext>(sharedCtx), attrib.Data());
    }
    void* GetFunction_(std::string_view name) const noexcept final
    {
        return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<const GLubyte*>(name.data())));
    }

    static inline const auto Singleton = oglLoader::RegisterLoader<GLXLoader_>();
};


}
