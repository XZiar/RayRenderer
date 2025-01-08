#include "TestRely.h"
#include "XCompCommon.h"
#include "OpenGLUtil/OpenGLUtil.h"
#include "SystemCommon/ConsoleEx.h"
#include "common/StringLinq.hpp"
#include <iostream>
#include <algorithm>

#if COMMON_OS_WIN
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include <Windows.h>
#elif COMMON_OS_DARWIN
#else
#   include <X11/X.h>
#   include <X11/Xlib.h>
//fucking X11 defines some terrible macro
#   undef Always
#   undef None
#   undef Success
#endif


using namespace std::string_view_literals;
using namespace common;
using namespace common::mlog;
using namespace oglu;
using std::string;
using std::cin;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"OGLStub", { GetConsoleBackend() });
    return logger;
}

struct EGLNativeWindow
{
    virtual EGLLoader::NativeDisplay GetDisplay() const noexcept = 0;
};

using HostResult = std::pair<std::shared_ptr<GLHost>, std::shared_ptr<void>>;

#if COMMON_OS_WIN

struct GDIWindow final : public EGLNativeWindow
{
    static constexpr std::u16string_view Name = u"GDI"sv;
    HWND Window = nullptr;
    HDC Handle = nullptr;
    GDIWindow()
    {
        Window = CreateWindowW(
            L"Static", L"Fake Window",            // window class, title
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN,  // style
            0, 0,                               // position x, y
            1, 1,                               // width, height
            NULL, NULL,                         // parent window, menu
            nullptr, NULL);                     // instance, param
        Handle = GetDC(Window);
        // Handle = CreateCompatibleDC(nullptr);
    }
    ~GDIWindow()
    {
        ReleaseDC(Window, Handle);
        DestroyWindow(Window);
    }
    EGLLoader::NativeDisplay GetDisplay() const noexcept final { return Handle; }
};

static HostResult GetHostWGL(oglLoader& loader)
{
    auto window = std::make_shared<GDIWindow>();
    return { static_cast<WGLLoader&>(loader).CreateHost(window->Handle), window };
}

#endif
#if COMMON_OS_UNIX && !COMMON_OS_DARWIN

struct X11Window final : public EGLNativeWindow
{
    static constexpr std::u16string_view Name = u"X11"sv;
    Display* Handle = nullptr;
    int32_t Screen = 0;
    X11Window()
    {
        const auto dispName = common::GetEnvVar("DISPLAY");
        const auto dispStr = dispName.empty() ? ":0.0" : dispName.c_str();
        Handle = XOpenDisplay(dispStr);
        if (!Handle)
        {
            log().Error(u"Failed to open display [{}]\n", dispStr);
            COMMON_THROW(BaseException, u"Error");
        }
        Screen = DefaultScreen(Handle);
    }
    ~X11Window()
    {
    }
    EGLLoader::NativeDisplay GetDisplay() const noexcept final { return Handle; }
};

static HostResult GetHostGLX(oglLoader& loader)
{
    auto window = std::make_shared<X11Window>();
    return { static_cast<GLXLoader&>(loader).CreateHost(window->Handle, window->Screen, true), window };
}

#endif


template<typename T>
struct CreateWrap
{
    static std::shared_ptr<EGLNativeWindow> Create()
    {
        return std::make_shared<T>();
    }
};

template<typename... Ts>
static std::shared_ptr<EGLNativeWindow> CreateEGLWindow()
{
    constexpr auto TCount = sizeof...(Ts);
    static_assert(TCount > 0);
    std::array<std::shared_ptr<EGLNativeWindow>(*)(), TCount> funcs = { &CreateWrap<Ts>::Create... };
    uint32_t idx = 0;
    if (TCount > 1)
    {
        std::array<std::u16string_view, TCount> names = { Ts::Name... };
        idx = SelectIdx(names, u"Window", [&](const auto& name)
            {
                return name;
            });
    }
    return funcs[idx]();
}

