#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#define DELAYIMP_INSECURE_WRITABLE_HOOKS
#include <delayimp.h>
#include "DelayLoader.h"

namespace common
{

DelayLoader::LoadFunc DelayLoader::onLoadDLL = nullptr;
DelayLoader::LoadFunc DelayLoader::onGetFuncAddr = nullptr;

static FARPROC WINAPI delayHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
	switch (dliNotify)
	{
	case dliStartProcessing:
		break;
	case dliNotePreLoadLibrary:
		if (DelayLoader::onLoadDLL != nullptr)
		{
			const auto name = std::string(pdli->szDll);
			return (FARPROC)DelayLoader::onLoadDLL(std::wstring(name.begin(),name.end()));
		}
		break;
	case dliNotePreGetProcAddress:
		break;
	case dliFailLoadLib:
		break;
	case dliFailGetProc:
		break;
	case dliNoteEndProcessing:
		break;
	}
	return NULL;
}

}

PfnDliHook __pfnDliNotifyHook2 = common::delayHook;
