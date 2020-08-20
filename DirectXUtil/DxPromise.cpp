#include "DxPch.h"
#include "DxPromise.h"
#include "DxCmdQue.h"


namespace dxu
{
using namespace std::string_view_literals;


DxPromiseCore::DxPromiseCore(void* handle, const bool isException) :
    Handle(handle), IsException(isException)
{
}
DxPromiseCore::~DxPromiseCore()
{
    if (Handle)
        CloseHandle((HANDLE)Handle);
}

static common::PromiseState WaitHandle(void* handle, DWORD ms, std::shared_ptr<common::ExceptionBasicInfo>& ex)
{
    switch (WaitForSingleObject(handle, ms))
    {
    case WAIT_OBJECT_0:     return common::PromiseState::Executed;
    case WAIT_TIMEOUT:      return common::PromiseState::Executing;
    case WAIT_ABANDONED:    
    case WAIT_FAILED:
    default:                break;
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
