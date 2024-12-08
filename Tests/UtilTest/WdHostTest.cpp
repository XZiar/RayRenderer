#include "TestRely.h"
#include "WindowHost/WindowHost.h"
#include "ImageUtil/ImageUtil.h"
#include "SystemCommon/ConsoleEx.h"
#include "common/ResourceHelper.h"
#include "resource.h"
#include <thread>

using namespace common;
using namespace common::mlog;
using namespace xziar::gui;
using std::string;
using xziar::img::Image;
using xziar::img::ImageView;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"WDHost", { GetConsoleBackend() });
    return logger;
}


constexpr auto BtnToStr = [](xziar::gui::event::MouseButton btn)
{
    using namespace std::string_view_literals;
    using xziar::gui::event::MouseButton;
    switch (btn)
    {
    case MouseButton::Left: return "L"sv;
    case MouseButton::Middle: return "M"sv;
    case MouseButton::Right: return "R"sv;
    case MouseButton::Left | MouseButton::Right: return "LR"sv;
    default: return "X"sv;
    }
};

template<typename T>
static void SetBgImg(xziar::gui::WindowHost_& wd, const T& files) noexcept
{
    const auto cnt = std::size(files);
    for (size_t i = 0; i < cnt; ++i)
    {
        log().Verbose(u"--{}\n", files[i]);
    }
    for (size_t i = 0; i < cnt; ++i)
    {
        const auto fpath = files[i];
        try
        {
            const auto img = xziar::img::ReadImage(fpath, xziar::img::ImageDataType::BGRA);
            if (img.GetSize())
            {
                wd.SetBackground(img);
                return;
            }
        }
        catch (common::BaseException& be)
        {
            log().Error(u"Failed when try read image [{}]: {}\n", fpath, be.Message());
        }
        catch (...) {}
    }
};
static void OpenTestWindow(WindowBackend& backend)
{
    Image iconimg;
    {
        const auto icondat = common::ResourceHelper::GetData(L"BIN", IDR_IMG_TITLEICON);
        common::io::MemoryInputStream stream(icondat);
        iconimg = xziar::img::ReadImage(stream, u"PNG", xziar::img::ImageDataType::BGRA);
    }

    xziar::gui::CreateInfo wdInfo;
    wdInfo.Width = 1280, wdInfo.Height = 720, wdInfo.TargetFPS = 60, wdInfo.Title = u"WdHostTest";
    const auto window = backend.Create(wdInfo);
    window->Openning() += [&](const auto&) 
    { 
        log().Info(u"opened.\n");
        if (iconimg.GetSize())
            window->SetIcon(iconimg);
    };
    window->Closing() += [clickcnt = 0](const auto&, bool& should) mutable 
    {
        log().Info(u"attempt to close [{}].\n", clickcnt);
        should = clickcnt++ > 0;
    };
    window->Closed() += [](const auto&) { log().Info(u"closed.\n"); };
    window->Displaying() += [idx = 0u, tm = common::SimpleTimer()](const auto&) mutable
    {
        if (idx++ % 300 == 0)
        {
            if (idx > 1)
            {
                tm.Stop();
                log().Info(u"Complete 5 seconds in [{}]s.\n", tm.ElapseMs() / 1000.f);
            }
            tm.Start();
        }
    };
    window->Resizing() += [](const auto&, int32_t width, int32_t height)
    {
        log().Info(u"resize to [{:4} x {:4}].\n", width, height);
    };
    window->Minimizing() += [](const auto&)
    {
        log().Info(u"minimized.\n");
    };
    window->DPIChanging() += [](const auto&, float x, float y)
    {
        log().Info(u"DPI change to [{:4} x {:4}].\n", x, y);
    };
    window->DropFile() += [](auto& wd, const auto& evt)
    {
        log().Info(u"drop {} files at [{:4},{:4}]:\n", evt.Size(), evt.Pos.X, evt.Pos.Y);
        SetBgImg(wd, evt);
    };
    window->MouseEnter() += [](auto& wd, const auto& evt)
    {
        wd.SetWindowData("enter", evt.Pos);
        log().Info(u"Mouse enter at [{:4},{:4}]\n", evt.Pos.X, evt.Pos.Y);
    };
    window->MouseLeave() += [](const auto& wd, const auto& evt)
    {
        const auto enterPos = wd.template GetWindowData<event::Position>("enter");
        log().Info(u"Mouse leave at [{:4},{:4}], prev enter at [{:4},{:4}]\n", evt.Pos.X, evt.Pos.Y, 
            enterPos ? enterPos->X : -1, enterPos ? enterPos->Y : -1);
    };
    window->MouseButtonDown() += [](const auto&, const auto& evt)
    {
        log().Info(u"BtnDown: [{}], pressed: [{}]\n", BtnToStr(evt.ChangedButton), BtnToStr(evt.PressedButton));
    };
    window->MouseButtonUp() += [](const auto&, const auto& evt)
    {
        log().Info(u"BtnUp  : [{}], pressed: [{}]\n", BtnToStr(evt.ChangedButton), BtnToStr(evt.PressedButton));
    };
    window->MouseMove() += [](auto& wd, const auto& evt)
    {
        wd.SetTitle(FMTSTR2(u"Mouse move to [{:4},{:4}]", evt.Pos.X, evt.Pos.Y));
        //log().Info(u"Mouse move to [{:4},{:4}], moved [{:4},{:4}]\n", evt.Pos.X, evt.Pos.Y, evt.Delta.X, evt.Delta.Y);
    };
    window->MouseDrag() += [](const auto&, const auto& evt)
    {
        log().Info(u"Mouse drag from [{:4},{:4}] to [{:4},{:4}], just moved [{:4},{:4}]\n", 
            evt.BeginPos.X, evt.BeginPos.Y, evt.Pos.X, evt.Pos.Y, evt.Delta.X, evt.Delta.Y);
    };
    window->MouseScroll() += [](const auto&, const auto& evt)
    {
        log().Info(u"Mouse wheel [{:6.3f}, {:6.3f}] at [{:4},{:4}].\n", evt.Horizontal, evt.Vertical, evt.Pos.X, evt.Pos.Y);
    }; 
    const auto keyHandler = [](const auto&, const auto& evt)
    {
        using namespace std::string_view_literals;
        const auto printKey = evt.ChangedKey.TryGetPrintable();
        const auto ascii = printKey.value_or(' ');
        const auto number = printKey.has_value() ? 0 : common::enum_cast(evt.ChangedKey.Key);
        log().Info(u"Key changed [{}][{:4}] when [{}|{}|{}].\n", ascii, number,
            evt.HasCtrl()  ? "Ctrl"sv  : "    "sv,
            evt.HasShift() ? "Shift"sv : "     "sv,
            evt.HasAlt()   ? "Alt"sv   : "   "sv);
    };
    window->KeyDown() += keyHandler;
    window->KeyUp() += keyHandler;
    window->KeyDown() += [&](auto& wd, const auto& evt)
    {
        using namespace std::string_view_literals;
        const auto printKey = evt.ChangedKey.TryGetPrintable();
        if (evt.HasCtrl() && printKey == '1')
        {
            if (evt.HasShift())
            {
                wd.SetBackground({});
            }
            else
            {
                static const xziar::gui::FilePickerInfo fpInfo
                {
                    .Title = u"Open Image As Background"
                };
                const auto pms = backend.OpenFilePicker(fpInfo);
                pms->OnComplete([host = wd.GetSelf()](const auto& ret)
                {
                    try
                    {
                        SetBgImg(*host, ret->Get());
                    }
                    catch (common::BaseException& be)
                    {
                        log().Warning(u"Failed to pick image: {}.\n", be.Message());
                    }
                    catch (...) {}
                });
            }
        }
    };
    window->Show();
    getchar();
    window->Close();
}

