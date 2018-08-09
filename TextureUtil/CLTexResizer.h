#pragma once
#include "TexUtilRely.h"
#include "common/AsyncExecutor/AsyncManager.h"

namespace oglu::texutil
{

class TEXUTILAPI CLTexResizer : public NonCopyable, public NonMovable
{
private:
    common::asyexe::AsyncManager Executor;
    oglContext GLContext;
    oclu::oclContext CLContext;
    oclu::oclCmdQue ComQue;
    oclu::oclKernel KernelResizer;
public:
    CLTexResizer(const oglContext& glContext);
    ~CLTexResizer();
};



}

