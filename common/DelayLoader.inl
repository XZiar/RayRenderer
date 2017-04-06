#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#define DELAYIMP_INSECURE_WRITABLE_HOOKS
#include <delayimp.h>
#include "DelayLoader.h"

namespace common
{



bool DelayLoader::unload(const std::string& name)
{
	return __FUnloadDelayLoadedDLL2(name.c_str()) == TRUE;
}

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
			return (FARPROC)DelayLoader::onLoadDLL(pdli->szDll);
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
