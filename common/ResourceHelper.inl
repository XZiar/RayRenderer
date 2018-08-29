#include "ResourceHelper.h"
#include "Exceptions.hpp"

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN 1
#   include <Windows.h>
namespace common
{
static HMODULE& ThisDll()
{
    static HMODULE dll = nullptr;
    return dll;
}
void ResourceHelper::init(void* dll)
{
    ThisDll() = (HMODULE)dll;
}
std::vector<uint8_t> ResourceHelper::getData(const wchar_t *type, const int32_t id)
{
    HMODULE hdll = ThisDll();
    const auto hRsrc = FindResource(hdll, (wchar_t*)intptr_t(id), type);
    if (NULL == hRsrc)
        COMMON_THROW(BaseException, u"Failed to find resource");
    DWORD dwSize = SizeofResource(hdll, hRsrc);
    if (0 == dwSize)
        COMMON_THROW(BaseException, u"Resource has zero size");
    HGLOBAL hGlobal = LoadResource(hdll, hRsrc);
    if (NULL == hGlobal)
        COMMON_THROW(BaseException, u"Failed to load resource");
    LPVOID pBuffer = LockResource(hGlobal);
    if (NULL == pBuffer)
        COMMON_THROW(BaseException, u"Failed to lock resource");
    std::vector<uint8_t> data;
    data.resize(dwSize);
    memcpy_s(data.data(), dwSize, pBuffer, dwSize);
    return data;
}
}
#else
#   include <map>
namespace common
{
static std::map<int32_t, std::pair<const char*, size_t>>& RES_MAP()
{
    static std::map<int32_t, std::pair<const char*, size_t>> resMap;
    return resMap;
}
namespace detail
{
uint32_t RegistResource(const int32_t id, const char* ptrBegin, const char* ptrEnd)
{
    RES_MAP().emplace(id, std::pair<const char*, size_t>(ptrBegin, ptrEnd - ptrBegin));
    return 0;
}
}
void ResourceHelper::init(void* dll)
{ }
std::vector<uint8_t> ResourceHelper::getData(const wchar_t *, const int32_t id)
{
    const auto& resMap = RES_MAP();
    const auto res = resMap.find(id);
    if (res == resMap.cend())
        COMMON_THROW(BaseException, u"Failed to find resource");
    const auto ptr = res->second.first;
    const size_t size = res->second.second;
    if (0 == size)
        COMMON_THROW(BaseException, u"Resource has zero size");
    std::vector<uint8_t> data;
    data.resize(size);
    memcpy_s(data.data(), size, ptr, size);
    return data;
}
}

#endif