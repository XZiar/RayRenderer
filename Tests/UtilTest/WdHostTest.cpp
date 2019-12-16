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
    window->OnOpen    += [](const auto&) { log().info(u"opened.\n"); };
    window->OnClose   += [](const auto&) { log().info(u"closed.\n"); };
    bool first = true;
    window->OnDisplay += [&](const auto&) 
    {
        if (first)
            log().info(u"display.\n"); 
        first = false;
    };
    window->Open();
    getchar();
    window->Close();
}

const static uint32_t ID = RegistTest("WDHost", &WDHost);
