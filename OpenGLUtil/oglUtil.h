#pragma once
#include "oglRely.h"
#include "common/AsyncExecutor/AsyncAgent.h"

#define OGLU_OPTIMUS_ENABLE_NV extern "C" { _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001; }

namespace oglu
{

using common::asyexe::AsyncTaskFunc;

class OGLUAPI oglUtil
{
private:
public:
    static void init();
    static u16string getVersion();
    static optional<u16string> getError();
    static void applyTransform(Mat4x4& matModel, const TransformOP& op);
    static void applyTransform(Mat4x4& matModel, Mat3x3& matNormal, const TransformOP& op);
    static PromiseResult<void> invokeSyncGL(const AsyncTaskFunc& task, const u16string& taskName = u"", const common::asyexe::StackSize stackSize = common::asyexe::StackSize::Default);
    static PromiseResult<void> invokeAsyncGL(const AsyncTaskFunc& task, const u16string& taskName = u"", const common::asyexe::StackSize stackSize = common::asyexe::StackSize::Default);
    static common::asyexe::AsyncResult<void> SyncGL();
    static common::asyexe::AsyncResult<void> ForceSyncGL();
    static void TryTask();
};

}