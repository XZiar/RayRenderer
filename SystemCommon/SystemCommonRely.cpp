#include "SystemCommonPch.h"

#if defined(_WIN32)
namespace common
{

typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
static uint32_t GetWinBuildImpl()
{
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod)
    {
        RtlGetVersionPtr ptrGetVersion = reinterpret_cast<RtlGetVersionPtr>(::GetProcAddress(hMod, "RtlGetVersion"));
        if (ptrGetVersion != nullptr)
        {
            RTL_OSVERSIONINFOW rovi = { 0 };
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if (0 == ptrGetVersion(&rovi))
                return rovi.dwBuildNumber;
        }
    }
    return 0;
}

uint32_t GetWinBuildNumber()
{
    static uint32_t verNum = GetWinBuildImpl();
    return verNum;
}

}
#endif