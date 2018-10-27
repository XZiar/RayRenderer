#include "TestRely.h"
#include "common/FileEx.hpp"
#include "common/TimeUtil.hpp"

using namespace common::mlog;
using namespace common;
using std::vector;

static MiniLogger<false>& log()
{
    static MiniLogger<false> log(u"LogTest", { GetConsoleBackend() });
    return log;
}

static void TestLog()
{
    const auto oldLv = GetConsoleBackend()->GetLeastLevel();
    //GetConsoleBackend()->SetLeastLevel(LogLevel::Info);
    SimpleTimer timer;
    log().info(u"Plain call.\n");
    std::string name = "tst";
    timer.Start();
    for (uint32_t i = 0; i < 5000; ++i)
        log().verbose(FMT_STRING(u"Dummy Data Here {}.\n"), name);
    timer.Stop();
    //GetConsoleBackend()->SetLeastLevel(oldLv);
    log().success(u"Total {} us, each takes [{}] ns", timer.ElapseUs(), timer.ElapseNs() / 5000);
    getchar();
}


const static uint32_t ID = RegistTest("LogTest", &TestLog);
