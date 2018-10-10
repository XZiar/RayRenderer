#include "TestRely.h"
#include "common/AsyncExecutor/AsyncAgent.h"
#include "common/AsyncExecutor/AsyncManager.h"
#include "common/miniLogger/QueuedBackend.h"
#include "common/SpinLock.hpp"
#include "common/TimeUtil.hpp"
#include "common/StrCharset.hpp"
#include "fmt/format.h"
#include <thread>
#include <sstream>

using namespace common;
using namespace common::mlog;
using namespace common::asyexe;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"AsyncTest", { GetConsoleBackend() });
    return logger;
}

volatile uint32_t taskid = 0;

static auto getTimer(const uint32_t sec)
{
    return [=](const AsyncAgent& agent) 
    {
        const auto tid = taskid++;
        SimpleTimer timer;
        log().info(u"[{}]timer created\n", tid);
        while (true)
        {
            timer.Start();
            agent.Sleep(sec * 1000);
            timer.Stop();
            log().info(u"[{}]time reached within {}ms\n", tid, timer.ElapseMs());
        }
    };
}

static auto getException(const uint32_t sec, const std::u16string& str)
{
    return [=](const AsyncAgent& agent)
    {
        const auto tid = taskid++;
        SimpleTimer timer;
        log().info(u"[{}]exception created\n", tid);
        agent.Sleep(sec * 1000);
        log().info(u"[{}]begin throw\n", tid);
        COMMON_THROW(BaseException, u"Error Msg:\n" + str);
    };
}

static auto getStall(const uint32_t stms, const uint32_t slsec)
{
    return AsyncTaskFunc([=](const AsyncAgent& agent)
    {
        const auto tid = taskid++;
        SimpleTimer timer;
        log().info(u"[{}]stall created\n", tid);
        while (true)
        {
            timer.Start();
            std::this_thread::sleep_for(std::chrono::milliseconds(stms));
            log().info(u"[{}]time stalled\n", tid);
            agent.Sleep(slsec * 1000);
            timer.Stop();
            log().info(u"[{}]time reached within {}ms\n", tid, timer.ElapseMs());
        }
    });
}

static std::u16string fmtContext(const boost::context::continuation& ctx)
{
    std::ostringstream strstream;
    strstream << ctx;
    return str::to_u16string(strstream.str());
}

static void AsyncTest()
{
    log().info(u"Test 25=[{}], 100=[{}]\n", GetLogLevelStr((LogLevel)25), common::mlog::GetLogLevelStr((LogLevel)100));
    AsyncManager manager(u"MainAsyncer");
    manager.Start([&] { log().success(u"MainLoop thread initilized.\n"); }, [&] {log().success(u"MainLoop thread finished.\n"); });
    common::WRSpinLock wrLock;
    common::PromiseResult<void> pms;
    while (true)
    {
        std::string act;
        std::cin >> act;
        if (act == "timer")
        {
            uint32_t timesec = 0;
            std::cin >> timesec;
            manager.AddTask(getTimer(timesec));
        }
        else if (act == "stall")
        {
            uint32_t stms = 0, slsec = 0;
            std::cin >> stms >> slsec;
            manager.AddTask(getStall(stms, slsec));
        }
        else if (act == "ex")
        {
            uint32_t sec = 0;
            std::string msg;
            std::cin >> sec >> msg;
            pms = manager.AddTask((getException(sec, str::to_u16string(msg))));
        }
        else if (act == "wait" && pms)
        {
            try
            {
                pms->Wait();
            }
            catch (const BaseException &e)
            {
                log().warning(u"received excption:\n{}\n", e.message);
            }
            pms = nullptr;
        }
        else if (act == "list")
        {
            /*
            std::u16string buf = fmt::format(u"current[{:p}], main-ctx[{}]\n", (void*)manager.Current, fmtContext(manager.Context));
            for (common::asyexe::detail::AsyncTaskNode* ptr = manager.Head; ptr != nullptr; ptr = ptr->Next)
            {
                buf += fmt::format(u"[{:p}] status[{}], context[{}]\n", (void*)ptr, (uint8_t)ptr->Status, fmtContext(ptr->Context));
            }
            log().debug(u"list task-list\n{}\n", buf);
            */
        }
        else if (act == "exit")
        {
            manager.Stop();
        }
        else if (act == "reader")
        {
            uint32_t timesec = 0;
            std::cin >> timesec;
            std::thread([&](const uint32_t waitsec) 
            {
                const auto tid = taskid++;
                log().verbose(u"thr-reader[{}] init\n", tid);
                wrLock.LockRead();
                //std::_Atomic_fetch_add_4((std::_Uint4_t*)&taskid, 1, std::memory_order_seq_cst);
                log().success(u"thr-reader[{}] get lock\n", tid);
                std::this_thread::sleep_for(std::chrono::seconds(waitsec));
                log().success(u"thr-reader[{}] release lock\n", tid);
                wrLock.UnlockRead();
            }, timesec).detach();
        }
        else if (act == "writer")
        {
            uint32_t timesec = 0;
            std::cin >> timesec;
            std::thread([&](const uint32_t waitsec)
            {
                const auto tid = taskid++;
                log().verbose(u"thr-writer[{}] init\n", tid);
                wrLock.LockWrite();
                log().success(u"thr-writer[{}] get lock\n", tid);
                std::this_thread::sleep_for(std::chrono::seconds(waitsec));
                log().success(u"thr-writer[{}] release lock\n", tid);
                wrLock.UnlockWrite();
            }, timesec).detach();
        }
    }
}

const static uint32_t ID = RegistTest("AsyncTest", &AsyncTest);

