#include "RenderCore/RenderCore.h"
#include "RenderCore/RenderPass.h"
#include "RenderCore/SceneManager.h"
#include "RenderCore/PostProcessor.h"
#include "RenderCore/FontTest.h"
#include "ImageUtil/ImageUtil.h"
#include "WindowHost/WindowHost.h"
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
    log().error(FMT_STRING(u"Error when performing test:\n{}\n"), be.Message);
    std::u16string str(u"stack trace:\n");
    for (const auto& stack : be.GetStacks())
        fmt::format_to(std::back_inserter(str), FMT_STRING(u"{}:[{}]\t{}\n"), stack.File, stack.Line, stack.Func);
    str.append(u"\n");
    log().error(str);

    if (be.InnerException)
        PrintException(*be.InnerException);
}


void RunDizzCore()
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

    const auto window = WindowHost_::CreatePassive(1280, 720, u"DizzTest"sv);
    WindowHost wd2;
    std::promise<void> openPms, closePms;
#if COMMON_OS_WIN
    const auto loader = static_cast<WGLLoader*>(oglLoader::GetLoader("WGL"));
#elif COMMON_OS_LINUX
    const auto loader = static_cast<GLXLoader*>(oglLoader::GetLoader("GLX"));
    using GLXHost = std::shared_ptr<GLXLoader::GLXHost>;
#endif
    window->Openning() += [&](const auto&) 
    {
        log().info(u"opened.\n"); 
#if COMMON_OS_WIN
        const auto& hdc = *window->GetWindowData<void*>("HDC");
        const auto host = loader->CreateHost(hdc);
#elif COMMON_OS_LINUX
        const auto& host = *window->GetWindowData<GLXHost>("glhost");
        const auto wd = *window->GetWindowData<uint32_t>("window");
        host->InitDrawable(wd);
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
            if (!fs::exists(basePath))
                basePath = u"D:\\ProgramsTemps\\RayRenderer";
            if (!fs::exists(basePath))
                basePath = u"C:\\ProgramsTemps\\RayRenderer";
            ftest.value()->SetFont(basePath / u"test.ttf");
        }
        openPms.set_value();
    };
    window->Closed() += [&](const auto&)
    {
        isAnimate = false; 
        tester.release();
#if COMMON_OS_LINUX
        
        if (const auto host = window->GetWindowData<GLXHost>("glhost"); host)
            host->~GLXHost();
#endif
        closePms.set_value();
    };
    window->Displaying() += [&]() 
    { 
        const auto ctx = oglContext_::CurrentContext();
        oglu::oglDefaultFrameBuffer_::Get()->ClearAll();
        tester->Draw();
        oglContext_::CurrentContext()->SwapBuffer();
    };
    window->Show([&](std::string_view name) -> const void* 
    {
        if (name == "background")
        {
            static constexpr bool NoNeed = false;
            return &NoNeed;
        }
#if COMMON_OS_LINUX
        else if (name == "visual")
        {
            const auto display = window->GetWindowData<void*>("display");
            const auto defscreen = window->GetWindowData<int>("defscreen");
            if (display && defscreen)
            {
                if (const auto host = loader->CreateHost(*display, *defscreen); host)
                {
                    alignas(GLXHost) std::array<std::byte, sizeof(host)> tmp = { std::byte(0) };
                    new (tmp.data())GLXHost(host);
                    window->SetWindowData("glhost", tmp);
                    return &host->GetVisualId();
                }
            }
        }
#endif
        return nullptr;
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
                wd2 = WindowHost_::CreatePassive(800, 600, u"Temp"sv);
                wd2->Show();
                break;
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
        auto fname = std::u16string(files[0]);
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
    const auto runner = WindowHost_::Init();
    Expects(runner);
    common::BasicPromise<void> pms;
    std::thread Thread([&]()
        {
            pms.GetPromiseResult()->Get();
            RunDizzCore();
        });
    runner.RunInplace(&pms);
    if (Thread.joinable())
        Thread.join();
}
catch (const BaseException & be)
{
    PrintException(*be.InnerInfo());
}