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


static void WDHost()
{
    const auto window = WindowHost_::Create();
    window->Openning    += [](const auto&) { log().info(u"opened.\n"); };
    window->Closing     += [](const auto&, bool& should) { should = true; };
    window->Closed      += [](const auto&) { log().info(u"closed.\n"); };
    window->Displaying  += [first = true](const auto&) mutable
    {
        if (first)
            log().info(u"display.\n"); 
        first = false;
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
    window->MouseMove   += [](const auto&, const auto& evt)
    {
        log().info(u"Mouse move to [{:4},{:4}], moved [{:4},{:4}]\n", evt.Pos.X, evt.Pos.Y, evt.DeltaX, evt.DeltaY);
    };
    window->MouseWheel  += [](const auto&, const auto& evt)
    {
        log().info(u"Mouse wheel [{:6.3f}] at [{:4},{:4}].\n", evt.Delta, evt.Pos.X, evt.Pos.Y);
    };
    window->Show();
    getchar();
    window->Close();
}

const static uint32_t ID = RegistTest("WDHost", &WDHost);
