#include "ResourceHelper.h"
#include "Exceptions.hpp"
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

namespace common
{

void* ResourceHelper::thisdll = nullptr;

void ResourceHelper::init(void* dll)
{
	thisdll = dll;
}

std::vector<uint8_t> ResourceHelper::getData(const wchar_t *type, const int32_t id)
{
	HMODULE hdll = (HMODULE)thisdll;
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
	memmove(data.data(), pBuffer, dwSize);
	return data;
}

}