template<typename... Ts>
static HostResult GetHostEGL(oglLoader& loader)
{
    std::vector<std::pair<std::u16string, std::function<HostResult(EGLLoader&)>>> methods;
    auto& eglLdr = static_cast<EGLLoader&>(loader);
    const auto devs = eglLdr.GetDeviceList();
    if (!devs.empty())
    {
        const auto& xcdevs = xcomp::ProbeDevice();
        auto& fmter = GetLogFmt();
        uint32_t idx = 0;
        for (const auto& dev : devs)
            fmter.FormatToStatic(fmter.Str, FmtString(u"{@<W}device[{}] [@{:1}]{}\n"sv),
                idx++, GetIdx36(xcdevs.GetDeviceIndex(dev.XCompDevice)),
                dev.Name);
        common::mlog::SyncConsoleBackend();
        PrintToConsole(fmter);
        if (eglLdr.SupportCreateFromDevice())
        {
            methods.emplace_back(u"Device", [devs = devs](EGLLoader& eglLdr) -> HostResult
                {
                    const auto didx = SelectIdx(devs, u"device", nullptr);
                    return { eglLdr.CreateHostFromDevice(devs[didx]), {} };
                });
        }
    }
    if (eglLdr.GetType() == EGLLoader::EGLType::ANDROID)
    {
        methods.emplace_back(u"Android", [](EGLLoader& eglLdr) -> HostResult
            {
                return { eglLdr.CreateHostFromAndroid(true), {} };
            });
    }
    if (eglLdr.GetType() == EGLLoader::EGLType::ANGLE)
    {
        std::vector<EGLLoader::AngleBackend> bes;
        constexpr EGLLoader::AngleBackend BEs[] =
        { 
            EGLLoader::AngleBackend::D3D9, EGLLoader::AngleBackend::D3D11, EGLLoader::AngleBackend::D3D11on12, 
            EGLLoader::AngleBackend::GL, EGLLoader::AngleBackend::GLES, 
            EGLLoader::AngleBackend::Vulkan, EGLLoader::AngleBackend::SwiftShader, 
            EGLLoader::AngleBackend::Metal
        };
        for (const auto be : BEs)
        {
            if (eglLdr.CheckSupport(be))
                bes.emplace_back(be);
        }
        if (!bes.empty())
        {
            methods.emplace_back(u"Angle", [abes = std::move(bes)](EGLLoader& eglLdr) -> HostResult
                {
                    const auto beidx = SelectIdx(abes, u"Backend", [&](const auto& type)
                        {
                            return EGLLoader::GetAngleBackendName(type);
                        });
                    auto window = CreateEGLWindow<Ts...>();
                    return { eglLdr.CreateHostFromAngle(window->GetDisplay(), abes[beidx], true), window };
                });
        }
    }
    methods.emplace_back(u"DefaultWindow", [](EGLLoader& eglLdr) -> HostResult
        {
            return { eglLdr.CreateHost(true), {} };
        });
#if !(COMMON_OS_DARWIN || COMMON_OS_ANDROID)
    methods.emplace_back(u"Default", [](EGLLoader& eglLdr) -> HostResult
        {
            auto window = CreateEGLWindow<Ts...>();
            return { eglLdr.CreateHost(window->GetDisplay(), true), window };
        });
#endif
    Expects(!methods.empty());
    const auto midx = SelectIdx(methods, u"Method", [](const auto& method)
        {
            return method.first;
        });
    return methods[midx].second(eglLdr);
}

#if COMMON_OS_IOS

static HostResult GetHostEAGL(oglLoader& loader)
{
    return { static_cast<EAGLLoader&>(loader).CreateHost(true), {} };
}

#endif

template<typename... Ts>
static HostResult GetHost(oglLoader& loader)
{
#if COMMON_OS_WIN
    if (loader.Name() == "WGL") return GetHostWGL(loader);
#endif
#if COMMON_OS_IOS
    if (loader.Name() == "EAGL") return GetHostEAGL(loader);
#endif
#if COMMON_OS_UNIX && !COMMON_OS_DARWIN
    if (loader.Name() == "GLX") return GetHostGLX(loader);
#endif
    if (loader.Name() == "EGL") return GetHostEGL<Ts...>(loader);
    log().Error(u"Unknown loader\n");
    return { {}, {} };
}

std::u16string CommonDevInfoStr(const xcomp::CommonDeviceInfo& dev)
{
    return FMTSTR2(u"[{}] VID[{:#010x}] DID[{:#010x}]"sv, dev.Name, dev.VendorId, dev.DeviceId);
}

