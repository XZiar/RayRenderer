#include "DxPch.h"
#include "DxPromise.h"
#include "DxCmdQue.h"


namespace dxu
{
using namespace std::string_view_literals;


DxPromiseCore::DxPromiseCore(const DxCmdQue_& cmdQue, const uint64_t num, const bool isException) :
    CmdQue(std::static_pointer_cast<const DxCmdQue_>(cmdQue.shared_from_this())), Num(num), Handle(nullptr), IsException(isException)
{
}
DxPromiseCore::DxPromiseCore(const bool isException) :
    CmdQue({}), Num(UINT64_MAX), Handle(nullptr), IsException(isException)
{
    Expects(!isException);
}
DxPromiseCore::~DxPromiseCore()
{
    if (Handle)
        CloseHandle((HANDLE)Handle);
}

static common::PromiseState WaitHandle(void* handle, DWORD ms, std::shared_ptr<common::ExceptionBasicInfo>& ex)
{
    Expects(handle != nullptr);
    switch (WaitForSingleObject(handle, ms))
    {
    case WAIT_OBJECT_0:     
        PIXNotifyWakeFromFenceSignal(handle);
        return common::PromiseState::Executed;
    case WAIT_TIMEOUT:
        return common::PromiseState::Executing;
    case WAIT_ABANDONED:    
    case WAIT_FAILED:
    default:
        break;
    }
    ex = CREATE_EXCEPTION(DxException, HRESULT_FROM_WIN32(GetLastError()), u"Failed to wait DXPromise").InnerInfo();
    return common::PromiseState::Error;
}

[[nodiscard]] common::PromiseState DxPromiseCore::GetState() noexcept
{
    using common::PromiseState;
    if (IsException)
        return PromiseState::Error;
    return WaitHandle(Handle, 0, WaitException);
}

void DxPromiseCore::PreparePms()
{
    if (IsException)
        return;
    Expects(Handle == nullptr);
    Handle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (const common::HResultHolder hr = CmdQue->Fence->SetEventOnCompletion(Num, Handle); !hr)
    {
        WaitException = CREATE_EXCEPTION(DxException, hr, u"Failed to call SetEventOnCompletion").InnerInfo();
    }
}

common::PromiseState DxPromiseCore::WaitPms() noexcept
{
    if (WaitException)
        return common::PromiseState::Error;
    return WaitHandle(Handle, INFINITE, WaitException);
}

[[nodiscard]] uint64_t DxPromiseCore::ElapseNs() noexcept
{
    return 0;
}

std::optional<common::BaseException> DxPromiseCore::GetException() const
{
    if (WaitException)
        return WaitException->GetException();
    return {};
}


}
