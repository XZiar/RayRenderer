#include "RenderCore/RenderCore.h"
#include "RenderCore/RenderPass.h"
#include "RenderCore/SceneManager.h"
#include "RenderCore/PostProcessor.h"
#include "RenderCore/FontTest.h"
#include "ImageUtil/ImageUtil.h"
#include "FreeGLUTView/FreeGLUTView.h"
#include "common/Linq2.hpp"
#include <cstdint>
#include <cstdio>
#include <memory>


using namespace glutview;
using namespace b3d;
using std::string;
using std::wstring;
using std::u16string;
using std::vector;
using namespace common;


//OGLU_OPTIMUS_ENABLE_NV

std::unique_ptr<rayr::RenderCore> tester;
std::shared_ptr<rayr::Drawable> CurObj;
uint16_t CurPipe = 0;
FreeGLUTView window, wd2;
bool isAnimate = false;
bool isPostproc = true;


static common::mlog::MiniLogger<false>& log()
{
    static common::mlog::MiniLogger<false> logger(u"GLUTTest", { common::mlog::GetConsoleBackend() });
    return logger;
}

static std::shared_ptr<rayr::Drawable> LocateDrawable(const bool isPrev)
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
}

void onResize(FreeGLUTView wd, int w, int h)
{
    tester->Resize(w, h);
}

//conver to radius
constexpr float muler = (float)(PI_float / 180);

void onKeyboard(FreeGLUTView wd, KeyEvent keyevent)
{
    if (keyevent.hasCtrl())
    {
        switch (keyevent.SpecialKey())
        {
        case Key::F1:
            if (keyevent.hasShift())
                tester->DeSerialize(fs::temp_directory_path() / L"RayRenderer" / "testxzrp.dat");
            else
                tester->Serialize(fs::temp_directory_path() / L"RayRenderer" / "testxzrp.dat");
            break;
        case Key::F2:
            {
                CurPipe++;
                const auto& pps = tester->GetPipeLines();
                CurPipe = static_cast<uint16_t>(CurPipe % pps.size());
                const auto pipe = common::linq::FromIterable(pps)
                    .Skip(CurPipe).TryGetFirst().value();
                tester->ChangePipeLine(pipe);
            }
        case Key::F3:
            {
                const auto sceen = tester->Screenshot();
                xziar::img::WriteImage(sceen, fs::temp_directory_path() / L"RayRenderer" / "sceen.png");
                return;
            }
        default:
            break;
        }
    }
    else
    {
        switch (keyevent.SpecialKey())
        {
        case Key::Up:
            CurObj->Move(0, 0.1f, 0); break;
        case Key::Down:
            CurObj->Move(0, -0.1f, 0); break;
        case Key::Left:
            CurObj->Move(-0.1f, 0, 0); break;
        case Key::Right:
            CurObj->Move(0.1f, 0, 0); break;
        case Key::PageUp:
            CurObj->Move(0, 0, -0.1f); break;
        case Key::PageDown:
            CurObj->Move(0, 0, 0.1f); break;
        case Key::UNDEFINE:
            switch (keyevent.key)
            {
            case (uint8_t)Key::ESC:
                window.reset();
                return;
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
                wd2 = std::make_shared<glutview::detail::_FreeGLUTView>(800, 600);
                break;
            case 'X':
                wd2.reset(); break;
            case 13:
                if (keyevent.hasShift())
                    isAnimate = !isAnimate;
                else
                {
                    isPostproc = !isPostproc;
                    tester->GetPostProc()->SetEnable(isPostproc);
                    window->SetTitle(isPostproc ? "3D-PostProc" : "3D-Direct");
                } break;
            case '+':
                CurObj = LocateDrawable(false);
                break;
            case '-':
                CurObj = LocateDrawable(true);
                break;
            }
            //printf("U %.4f,%.4f,%.4f\nV %.4f,%.4f,%.4f\nN %.4f,%.4f,%.4f\n", tester->GetScene()->GetCamera()->Right().x, tester->GetScene()->GetCamera()->Right().y, tester->GetScene()->GetCamera()->Right().z,
            //    tester->GetScene()->GetCamera()->Up().x, tester->GetScene()->GetCamera()->Up().y, tester->GetScene()->GetCamera()->Up().z, tester->GetScene()->GetCamera()->Toward().x, tester->GetScene()->GetCamera()->Toward().y, tester->GetScene()->GetCamera()->Toward().z);
            break;
        }
    }
    wd->Refresh();
}

