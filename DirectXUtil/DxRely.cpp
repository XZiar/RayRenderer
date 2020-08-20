#include "DxPch.h"

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


DxException::DxException(std::u16string msg) : common::BaseException(T_<ExceptionInfo>{}, msg)
{ }

DxException::DxException(common::HResultHolder hresult, std::u16string msg) : DxException(msg)
{
    Attach("HResult", hresult);
    Attach("detail", hresult.ToStr());
}


}