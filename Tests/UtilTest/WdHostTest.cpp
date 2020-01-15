#include "TestRely.h"
#include "WindowHost/WindowHost.h"

using namespace common;
using namespace common::mlog;
using namespace xziar::gui;
using std::string;
using std::cin;

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

static void WDHost()
{
    const auto window = WindowHost_::CreateActive();
    window->Openning    += [](const auto&) { log().info(u"opened.\n"); };
    window->Closing     += [](const auto&, bool& should) { should = true; };
    window->Closed      += [](const auto&) { log().info(u"closed.\n"); };
    window->Displaying  += [idx = 0u, tm = common::SimpleTimer()](const auto&) mutable
    {
        if (idx++ % 300 == 0)
        {
            log().info(u"display.\n");
            if (idx > 1)
            {
                tm.Stop();
                log().info(u"Complete 5 seconds in [{}]s.\n", tm.ElapseMs() / 1000.f);
            }
            tm.Start();
        }
    };
    window->Resizing    += [](const auto&, int32_t width, int32_t height)
    {
        log().info(u"resize to [{:4} x {:4}].\n", width, height);
    };
    window->MouseEnter  += [](const auto&, const auto& evt)
    {
        log().info(u"Mouse enter at [{:4},{:4}]\n", evt.Pos.X, evt.Pos.Y);
    };
    window->MouseLeave  += [](const auto&, const auto& evt)
    {
        log().info(u"Mouse leave at [{:4},{:4}]\n", evt.Pos.X, evt.Pos.Y);
    };
    window->MouseButtonDown += [](const auto&, const auto& evt)
    {
        log().info(u"BtnDown: [{}], pressed: [{}]\n", BtnToStr(evt.ChangedButton), BtnToStr(evt.PressedButton));
    };
    window->MouseButtonUp   += [](const auto&, const auto& evt)
    {
        log().info(u"BtnUp  : [{}], pressed: [{}]\n", BtnToStr(evt.ChangedButton), BtnToStr(evt.PressedButton));
    };
    /*window->MouseMove   += [](const auto&, const auto& evt)
    {
        log().info(u"Mouse move to [{:4},{:4}], moved [{:4},{:4}]\n", evt.Pos.X, evt.Pos.Y, evt.Delta.X, evt.Delta.Y);
    };*/
    window->MouseDrag   += [](const auto&, const auto& evt)
    {
        log().info(u"Mouse drag from [{:4},{:4}] to [{:4},{:4}], just moved [{:4},{:4}]\n", 
            evt.BeginPos.X, evt.BeginPos.Y, evt.Pos.X, evt.Pos.Y, evt.Delta.X, evt.Delta.Y);
    };
    window->MouseWheel  += [](const auto&, const auto& evt)
    {
        log().info(u"Mouse wheel [{:6.3f}] at [{:4},{:4}].\n", evt.Delta, evt.Pos.X, evt.Pos.Y);
    }; 
    const auto keyHandler = [](const auto&, const auto& evt)
    {
        using namespace std::string_view_literals;
        const auto printKey = evt.ChangedKey.TryGetPrintable();
        const auto ascii = printKey.value_or(' ');
        const auto number = printKey.has_value() ? 0 : common::enum_cast(evt.ChangedKey.Key);
        log().info(u"Key changed [{}][{:4}] when [{}|{}|{}].\n", ascii, number,
            evt.HasCtrl()  ? "Ctrl"sv  : "    "sv,
            evt.HasShift() ? "Shift"sv : "     "sv,
            evt.HasAlt()   ? "Alt"sv   : "   "sv);
    };
    window->KeyDown += keyHandler;
    window->KeyUp   += keyHandler;
    window->Show();
    getchar();
    window->Close();
}

const static uint32_t ID = RegistTest("WDHost", &WDHost);
