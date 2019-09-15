#pragma once
#include "TexUtilRely.h"
#include "AsyncExecutor/AsyncManager.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

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

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
