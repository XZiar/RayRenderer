#include "dxPch.h"
#include "comdef.h"

#if defined(_WIN32)
#   pragma comment (lib, "D3d12.lib")
#   pragma comment (lib, "DXGI.lib")
#endif
#pragma message("Compiling DirectXUtil with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace dxu
{

using namespace common::mlog;
MiniLogger<false>& dxLog()
{
    static MiniLogger<false> dxlog(u"DirectXUtil", { GetConsoleBackend() });
    return dxlog;
}


DXException::DXException(int32_t hresult, std::u16string msg) : common::BaseException(T_<ExceptionInfo>{}, msg)
{
    _com_error err(hresult);
    Attach("HResult", hresult);
    std::u16string hrmsg(reinterpret_cast<const char16_t*>(err.ErrorMessage()));
    Attach("Detail", std::move(hrmsg));
}


}