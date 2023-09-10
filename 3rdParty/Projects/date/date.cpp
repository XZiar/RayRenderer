#include "common/ResourceHelper.inl"
#include "common/CommonRely.hpp"
#include "resource.h"

#pragma message("Compiling date with tzdata[" STRINGIZE(TZDATA_VERSION) "]" )


#if COMMON_OS_WIN

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        common::ResourceHelper::Init(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

#endif

std::pair<const char*, size_t> tz_get_file_data(std::string_view name) noexcept
{
    common::span<const std::byte> ret;
    if (name == "version")
    {
        static constexpr std::string_view ver = STRINGIZE(TZDATA_VERSION);
        ret = { reinterpret_cast<const std::byte*>(ver.data()), ver.size() };
    }
#define LoadData(idr, fname) if (name == #fname) ret = common::ResourceHelper::GetData(L"TZDATA", IDR_TZDATA_##idr)
    else LoadData(AFRICA, africa);
    else LoadData(ANTARCTICA, antarctica);
    else LoadData(ASIA, asia);
    else LoadData(AUSTRALASIA, australasia);
    else LoadData(BACKWARD, backward);
    else LoadData(BACKZONE, backzone);
    else LoadData(ETCETERA, etcetera);
    else LoadData(EUROPE, europe);
    else LoadData(NORTHAMERICA, northamerica);
    else LoadData(SOUTHAMERICA, southamerica);
    else LoadData(LEAPSECONDS, leapseconds);
    else LoadData(WINDOWSZONES, windowsZones.xml);
#undef LoadData
    return { reinterpret_cast<const char*>(ret.data()), ret.size() };
}
