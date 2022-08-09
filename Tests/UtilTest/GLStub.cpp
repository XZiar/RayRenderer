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


#if COMMON_OS_WIN
template<typename... Args>
static std::shared_ptr<GLHost> GetHostWGL(oglLoader& loader, HDC dc, Args&&...)
{
    return static_cast<WGLLoader&>(loader).CreateHost(dc);
}
#endif
#if COMMON_OS_UNIX && !COMMON_OS_DARWIN
template<typename... Args>
static std::shared_ptr<GLHost> GetHostGLX(oglLoader& loader, Display* display, int32_t screen, Args&&...)
{
    return static_cast<GLXLoader&>(loader).CreateHost(display, screen, true);
}
#endif
template<typename... Args>
static std::shared_ptr<GLHost> GetHostEGL(oglLoader& loader, [[maybe_unused]] void* dc, Args&&...)
{
    auto& eglLdr = static_cast<EGLLoader&>(loader);
    if (eglLdr.GetType() == EGLLoader::EGLType::ANDROID)
    {
        return eglLdr.CreateHostFromAndroid(true);
    }
    else if (eglLdr.GetType() == EGLLoader::EGLType::ANGLE)
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
            const auto beidx = SelectIdx(bes, u"Backend", [&](const auto& type)
                {
                    return EGLLoader::GetAngleBackendName(type);
                });
            return eglLdr.CreateHostFromAngle(dc, bes[beidx], true);
        }
    }
#if COMMON_OS_DARWIN || COMMON_OS_ANDROID
    return eglLdr.CreateHost(0, true);
#else
    return eglLdr.CreateHost(dc, true);
#endif
}
#if COMMON_OS_IOS
template<typename... Args>
static std::shared_ptr<GLHost> GetHostEAGL(oglLoader& loader, void*, Args&&...)
{
    return static_cast<EAGLLoader&>(loader).CreateHost(true);
}
#endif

template<typename... Args>
static std::shared_ptr<GLHost> GetHost(oglLoader& loader, const Args&... args)
{
#if COMMON_OS_WIN
    if (loader.Name() == "WGL") return GetHostWGL(loader, args...);
#endif
#if COMMON_OS_IOS
    if (loader.Name() == "EAGL") return GetHostEAGL(loader, args...);
#endif
#if COMMON_OS_UNIX && !COMMON_OS_DARWIN
    if (loader.Name() == "GLX") return GetHostGLX(loader, args...);
#endif
    if (loader.Name() == "EGL") return GetHostEGL(loader, args...);
    
    log().Error(u"Unknown loader\n");
    return {};
}

std::u16string CommonDevInfoStr(const xcomp::CommonDeviceInfo& dev)
{
    return fmt::format(FMT_STRING(u"[{}] VID[{:#010x}] DID[{:#010x}]"sv),
        dev.Name, dev.VendorId, dev.DeviceId);
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

#if COMMON_OS_WIN
    HWND tmpWND = CreateWindowW(
        L"Static", L"Fake Window",            // window class, title
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN,  // style
        0, 0,                               // position x, y
        1, 1,                               // width, height
        NULL, NULL,                         // parent window, menu
        nullptr, NULL);                     // instance, param
    HDC tmpDC = GetDC(tmpWND);
#elif COMMON_OS_DARWIN
    void* display = nullptr;
#else
    const char* const disp = getenv("DISPLAY");
    Display* display = XOpenDisplay(disp ? disp : ":0.0");
    if (!display)
    {
        log().Error(u"Failed to open display\n");
        COMMON_THROW(BaseException, u"Error");
    }
    const auto defScreen = DefaultScreen(display);
#endif

    while (true)
    {
        const auto ldridx = SelectIdx(loaders, u"loader", [&](const auto& loader)
            {
                return FMTSTR2(u"[{}] {}", loader->Name(), loader->Description());
            });

        auto& loader = *loaders[ldridx];
#if COMMON_OS_WIN
        const auto host = GetHost(loader, tmpDC);
#elif COMMON_OS_DARWIN
        const auto host = GetHost(loader, display);
#else
        const auto host = GetHost(loader, display, defScreen);
#endif
        if (!host)
        {
            log().Error(u"Failed to init [{}] Host\n", loader.Name());
            continue;
        }
        log().Success(u"Init GLHost[{}] version [{}.{}]\n", loader.Name(), host->GetVersion() / 10, host->GetVersion() % 10);
        if (const auto cmDev = host->GetCommonDevice())
        {
            log().Success(FmtString(u"Host is on common device: {}\n"sv), CommonDevInfoStr(*cmDev));
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

