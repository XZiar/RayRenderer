#pragma once

#include "oglRely.h"
#include "oglContext.h"
#include "AsyncExecutor/AsyncManager.h"

namespace oglu
{


class OGLUAPI oglWorker
{
private:
    const std::u16string Name;
    common::asyexe::AsyncManager ShareExecutor, IsolateExecutor;
    oglContext ShareContext, IsolateContext;
public:
    oglWorker(std::u16string name) : Name(name), ShareExecutor(u"oglWorker[S]-" + name), IsolateExecutor(u"oglWorker[I]-" + name)
    {
    }
    ~oglWorker()
    {
        ShareExecutor.Stop();
        IsolateExecutor.Stop();
    }
    //uses current glContext
    void Start();
    template<typename Func>
    auto InvokeShare(Func&& task, std::u16string taskname = u"", uint32_t stackSize = 0)
    {
        return ShareExecutor.AddTask(std::forward<Func>(task), taskname, stackSize);
    }
    template<typename Func>
    auto InvokeIsolate(Func&& task, std::u16string taskname = u"", uint32_t stackSize = 0)
    {
        return IsolateExecutor.AddTask(std::forward<Func>(task), taskname, stackSize);
    }
};



}