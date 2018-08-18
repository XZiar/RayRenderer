#pragma once

#include "oglRely.h"
#include "oglContext.h"
#include "common/AsyncExecutor/AsyncManager.h"

namespace oglu::detail
{


using common::asyexe::AsyncTaskFunc;
class MTWorker
{
private:
    const u16string Name, Prefix;
    common::asyexe::AsyncManager Executor;
    oglContext Context;
public:
    MTWorker(u16string name) : Name(name), Prefix(u"OGLU-Worker " + name), Executor(u"OGLU-" + name)
    {
    }
    ~MTWorker()
    {
        Executor.Stop();
    }
    void start(oglContext&& context);
    common::PromiseResult<void> DoWork(const AsyncTaskFunc& work, const u16string& taskName, const uint32_t stackSize);
};



}