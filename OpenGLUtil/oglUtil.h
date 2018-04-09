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
    static void __cdecl init();
    static u16string __cdecl getVersion();
    static optional<wstring> __cdecl getError();
    static void applyTransform(Mat4x4& matModel, const TransformOP& op);
    static void applyTransform(Mat4x4& matModel, Mat3x3& matNormal, const TransformOP& op);
    static PromiseResult<void> __cdecl invokeSyncGL(const AsyncTaskFunc& task, const u16string& taskName = u"", const common::asyexe::StackSize stackSize = common::asyexe::StackSize::Default);
    static PromiseResult<void> __cdecl invokeAsyncGL(const AsyncTaskFunc& task, const u16string& taskName = u"", const common::asyexe::StackSize stackSize = common::asyexe::StackSize::Default);
    static common::asyexe::AsyncResult<void> __cdecl SyncGL();
    static common::asyexe::AsyncResult<void> __cdecl ForceSyncGL();
};

}