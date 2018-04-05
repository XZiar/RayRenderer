#include "ThreadEx.h"
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <ProcessThreadsApi.h>
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
    /*
    const DWORD tid = GetCurrentThreadId();
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
    */
    std::u16string u16TName(threadName.cbegin(), threadName.cend());
    SetThreadName(u16TName);
    return true;
}

bool __cdecl SetThreadName(const std::u16string& threadName)
{
    SetThreadDescription(GetCurrentThread(), (PCWSTR)threadName.c_str());
    return true;
}

ThreadExitor& __cdecl ThreadExitor::GetThreadExitor()
{
    thread_local static ThreadExitor exitor;
    return exitor;
}

ThreadExitor::~ThreadExitor()
{
    for (const auto& funcPair : Funcs)
    {
        std::get<1>(funcPair)();
    }
}


static void* CopyThreadHandle(void *src)
{
    HANDLE Handle = nullptr;
    DuplicateHandle(GetCurrentProcess(), (HANDLE)src, GetCurrentProcess(), &Handle, SYNCHRONIZE | THREAD_QUERY_INFORMATION, false, 0);
    return Handle;
}

ThreadObject::~ThreadObject()
{
    if (Handle)
        ::CloseHandle((HANDLE)Handle);
}
bool ThreadObject::IsAlive() const
{
    if (!Handle)
        return false;
    const auto rc = ::WaitForSingleObject((HANDLE)Handle, 0);
    return rc != WAIT_OBJECT_0;
}
bool ThreadObject::IsCurrent() const
{
    return GetThreadId((HANDLE)Handle) == GetCurrentThreadId();
}
uint32_t ThreadObject::GetId() const 
{
    return GetThreadId((HANDLE)Handle);
}


uint32_t ThreadObject::GetCurrentThreadId()
{
    return ::GetCurrentThreadId();
}
ThreadObject ThreadObject::GetCurrentThreadObject()
{
    return ThreadObject{ CopyThreadHandle(GetCurrentThread()) };
}
ThreadObject ThreadObject::GetThreadObject(std::thread& thr)
{
    return ThreadObject{ CopyThreadHandle(thr.native_handle()) };
}


}