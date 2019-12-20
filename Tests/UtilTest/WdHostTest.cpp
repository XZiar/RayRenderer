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
    window->MouseMove   += [](const auto&, int32_t x, int32_t y, int32_t flags)
    {
        log().info(u"Mouse move to [{:4},{:4}]. flags[{}]\n", x, y, flags);
    };
    window->MouseWheel  += [](const auto&, int32_t x, int32_t y, float dz, int32_t flags)
    {
        log().info(u"Mouse wheel [{:6.3f}] at [{:4},{:4}]. flags[{}]\n", dz, x, y, flags);
    };
    window->Show();
    getchar();
    window->Close();
}

const static uint32_t ID = RegistTest("WDHost", &WDHost);
