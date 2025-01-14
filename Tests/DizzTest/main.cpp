#include "RenderCore/RenderCore.h"
#include "RenderCore/RenderPass.h"
#include "RenderCore/SceneManager.h"
#include "RenderCore/PostProcessor.h"
#include "RenderCore/FontTest.h"
#include "ImageUtil/ImageUtil.h"
#include "WindowHost/WindowHost.h"
#include "SystemCommon/Format.h"
#include "common/Linq2.hpp"
#include <cstdint>
#include <cstdio>
#include <memory>
#include <future>


using namespace std::string_view_literals;
using namespace xziar::gui;
using namespace common;
using namespace oglu;
using std::string;
using std::wstring;
using std::u16string;
using std::vector;


static common::mlog::MiniLogger<false>& log()
{
    static common::mlog::MiniLogger<false> logger(u"DizzTest", { common::mlog::GetConsoleBackend() });
    return logger;
}


//convert to radius
constexpr float muler = (float)(common::math::PI_float / 180);


auto FindPath()
{
    fs::path shdpath(UTF16ER(__FILE__));
    return shdpath.parent_path().parent_path() / u"RenderCore";
}


void PrintException(const common::ExceptionBasicInfo& be)
{
    log().Error(FmtString(u"Error when performing test:\n{}\n"), be.Message);
    std::u16string str(u"stack trace:\n");
    for (const auto& stack : be.GetStacks())
        common::str::Formatter<char16_t>{}.FormatToStatic(str, FmtString(u"{}:[{}]\t{}\n"), stack.File, stack.Line, stack.Func);
    str.append(u"\n");
    log().Error(str);

    if (be.InnerException)
        PrintException(*be.InnerException);
}


