#include "RenderCore/RenderCore.h"
//#include "RenderCore/BasicTest.h"
#include <cstdint>
#include <cstdio>
#include <memory>
#include "FreeGLUTView/FreeGLUTView.h"


using namespace glutview;
using namespace b3d;
using std::string;
using std::wstring;
using std::u16string;
using std::vector;
using namespace common;

//OGLU_OPTIMUS_ENABLE_NV

std::unique_ptr<rayr::RenderCore> tester;
uint16_t curObj = 0;
FreeGLUTView window, wd2;
bool isAnimate = false;
bool isPostproc = true;

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
        default:
            break;
        }
    }
    else
    {
        switch (keyevent.SpecialKey())
        {
        case Key::Up:
            tester->GetScene()->GetDrawables()[curObj]->Move(0, 0.1f, 0); break;
        case Key::Down:
            tester->GetScene()->GetDrawables()[curObj]->Move(0, -0.1f, 0); break;
        case Key::Left:
            tester->GetScene()->GetDrawables()[curObj]->Move(-0.1f, 0, 0); break;
        case Key::Right:
            tester->GetScene()->GetDrawables()[curObj]->Move(0.1f, 0, 0); break;
        case Key::PageUp:
            tester->GetScene()->GetDrawables()[curObj]->Move(0, 0, -0.1f); break;
        case Key::PageDown:
            tester->GetScene()->GetDrawables()[curObj]->Move(0, 0, 0.1f); break;
        case Key::UNDEFINE:
            switch (keyevent.key)
            {
            case (uint8_t)Key::ESC:
                window.release();
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
                tester->GetScene()->GetDrawables()[curObj]->Rotate(0, 0, 3 * muler); break;
            case 'D':
                tester->GetScene()->GetDrawables()[curObj]->Rotate(0, 0, -3 * muler); break;
            case 'W':
                tester->GetScene()->GetDrawables()[curObj]->Rotate(3 * muler, 0, 0); break;
            case 'S':
                tester->GetScene()->GetDrawables()[curObj]->Rotate(-3 * muler, 0, 0); break;
            case 'Q':
                tester->GetScene()->GetDrawables()[curObj]->Rotate(0, 3 * muler, 0); break;
            case 'E':
                tester->GetScene()->GetDrawables()[curObj]->Rotate(0, -3 * muler, 0); break;
            case 'x':
                wd2.reset(800, 600);
                break;
            case 'X':
                wd2.release(); break;
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
                curObj++;
                if (curObj >= (uint16_t)(tester->GetScene()->GetDrawables().size()))
                    curObj = 0;
                break;
            case '-':
                if (curObj == 0)
                    curObj = (uint16_t)(tester->GetScene()->GetDrawables().size());
                curObj--;
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
        tester->GetScene()->GetCamera()->Move((msevent.dx * 10.f / tester->GetScene()->GetCamera()->Width), (msevent.dy * 10.f / tester->GetScene()->GetCamera()->Height), 0);
        break;
    case MouseEventType::Wheel:
        tester->GetScene()->GetCamera()->Move(0, 0, (float)msevent.dx);
        printf("camera at %5f,%5f,%5f\n", tester->GetScene()->GetCamera()->Position.x, tester->GetScene()->GetCamera()->Position.y, tester->GetScene()->GetCamera()->Position.z);
        break;
    default:
        return;
    }
    wd->Refresh();
}

void autoRotate()
{
    tester->GetScene()->GetDrawables()[curObj]->Rotate(0, 3 * muler, 0);
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
                    curObj = (uint16_t)(tester->GetScene()->GetDrawables().size() - 1);
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

int wmain([[maybe_unused]]int argc, [[maybe_unused]]wchar_t *argv[])
{
    printf("miniBLAS intrin:%s\n", miniBLAS::miniBLAS_intrin());
    FreeGLUTViewInit();
    window.reset();
    tester.reset(new rayr::RenderCore());
    tester->TestSceneInit();
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
        const auto light = Wrapper<rayr::PointLight>(std::in_place);
        light->color = b3d::Vec4(0.3, 1.0, 0.3, 1.0);
        tester->GetScene()->AddLight(light);
        //tester->Cur3DProg()->State().SetSubroutine("lighter", "basic");
    }

    FreeGLUTViewRun();
}