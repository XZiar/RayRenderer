#include "RenderCore/RenderCore.h"
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

std::unique_ptr<rayr::BasicTest> tester;
uint16_t curObj = 0;
FreeGLUTView window, wd2;
bool isAnimate = false;

void onResize(FreeGLUTView wd, int w, int h)
{
    tester->Resize(w, h);
}

//conver to radius
constexpr float muler = (float)(PI_float / 180);

void onKeyboard(FreeGLUTView wd, KeyEvent keyevent)
{
    switch (keyevent.SpecialKey())
    {
    case Key::Up:
        tester->Objects()[curObj]->Move(0, 0.1f, 0); break;
    case Key::Down:
        tester->Objects()[curObj]->Move(0, -0.1f, 0); break;
    case Key::Left:
        tester->Objects()[curObj]->Move(-0.1f, 0, 0); break;
    case Key::Right:
        tester->Objects()[curObj]->Move(0.1f, 0, 0); break;
    case Key::PageUp:
        tester->Objects()[curObj]->Move(0, 0, -0.1f); break;
    case Key::PageDown:
        tester->Objects()[curObj]->Move(0, 0, 0.1f); break;
    case Key::UNDEFINE:
        switch (keyevent.key)
        {
        case (uint8_t)Key::ESC:
            window.release();
            return;
        case 'a'://pan to left
            tester->cam.yaw(3 * muler); break;
        case 'd'://pan to right
            tester->cam.yaw(-3 * muler); break;
        case 'w'://pan to up
            tester->cam.pitch(3 * muler); break;
        case 's'://pan to down
            tester->cam.pitch(-3 * muler); break;
        case 'q'://pan to left
            tester->cam.roll(-3 * muler); break;
        case 'e'://pan to left
            tester->cam.roll(3 * muler); break;
        case 'A':
            tester->Objects()[curObj]->Rotate(0, 0, 3 * muler); break;
        case 'D':
            tester->Objects()[curObj]->Rotate(0, 0, -3 * muler); break;
        case 'W':
            tester->Objects()[curObj]->Rotate(3 * muler, 0, 0); break;
        case 'S':
            tester->Objects()[curObj]->Rotate(-3 * muler, 0, 0); break;
        case 'Q':
            tester->Objects()[curObj]->Rotate(0, 3 * muler, 0); break;
        case 'E':
            tester->Objects()[curObj]->Rotate(0, -3 * muler, 0); break;
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
                tester->mode = !tester->mode;
                window->setTitle(tester->mode ? "3D" : "2D");
            } break;
        case '+':
            curObj++;
            if (curObj >= (uint16_t)(tester->Objects().size()))
                curObj = 0;
            break;
        case '-':
            if (curObj == 0)
                curObj = (uint16_t)(tester->Objects().size());
            curObj--;
            break;
        }
        printf("U %.4f,%.4f,%.4f\nV %.4f,%.4f,%.4f\nN %.4f,%.4f,%.4f\n",
            tester->cam.u.x, tester->cam.u.y, tester->cam.u.z, tester->cam.v.x, tester->cam.v.y, tester->cam.v.z, tester->cam.n.x, tester->cam.n.y, tester->cam.n.z);
        break;
    }
    wd->refresh();
}

void onMouseEvent(FreeGLUTView wd, MouseEvent msevent)
{
    static const char *tpname[] = { "DOWN", "UP", "MOVE", "OVER", "WHEEL" };
    //printf("%5s mouse %d on window%d\n", tpname[(uint8_t)msevent.type], (uint8_t)msevent.btn, id);
    switch (msevent.type)
    {
    case MouseEventType::Moving:
        tester->cam.Move((msevent.dx * 10.f / tester->cam.width), (msevent.dy * 10.f / tester->cam.height), 0);
        break;
    case MouseEventType::Wheel:
        tester->cam.Move(0, 0, (float)msevent.dx);
        printf("camera at %5f,%5f,%5f\n", tester->cam.position.x, tester->cam.position.y, tester->cam.position.z);
        break;
    default:
        return;
    }
    wd->refresh();
}

void autoRotate()
{
    tester->Objects()[curObj]->Rotate(0, 3 * muler, 0);
}

bool onTimer(FreeGLUTView wd, uint32_t elapseMS)
{
    if (isAnimate)
    {
        autoRotate();
        wd->refresh();
    }
    return true;
}

void onDropFile(FreeGLUTView wd, u16string fname)
{
    const auto extName = fs::path(fname).extension().u16string();
    if (extName == u".obj")
    {
        tester->LoadModelAsync(*(u16string*)&fname, [&, wd](auto model)
        {
            wd->invoke([&, model]
            {
                if (tester->AddObject(model))
                {
                    curObj = (uint16_t)(tester->Objects().size() - 1);
                    tester->Objects()[curObj]->Rotate(-90 * muler, 0, 0);
                    tester->Objects()[curObj]->Move(-1, 0, 0);
                    return true;
                }
                return false;
            });
        });
    }
    else if (extName == u".glsl")
    {
        tester->LoadShaderAsync(*(u16string*)&fname, u"test0", [&, wd](auto shd) 
        {
            wd->invoke([&, shd] 
            {
                return tester->AddShader(shd);
            });
        });
    }
}

auto FindPath()
{
    fs::path shdpath(UTF16ER(__FILE__));
    return shdpath.parent_path().parent_path() / u"RenderCore";
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
    printf("miniBLAS intrin:%s\n", miniBLAS::miniBLAS_intrin());
    FreeGLUTViewInit();
    
    window.reset();
    tester.reset(new rayr::BasicTest(FindPath()));
    window->funDisp = [&](FreeGLUTView wd) { tester->Draw(); };
    window->funReshape = onResize;
    window->setTitle("3D");
    window->funKeyEvent = onKeyboard;
    window->funMouseEvent = onMouseEvent;
    window->setTimerCallback(onTimer, 20);
    window->funDropFile = onDropFile;
    window->funOnClose = [&](FreeGLUTView wd) { tester.release(); };
    if (false)
    {
        const auto light = Wrapper<b3d::PointLight>(std::in_place);
        light->color = b3d::Vec4(0.3, 1.0, 0.3, 1.0);
        tester->AddLight(light);
        tester->Cur3DProg()->State().SetSubroutine("lighter", "basic");
    }

    FreeGLUTViewRun();
}