void RunDizzCore(WindowBackend& backend)
{
    std::unique_ptr<dizz::RenderCore> tester;
    std::shared_ptr<dizz::Drawable> CurObj;
    uint16_t CurPipe = 0;
    bool isAnimate = false;
    bool isPostproc = true;

    const auto LocateDrawable = [&](const bool isPrev) -> std::shared_ptr<dizz::Drawable>
    {
        const auto& drws = tester->GetScene()->GetDrawables();
        auto cur = drws.begin();
        while (cur != drws.end() && cur->second != CurObj)
            ++cur;
        if (cur != drws.end())
        {
            if (isPrev)
                return cur == drws.begin() ? (--drws.end())->second : (--cur)->second;
            ++cur;
            return cur == drws.end() ? drws.begin()->second : cur->second;
        }
        else
            return {};
    };

#if COMMON_OS_WIN
    const auto loader = static_cast<WGLLoader*>(oglLoader::GetLoader("WGL"));
    using WdType = Win32Backend::Win32WdHost;
#elif COMMON_OS_LINUX
    const auto loader = static_cast<GLXLoader*>(oglLoader::GetLoader("GLX"));
    using WdType = XCBBackend::XCBWdHost;
    const auto& xcbBackend = static_cast<XCBBackend&>(backend);
    const auto host = loader->CreateHost(xcbBackend.GetDisplay(), xcbBackend.GetDefaultScreen());
#endif
    xziar::gui::CreateInfo wdInfo
    {
        .Title = u"DizzTest",
        .Width = 1280u,
        .Height = 720u,
        .UseDefaultRenderer = false,
    };
    const auto window = std::dynamic_pointer_cast<WdType>(backend.Create(wdInfo));
    WindowHost wd2;
    std::promise<void> openPms, closePms;

    window->Openning() += [&](const auto&) 
    {
        log().Info(u"opened.\n"); 
#if COMMON_OS_WIN
        const auto host = loader->CreateHost(window->GetHDC());
#elif COMMON_OS_LINUX
        host->InitDrawable(window->GetWindow());
#endif
        tester.reset(new dizz::RenderCore(*host));
        tester->TestSceneInit();
        tester->Resize(1280, 720);
        CurObj = tester->GetScene()->GetDrawables().begin()->second;

        //if (false)
        {
            const auto light = std::make_shared<dizz::PointLight>();
            light->Color = mbase::Vec4(0.3f, 1.0f, 0.3f, 1.0f);
            tester->GetScene()->AddLight(light);
            //tester->Cur3DProg()->State().SetSubroutine("lighter", "basic");
        }

        const auto ftest = common::linq::FromIterable(tester->GetRenderPasses())
            .Select([](const auto& pipe) { return std::dynamic_pointer_cast<dizz::FontTester>(pipe); })
            .Where([](const auto& pipe) { return (bool)pipe; })
            .TryGetFirst();
        if (ftest.has_value())
        {
            fs::path basePath = u"C:\\Programs Temps\\RayRenderer";
            std::error_code ec;
            if (!fs::exists(basePath, ec))
                basePath = u"D:\\ProgramsTemps\\RayRenderer";
            if (!fs::exists(basePath, ec))
                basePath = u"C:\\ProgramsTemps\\RayRenderer";
            ftest.value()->SetFont(basePath / u"test.ttf");
        }
        openPms.set_value();
    };
    window->Closed() += [&](const auto&)
    {
        isAnimate = false; 
        tester.release();
        closePms.set_value();
    };
    window->Displaying() += [&]() 
    { 
        const auto ctx = oglContext_::CurrentContext();
        oglu::oglDefaultFrameBuffer_::Get()->ClearAll();
        tester->Draw();
        ctx->SwapBuffer();
    };
    window->Show([&](std::string_view name) -> std::any
    {
        if (name == "background")
            return false;
#if COMMON_OS_LINUX
        else if (name == "visual")
            return host->GetVisualId();
#endif
        return {};
    });
    openPms.get_future().get();

    window->Resizing() += [&](const auto&, int32_t width, int32_t height)
    {
        window->InvokeUI([=, &tester](auto&)
        {
            tester->Resize(width, height);
        });
    };
    window->MouseDrag() += [&](const auto&, const auto& evt)
    {
        const auto [w, h] = tester->GetWindowSize();
        tester->GetScene()->GetCamera()->Move((evt.Delta.X * 10.f / w), (evt.Delta.Y * 10.f / h), 0);
        window->Invalidate();
    };
    window->MouseScroll() += [&](const auto&, const auto& evt)
    {
        tester->GetScene()->GetCamera()->Move(0, 0, evt.Vertical);
        window->Invalidate();
    };
    window->KeyDown() += [&](const auto&, const auto& evt)
    {
        if (evt.HasCtrl())
        {
            switch (evt.ChangedKey.Key)
            {
            case event::CommonKeys::F1:
                if (evt.HasShift())
                    tester->DeSerialize(fs::temp_directory_path() / L"RayRenderer" / "testxzrp.dat");
                else
                    tester->Serialize(fs::temp_directory_path() / L"RayRenderer" / "testxzrp.dat");
                break;
            case event::CommonKeys::F2:
                {
                    CurPipe++;
                    const auto& pps = tester->GetPipeLines();
                    CurPipe = static_cast<uint16_t>(CurPipe % pps.size());
                    const auto pipe = common::linq::FromIterable(pps)
                        .Skip(CurPipe).TryGetFirst().value();
                    tester->ChangePipeLine(pipe);
                } break;
            case event::CommonKeys::F3:
                {
                    const auto sceen = tester->Screenshot();
                    xziar::img::WriteImage(sceen, fs::temp_directory_path() / L"RayRenderer" / "sceen.png");
                } return;
            default:
                break;
            }
        }
        else
        {
            switch (common::enum_cast(evt.ChangedKey.Key))
            {
            case common::enum_cast(event::CommonKeys::Up):
                CurObj->Move(0, 0.1f, 0); break;
            case common::enum_cast(event::CommonKeys::Down):
                CurObj->Move(0, -0.1f, 0); break;
            case common::enum_cast(event::CommonKeys::Left):
                CurObj->Move(-0.1f, 0, 0); break;
            case common::enum_cast(event::CommonKeys::Right):
                CurObj->Move(0.1f, 0, 0); break;
            case common::enum_cast(event::CommonKeys::PageUp):
                CurObj->Move(0, 0, -0.1f); break;
            case common::enum_cast(event::CommonKeys::PageDown):
                CurObj->Move(0, 0, 0.1f); break;
            case common::enum_cast(event::CommonKeys::Esc):
                window->Close(); return;
            case common::enum_cast(event::CommonKeys::Enter):
                if (evt.HasShift())
                    isAnimate = !isAnimate;
                else
                {
                    isPostproc = !isPostproc;
                    tester->GetPostProc()->SetEnable(isPostproc);
                    window->SetTitle(isPostproc ? u"3D-PostProc" : u"3D-Direct");
                }
                break;
            case 'a'://pan to left
                tester->GetScene()->GetCamera()->Yaw(3 * muler); break;
            case 'd'://pan to right
                tester->GetScene()->GetCamera()->Yaw(-3 * muler); break;
            case 'w'://pan to up
                tester->GetScene()->GetCamera()->Pitch(3 * muler); break;
            case 's'://pan to down
                tester->GetScene()->GetCamera()->Pitch(-3 * muler); break;
            case 'q'://pan to left
                tester->GetScene()->GetCamera()->Roll(-3 * muler); break;
            case 'e'://pan to left
                tester->GetScene()->GetCamera()->Roll(3 * muler); break;
            case 'A':
                CurObj->Rotate(0, 0, 3 * muler); break;
            case 'D':
                CurObj->Rotate(0, 0, -3 * muler); break;
            case 'W':
                CurObj->Rotate(3 * muler, 0, 0); break;
            case 'S':
                CurObj->Rotate(-3 * muler, 0, 0); break;
            case 'Q':
                CurObj->Rotate(0, 3 * muler, 0); break;
            case 'E':
                CurObj->Rotate(0, -3 * muler, 0); break;
            case 'x':
            {
                xziar::gui::CreateInfo info;
                info.Width = 800, info.Height = 600, info.Title = u"Temp";
                wd2 = backend.Create(info);
                wd2->Show();
            } break;
            case 'X':
                wd2->Close(); break;
            case '+':
                CurObj = LocateDrawable(false); break;
            case '-':
                CurObj = LocateDrawable(true); break;
            }
        }
        window->Invalidate();
    };
    window->DropFile() += [&](auto&, const auto& files)
    {
        if (!files.size()) return;
        auto fname = std::u16string(*files.begin());
        const auto extName = fs::path(fname).extension().u16string();
        if (extName == u".obj")
        {
            tester->LoadModelAsync(fname, [&, window](auto model)
                {
                    window->InvokeUI([&, model](auto& wd)
                    {
                        if (tester->GetScene()->AddObject(model))
                        {
                            CurObj = model;
                            model->Rotate(-90 * muler, 0, 0);
                            model->Move(-1, 0, 0);
                            for (const auto& shd : tester->GetRenderPasses())
                                shd->RegistDrawable(model);
                            wd.Invalidate();
                        }
                    });
                });
        }
        else if (extName == u".glsl")
        {
            tester->LoadShaderAsync(fname, u"test0", [&, window](auto shd)
                {
                    window->InvokeUI([&, shd](auto& wd)
                    {
                        tester->AddShader(shd);
                        wd.Invalidate();
                    });
                });
        }
    };

    auto closeFut = closePms.get_future();
    std::thread thr([&]()
        {
            while (closeFut.wait_for(std::chrono::milliseconds(20)) == std::future_status::timeout)
            {
                if (isAnimate)
                {
                    CurObj->Rotate(0, 3 * muler, 0);
                    window->Invalidate();
                }
            }
        });
    closeFut.wait();
    if (thr.joinable())
        thr.join();
}

int main([[maybe_unused]]int argc, [[maybe_unused]]char *argv[]) try
{
    const auto backend = WindowBackend::GetBackends()[0];
    backend->Init();
    common::BasicPromise<void> pms;
    std::thread Thread([&]()
        {
            pms.GetPromiseResult()->Get();
            RunDizzCore(*backend);
        });
    backend->Run(true, &pms);
    if (Thread.joinable())
        Thread.join();
}
catch (const BaseException & be)
{
    PrintException(*be.InnerInfo());
}