static void OGLStub()
{
    PrintCommonDevice();
    const auto loaders = oglLoader::GetLoaders();
    if (loaders.size() == 0)
    {
        log().Error(u"No OpenGL loader found!\n");
        return;
    }

    while (true)
    {
        const auto ldridx = SelectIdx(loaders, u"loader", [&](const auto& loader)
            {
                return FMTSTR2(u"[{}] {}", loader->Name(), loader->Description());
            });

        auto& loader = *loaders[ldridx];
#if COMMON_OS_WIN
        using TW = GDIWindow;
#elif COMMON_OS_DARWIN
        using TW = NullWindow;
#else
        using TW = X11Window;
#endif
        [[maybe_unused]]  const auto [host, window] = GetHost<TW>(loader);
        if (!host)
        {
            log().Error(u"Failed to init [{}] Host\n", loader.Name());
            continue;
        }
        log().Success(u"Init GLHost[{}] version [{}.{}]\n", loader.Name(), host->GetVersion() / 10, host->GetVersion() % 10);
        {
            std::u16string devtxt;
            if (const auto eglHost = std::dynamic_pointer_cast<EGLLoader::EGLHost>(host); eglHost)
            {
                if (const auto dev = eglHost->GetDeviceInfo(); dev)
                {
                    const auto& xcdevs = xcomp::ProbeDevice();
                    APPEND_FMT(devtxt, u"Host is on device:[@{:1}]{} ({})\n"sv,
                        GetIdx36(xcdevs.GetDeviceIndex(dev->XCompDevice)), dev->Name, 
                        dev->XCompDevice ? CommonDevInfoStr(*dev->XCompDevice) : u"");
                    for (const auto ext : dev->Extensions)
                    {
                        APPEND_FMT(devtxt, u"--{}\n"sv, ext);
                    }
                }
            }
            if (const auto cmDev = host->GetCommonDevice(); devtxt.empty() && cmDev)
            {
                APPEND_FMT(devtxt, u"Host is on common device: {}\n"sv, CommonDevInfoStr(*cmDev));
            }
            if (!devtxt.empty())
                log().Success(devtxt);
        }

        CreateInfo cinfo;
        cinfo.PrintFuncLoadFail = cinfo.PrintFuncLoadSuccess = true;
        std::vector<GLType> glTypes;
        for (const auto type : std::array<GLType, 2>{ GLType::Desktop, GLType::ES })
        {
            if (host->CheckSupport(type))
                glTypes.emplace_back(type);
        }
        const auto gltidx = SelectIdx(glTypes, u"GLType", [&](const auto& type)
            {
                return type == GLType::Desktop ? u"[GL]" : u"[GLES]";
            });
        cinfo.Type = glTypes[gltidx];
        const auto ctx = host->CreateContext(cinfo);
        if (!ctx)
        {
            log().Error(u"Failed to init [{}] context from [{}] host\n", cinfo.Type == GLType::Desktop ? u"GL"sv : u"GLES"sv, loader.Name());
            continue;
        }

        ctx->UseContext();
        ctx->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        std::u16string infotxt;
        APPEND_FMT(infotxt, u"GL Context [{}] [{}]: [{}]\n"sv, ctx->Capability->VendorString, ctx->Capability->RendererString, ctx->Capability->VersionString);
        APPEND_FMT(infotxt, u"LUID: [{}]\n"sv, Hex2Str(ctx->GetLUID()));
        APPEND_FMT(infotxt, u"UUID: [{}]\n"sv, Hex2Str(ctx->GetUUID()));
        log().Info(infotxt);
        if (ctx->XCompDevice)
        {
            log().Success(FmtString(u"Match common device: {}\n"sv), CommonDevInfoStr(*ctx->XCompDevice));
        }
        log().Info(u"{}\n", ctx->Capability->GenerateSupportLog());
        while (true)
        {
            common::mlog::SyncConsoleBackend();
            string fpath = common::console::ConsoleEx::ReadLine("input opengl file:");
            if (fpath == "EXTENSION")
            {
                string exttxts("Platform Extensions:\n");
                for (const auto& ext : ctx->GetPlatformExtensions())
                    exttxts.append(ext).append("\n");
                exttxts.append("Extensions:\n");
                for (const auto& ext : ctx->GetExtensions())
                    exttxts.append(ext).append("\n");
                log().Verbose(u"{}\n", exttxts);
                continue;
            }
            else if (fpath == "BREAK")
                break;
            else if (fpath == "clear")
            {
                GetConsole().ClearConsole();
                continue;
            }
            bool exConfig = false;
            if (fpath.size() > 0 && fpath.back() == '#')
                fpath.pop_back(), exConfig = true;
            common::fs::path filepath = FindPath() / fpath;
            log().Debug(u"loading gl file [{}]\n", filepath.u16string());
            try
            {
                const auto shaderSrc = common::file::ReadAllText(filepath);
                ShaderConfig config;
                if (exConfig)
                {
                    string line;
                    while (cin >> line)
                    {
                        ClearReturn();
                        if (line.size() == 0) break;
                        const auto parts = common::str::Split(line, '=');
                        string key(parts[0].substr(1));
                        switch (line.front())
                        {
                        case '#':
                            if (parts.size() > 1)
                                config.Defines[key] = string(parts[1].cbegin(), parts.back().cend());
                            else
                                config.Defines[key] = std::monostate{};
                            continue;
                        case '@':
                            if (parts.size() == 2)
                                config.Routines.insert_or_assign(key, string(parts[1]));
                            continue;
                        }
                        break;
                    }
                }
                auto stub = oglProgram_::Create();
                stub.AddExtShaders(shaderSrc, config);
                log().Info(u"Try Draw Program\n");
                [[maybe_unused]] auto drawProg = stub.LinkDrawProgram(u"Draw Prog");
                log().Info(u"Try Compute Program\n");
                [[maybe_unused]] auto compProg = stub.LinkComputeProgram(u"Compute Prog");
            }
            catch (const BaseException& be)
            {
                PrintException(be, u"Error here");
            }
        }
    }
    getchar();
}

const static uint32_t ID = RegistTest("OGLStub", &OGLStub);