void onMouseEvent(FreeGLUTView wd, MouseEvent msevent)
{
    static const char *tpname[] = { "DOWN", "UP", "MOVE", "OVER", "WHEEL" };
    //printf("%5s mouse %d on window%d\n", tpname[(uint8_t)msevent.type], (uint8_t)msevent.btn, id);
    switch (msevent.type)
    {
    case MouseEventType::Moving:
        {
            const auto[w, h] = tester->GetWindowSize();
            tester->GetScene()->GetCamera()->Move((msevent.dx * 10.f / w), (msevent.dy * 10.f / h), 0);
        } break;
    case MouseEventType::Wheel:
        tester->GetScene()->GetCamera()->Move(0, 0, (float)msevent.dx);
        // printf("camera at %5f,%5f,%5f\n", tester->GetScene()->GetCamera()->Position.x, tester->GetScene()->GetCamera()->Position.y, tester->GetScene()->GetCamera()->Position.z);
        break;
    default:
        return;
    }
    wd->Refresh();
}

void autoRotate()
{
    CurObj->Rotate(0, 3 * muler, 0);
}

bool onTimer(FreeGLUTView wd, uint32_t)
{
    if (isAnimate)
    {
        autoRotate();
        wd->Refresh();
    }
    return true;
}

void onDropFile(FreeGLUTView wd, u16string fname)
{
    const auto extName = fs::path(fname).extension().u16string();
    if (extName == u".obj")
    {
        tester->LoadModelAsync(fname, [&, wd](auto model)
        {
            wd->Invoke([&, model](const FreeGLUTView& wd)
            {
                if (tester->GetScene()->AddObject(model))
                {
                    CurObj = model;
                    model->Rotate(-90 * muler, 0, 0);
                    model->Move(-1, 0, 0);
                    for (const auto& shd : tester->GetRenderPasses())
                        shd->RegistDrawable(model);
                    wd->Refresh();
                }
            });
        });
    }
    else if (extName == u".glsl")
    {
        tester->LoadShaderAsync(fname, u"test0", [&, wd](auto shd) 
        {
            wd->Invoke([&, shd](const FreeGLUTView& wd)
            {
                tester->AddShader(shd);
                wd->Refresh();
            });
        });
    }
}

auto FindPath()
{
    fs::path shdpath(UTF16ER(__FILE__));
    return shdpath.parent_path().parent_path() / u"RenderCore";
}


void PrintException(const common::BaseException& be)
{
    log().error(FMT_STRING(u"Error when performing test:\n{}\n"), be.message);
    fmt::basic_memory_buffer<char16_t> buf;
    for (const auto& stack : be.Stack())
        fmt::format_to(buf, FMT_STRING(u"{}:[{}]\t{}\n"), stack.File, stack.Line, stack.Func);
    log().error(FMT_STRING(u"stack trace:\n{}\n"), std::u16string_view(buf.data(), buf.size()));

    if (const auto inEx = std::dynamic_pointer_cast<common::BaseException>(be.NestedException()); inEx)
        PrintException(*inEx);
}

#if COMMON_OS_UNIX
#   include <X11/Xlib.h>
#endif

int main([[maybe_unused]]int argc, [[maybe_unused]]char *argv[]) try
{
    oglu::oglUtil::InitGLEnvironment();
#if COMMON_OS_UNIX
    XInitThreads();
    getchar(); // hack for linux's attach
#endif
    log().info("miniBLAS intrin:[{}]\n", miniBLAS::miniBLAS_intrin());
    FreeGLUTViewInit();
    window = std::make_shared<glutview::detail::_FreeGLUTView>();
    tester.reset(new rayr::RenderCore());
    tester->TestSceneInit();
    CurObj = tester->GetScene()->GetDrawables().begin()->second;
    window->funDisp = [&](FreeGLUTView wd) { tester->Draw(); };
    window->funReshape = onResize;
    window->SetTitle("3D");
    window->funKeyEvent = onKeyboard;
    window->funMouseEvent = onMouseEvent;
    window->SetTimerCallback(onTimer, 20);
    window->funDropFile = onDropFile;
    window->funOnClose = [&](FreeGLUTView wd) { isAnimate = false; tester.release(); };
    //if (false)
    {
        const auto light = std::make_shared<rayr::PointLight>();
        light->Color = b3d::Vec4(0.3, 1.0, 0.3, 1.0);
        tester->GetScene()->AddLight(light);
        //tester->Cur3DProg()->State().SetSubroutine("lighter", "basic");
    }

    const auto ftest = common::linq::FromIterable(tester->GetRenderPasses())
        .Select([](const auto& pipe) { return std::dynamic_pointer_cast<rayr::FontTester>(pipe); })
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
    FreeGLUTViewRun();
    return 0;
}
catch (const BaseException & be)
{
    PrintException(be);
}