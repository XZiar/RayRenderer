#include "TestRely.h"
#include "common/AsyncExecutor/AsyncAgent.h"
#include "common/AsyncExecutor/AsyncManager.h"
#include "common/AsyncExecutor/AsyncProxy.h"
#include "common/SpinLock.hpp"
#include "common/TimeUtil.hpp"
#include "StringCharset/Convert.h"
#include "fmt/format.h"
#include <thread>
#if defined(_WIN32)
#  include <conio.h>
#else
#  include <termios.h>
inline char _getch()
{
    char buf = 0;
    termios oldx;
    tcgetattr(0, &oldx);
    termios newx = oldx;
    newx.c_lflag &= ~ICANON;
    newx.c_lflag &= ~ECHO;
    newx.c_cc[VMIN] = 1;
    newx.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &newx);
    read(0, &buf, 1);
    tcsetattr(0, TCSADRAIN, &oldx);
    return buf;
 }
#endif

using namespace common;
using namespace common::mlog;
using namespace common::asyexe;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"AsyncTest", { GetConsoleBackend() });
    return logger;
}

//static auto getException(const uint32_t sec, const std::u16string& str)
//{
//    return [=](const AsyncAgent& agent)
//    {
//        const auto tid = taskid++;
//        SimpleTimer timer;
//        log().info(u"[{}]exception created\n", tid);
//        agent.Sleep(sec * 1000);
//        log().info(u"[{}]begin throw\n", tid);
//        COMMON_THROW(BaseException, u"Error Msg:\n" + str);
//    };
//}

//static auto getStall(const uint32_t stms, const uint32_t slsec)
//{
//    return AsyncTaskFunc([=](const AsyncAgent& agent)
//    {
//        const auto tid = taskid++;
//        SimpleTimer timer;
//        log().info(u"[{}]stall created\n", tid);
//        while (true)
//        {
//            timer.Start();
//            std::this_thread::sleep_for(std::chrono::milliseconds(stms));
//            log().info(u"[{}]time stalled\n", tid);
//            agent.Sleep(slsec * 1000);
//            timer.Stop();
//            log().info(u"[{}]time reached within {}ms\n", tid, timer.ElapseMs());
//        }
//    });
//}



static void AsyncTest()
{
    log().info(u"Test 25=[{}], 100=[{}]\n", GetLogLevelStr((LogLevel)25), common::mlog::GetLogLevelStr((LogLevel)100));
    AsyncManager producer(u"Producer");
    producer.Start([&] { log().success(u"Producer thread initilized.\n"); }, [&] {log().success(u"Producer thread finished.\n"); });
    common::WRSpinLock wrLock;
    common::PromiseResult<void> pms;
    while (true)
    {
        const auto key = _getch();
        if (key == 'x')
        {
            auto pms = producer.AddTask([](const AsyncAgent& agent)
                {
                    agent.Sleep(3000);
                    //return 1;
                });
            AsyncProxy::OnComplete(pms, [&]() { printf("Recieved one [%d]\n", 1); });
        }
    }
}

const static uint32_t ID = RegistTest("AsyncTest", &AsyncTest);

