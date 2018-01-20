#include "ThreadEx.h"
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <thread>

namespace common
{
namespace detail
{
	constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;
}

#pragma pack(push,8) 
struct THREADNAME_INFO
{
	DWORD dwType;  // Must be 0x1000.
	LPCSTR szName;  // Pointer to name (in user addr space).
	DWORD dwThreadID;  // Thread ID (-1=caller thread).
	DWORD dwFlags;  // Reserved for future use, must be zero.
};
#pragma pack(pop)

bool __cdecl SetThreadName(const std::string& threadName)
{
	const DWORD tid = *(DWORD *)&std::this_thread::get_id();
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName.c_str();
	info.dwThreadID = tid;
	info.dwFlags = 0;
	__try
	{
		RaiseException(detail::MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{ }
	return true;
}

}