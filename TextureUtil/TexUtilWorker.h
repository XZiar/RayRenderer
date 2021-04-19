#pragma once
#include "TexUtilRely.h"
#include "AsyncExecutor/AsyncManager.h"


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oglu::texutil
{

class TEXUTILAPI TexUtilWorker : public common::NonCopyable, public common::NonMovable
{
private:
    common::asyexe::AsyncManager Executor;
public:
    oglContext GLContext;
    oclu::oclContext CLContext;
    oclu::oclCmdQue CmdQue;
    TexUtilWorker(oglContext&& glContext, const oclu::oclContext& clContext);
    ~TexUtilWorker();
    template<typename T>
    auto AddTask(T&& task) { return Executor.AddTask(task); }
};

}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
