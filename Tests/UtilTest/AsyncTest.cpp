#include "TestRely.h"
#include "SystemCommon/AsyncAgent.h"
#include "SystemCommon/AsyncManager.h"
#include "SystemCommon/ConsoleEx.h"
#include "common/SpinLock.hpp"
#include "common/TimeUtil.hpp"
#include "SystemCommon/StringConvert.h"
#include <thread>


using namespace common;
using namespace common::mlog;
using namespace common::asyexe;
using namespace common::console;

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
//        log().Info(u"[{}]exception created\n", tid);
//        agent.Sleep(sec * 1000);
//        log().Info(u"[{}]begin throw\n", tid);
//        COMMON_THROW(BaseException, u"Error Msg:\n" + str);
//    };
//}

//static auto getStall(const uint32_t stms, const uint32_t slsec)
//{
//    return AsyncTaskFunc([=](const AsyncAgent& agent)
//    {
//        const auto tid = taskid++;
//        SimpleTimer timer;
//        log().Info(u"[{}]stall created\n", tid);
//        while (true)
//        {
//            timer.Start();
//            std::this_thread::sleep_for(std::chrono::milliseconds(stms));
//            log().Info(u"[{}]time stalled\n", tid);
//            agent.Sleep(slsec * 1000);
//            timer.Stop();
//            log().Info(u"[{}]time reached within {}ms\n", tid, timer.ElapseMs());
//        }
//    });
//}



static void AsyncTest()
{
    log().Info(u"Test 25=[{}], 100=[{}]\n", GetLogLevelStr((LogLevel)25), common::mlog::GetLogLevelStr((LogLevel)100));
    AsyncManager producer(u"Producer");
    producer.Start([&] { log().Success(u"Producer thread initilized.\n"); }, [&] {log().Success(u"Producer thread finished.\n"); });
    common::WRSpinLock wrLock;
    common::PromiseResult<void> pms;
    while (true)
    {
        const auto key = ConsoleEx::ReadCharImmediate(false);
        if (key == 'x')
        {
            auto pms = producer.AddTask([](const AsyncAgent& agent)
                {
                    agent.Sleep(3000);
                    //return 1;
                });
            pms->OnComplete([]() 
                { 
                    printf("Recieved one [%d]\n", 1);
                });
        }
        else if (key == 'q')
        {
            producer.Stop();
            break;
        }
    }

    getchar();
    getchar();
    getchar();
}

const static uint32_t ID = RegistTest("AsyncTest", &AsyncTest);

