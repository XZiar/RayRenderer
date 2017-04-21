#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include "ResourceHelper.h"

namespace common
{

void* ResourceHelper::thisdll = nullptr;

void ResourceHelper::init(void* dll)
{
	thisdll = dll;
}

ResourceHelper::Result ResourceHelper::getData(std::vector<uint8_t>& data, const wchar_t *type, const int32_t id)
{
	HMODULE hdll = (HMODULE)thisdll;
	const auto hRsrc = FindResource(hdll, (wchar_t*)intptr_t(id), type);
	if (NULL == hRsrc)
		return Result::FindFail;
	DWORD dwSize = SizeofResource(hdll, hRsrc);
	if (0 == dwSize)
		return Result::ZeroSize;
	HGLOBAL hGlobal = LoadResource(hdll, hRsrc);
	if (NULL == hGlobal)
		return Result::LoadFail;
	LPVOID pBuffer = LockResource(hGlobal);
	if (NULL == pBuffer)
		return Result::LockFail;
	data.resize(dwSize);
	memmove(data.data(), pBuffer, dwSize);
	return Result::Success;
}

}
