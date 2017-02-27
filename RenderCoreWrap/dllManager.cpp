#pragma unmanaged
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <cstdio>
#define DELAYIMP_INSECURE_WRITABLE_HOOKS
#include <delayimp.h>

FARPROC WINAPI delayHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
	LPCSTR name = nullptr;
	switch (dliNotify)
	{
	case dliStartProcessing:
		break;
	case dliNotePreLoadLibrary:
		name = pdli->szDll;
		//return (FARPROC)LoadLibrary(L"fakeRenderCore.dll");
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

PfnDliHook __pfnDliNotifyHook2 = delayHook;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		LoadLibrary(L"OpenGLUtil.dll");
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

#pragma managed
