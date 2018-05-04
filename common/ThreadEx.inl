#include "ThreadEx.h"
#include "StrCharset.hpp"
#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN 1
# include <Windows.h>
# include <ProcessThreadsApi.h>
#else
#   include <pthread.h>
#   include <unistd.h>
#   include <sys/types.h>
#   include <sys/resource.h>
#   include <sys/wait.h>
#   include <sys/sysinfo.h>
# if defined(__APPLE__)
#   include <sys/sysctl.h>
#   include <mach/mach_types.h>
#   include <mach/thread_act.h>
# else
#   include <sched.h>
# endif
#endif
#include <thread>


namespace common
{

//#pragma pack(push,8) 
//struct THREADNAME_INFO
//{
//    DWORD dwType;  // Must be 0x1000.
//    LPCSTR szName;  // Pointer to name (in user addr space).
//    DWORD dwThreadID;  // Thread ID (-1=caller thread).
//    DWORD dwFlags;  // Reserved for future use, must be zero.
//};
//#pragma pack(pop)

bool __cdecl SetThreadName(const std::string& threadName)
{
    /*
    constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;
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
#if defined(_WIN32)
    ::SetThreadDescription(::GetCurrentThread(), (PCWSTR)threadName.c_str()); // supported since Win10 1607
#else
    const auto u8TName = str::to_u8string(threadName, str::Charset::UTF8);
# if defined(__APPLE__)
    pthread_setname_np(u8TName.c_str());
# else
    pthread_setname_np(pthread_self(), u8TName.c_str())
# endif
#endif
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


ThreadObject::~ThreadObject()
{
#if defined(_WIN32)
    if (Handle != 0)
        ::CloseHandle((HANDLE)Handle);
#endif
}
bool ThreadObject::IsAlive() const
{
    if (Handle != 0)
        return false;
#if defined(_WIN32)
    const auto rc = ::WaitForSingleObject((HANDLE)Handle, 0);
    return rc != WAIT_OBJECT_0;
#else
    return true; // TODO: not implemented (unsupported)
#endif
}
bool ThreadObject::IsCurrent() const
{
    return GetThreadId((HANDLE)Handle) == ThreadObject::GetCurrentThreadId();
}
uint32_t ThreadObject::GetId() const 
{
#if defined(_WIN32)
    return ::GetThreadId((HANDLE)Handle);
#else
    pthread_id_np_t   tid;
    pthread_getunique_np((pthread_t)Handle, &tid);
    return tid;
#endif
}


uint32_t ThreadObject::GetCurrentThreadId()
{
#if defined(_WIN32)
    return ::GetCurrentThreadId();
#else
    return pthread_getthreadid_np();
#endif
}

#if defined(_WIN32)
forceinline static uintptr_t CopyThreadHandle(void *src)
{
    HANDLE Handle = nullptr;
    DuplicateHandle(GetCurrentProcess(), (HANDLE)src, GetCurrentProcess(), &Handle, SYNCHRONIZE | THREAD_QUERY_INFORMATION, false, 0);
    return reinterpret_cast<uintptr_t>(Handle);
}
#endif

ThreadObject ThreadObject::GetCurrentThreadObject()
{
#if defined(_WIN32)
    return ThreadObject(CopyThreadHandle(GetCurrentThread()));
#else
    return ThreadObject(pthread_self());
#endif
}
ThreadObject ThreadObject::GetThreadObject(std::thread& thr)
{
#if defined(_WIN32)
    return ThreadObject(CopyThreadHandle(thr.native_handle()));
#else
    return ThreadObject(thr.native_handle());
#endif
}


}