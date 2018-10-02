#pragma once
#include "TexUtilRely.h"
#include "common/AsyncExecutor/AsyncManager.h"

namespace oglu::texutil
{
using common::PromiseResult;
using namespace oclu;

class TEXUTILAPI TexUtilWorker : public NonCopyable, public NonMovable
{
private:
    common::asyexe::AsyncManager Executor;
public:
    oglContext GLContext;
    oclContext CLContext;
    oclCmdQue CmdQue;
    TexUtilWorker(oglContext&& glContext, const oclu::oclContext& clContext);
    ~TexUtilWorker();
    template<typename T>
    auto AddTask(T&& task) { return Executor.AddTask(task); }
};

}
