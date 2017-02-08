#include "ResourceHelper.h"
#include "string"

namespace common
{

HINSTANCE ResourceHelper::thisdll = nullptr;

void ResourceHelper::init(HINSTANCE dll)
{
	thisdll = dll;
}

ResourceHelper::Result ResourceHelper::getData(std::vector<uint8_t>& data, const wchar_t *type, const int32_t id)
{
	const auto hRsrc = FindResource(thisdll, (wchar_t*)id, type);
	if (NULL == hRsrc)
		return Result::FindFail;
	DWORD dwSize = SizeofResource(thisdll, hRsrc);
	if (0 == dwSize)
		return Result::ZeroSize;
	HGLOBAL hGlobal = LoadResource(thisdll, hRsrc);
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


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	common::ResourceHelper::init(hinstDLL);
	return TRUE;
}