static void WDHost()
{
    const auto backends = WindowBackend::GetBackends();
    if (backends.size() == 0)
    {
        log().Error(u"No WindowHost backend found!\n");
        return;
    }
    const auto whbidx = SelectIdx(backends, u"backend", [&](const auto& backend)
        {
            return FMTSTR2(u"[{}] {:2}|{:4}|{:2}|{:2}|{:2}", backend->Name(),
                backend->CheckFeature("OpenGL")     ? u"GL"   : u"",
                backend->CheckFeature("OpenGLES")   ? u"GLES" : u"",
                backend->CheckFeature("DirectX")    ? u"DX"   : u"",
                backend->CheckFeature("Vulkan")     ? u"VK"   : u"",
                backend->CheckFeature("NewThread")  ? u"NT"   : u"");
        });

    auto& backend = *backends[whbidx];
    backend.Init();

    bool runInplace = true;
    if (backend.CheckFeature("NewThread"))
    {
        GetConsole().Print(common::CommonColor::BrightWhite, u"Run WdHost on new thread? [y/n]\n");
        while (true)
        {
            const auto ch = common::console::ConsoleEx::ReadCharImmediate(false);
                 if (ch == 'y' || ch == 'Y') { runInplace = false; break; }
            else if (ch == 'n' || ch == 'N') { runInplace = true;  break; }
        }
    }

    if (runInplace)
    {
        common::BasicPromise<void> pms;
        std::thread Thread([&]() 
            {
                pms.GetPromiseResult()->Get();
                OpenTestWindow(backend);
            });
        backend.Run(false, &pms);
        if (Thread.joinable())
            Thread.join();
    }
    else
    {
        backend.Run(true);
        OpenTestWindow(backend);
    }
}

const static uint32_t ID = RegistTest("WDHost", &WDHost);
