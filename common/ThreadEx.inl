#include "ThreadEx.h"
#include "StrCharset.hpp"
#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN 1
# define NOMINMAX 1
# include <Windows.h>
# include <ProcessThreadsApi.h>
# include "WinVersionHelper.hpp"
#else
//# define _GNU_SOURCE
# include <errno.h>
# include <pthread.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/resource.h>
# include <sys/wait.h>
# include <sys/sysinfo.h>
# if defined(__APPLE__)
#   include <sys/sysctl.h>
#   include <mach/mach_types.h>
#   include <mach/thread_act.h>
# else
#   include <sched.h>
#   include <sys/syscall.h>
# endif
#endif
#include <thread>


namespace common
{

namespace detail
{
#pragma pack(push,8) 
struct THREADNAME_INFO
{
    DWORD dwType;  // Must be 0x1000.
    LPCSTR szName;  // Pointer to name (in user addr space).
    DWORD dwThreadID;  // Thread ID (-1=caller thread).
    DWORD dwFlags;  // Reserved for future use, must be zero.
};
#pragma pack(pop)
}

bool CDECLCALL SetThreadName(const std::string& threadName)
{
    const std::u16string u16TName(threadName.cbegin(), threadName.cend());
    SetThreadName(u16TName);
    return true;
}

static void SetThreadNameImpl(const detail::THREADNAME_INFO* info)
{
    __try
    {
        RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    { }
}

bool CDECLCALL SetThreadName(const std::u16string& threadName)
{
#if defined(_WIN32)
    if (GetWinBuildNumber() >= 14393) // supported since Win10 1607
        ::SetThreadDescription(::GetCurrentThread(), (PCWSTR)threadName.c_str()); 
    else
    {
        constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;
        const DWORD tid = GetCurrentThreadId();
        detail::THREADNAME_INFO info;
        info.dwType = 0x1000;
        const auto asciiThreadName = str::to_string(threadName, str::Charset::ASCII, str::Charset::UTF16LE);
        info.szName = asciiThreadName.c_str();
        info.dwThreadID = tid;
        info.dwFlags = 0;
        SetThreadNameImpl(&info);
    }
#else
    const auto u8TName = str::to_u8string(threadName, str::Charset::UTF8);
# if defined(__APPLE__)
    pthread_setname_np(u8TName.c_str());
# else
    pthread_setname_np(pthread_self(), u8TName.c_str());
# endif
#endif
    return true;
}

ThreadExitor& CDECLCALL ThreadExitor::GetThreadExitor()
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
#if defined(_WIN32)
    if (Handle == (uintptr_t)INVALID_HANDLE_VALUE)
        return false;
    const auto rc = ::WaitForSingleObject((HANDLE)Handle, 0);
    return rc != WAIT_OBJECT_0;
#else
    if (Handle == 0)
        return false;
    const auto ret = pthread_tryjoin_np((pthread_t)Handle, nullptr); // only suitable for joinable thread
    return ret == EBUSY;
#endif
}
bool ThreadObject::IsCurrent() const
{
    return TId == ThreadObject::GetCurrentThreadId();
}
uint64_t ThreadObject::GetId() const
{
#if defined(_WIN32)
    return ::GetThreadId((HANDLE)Handle);
#elif defined(__APPLE__)
    pthread_id_np_t   tid;
    pthread_getunique_np((pthread_t)Handle, &tid);
    return tid;
#else
    return syscall(SYS_gettid);
    //return Handle;
#endif
}


uint64_t ThreadObject::GetCurrentThreadId()
{
#if defined(_WIN32)
    return ::GetCurrentThreadId();
#elif defined(__APPLE__)
    return pthread_getthreadid_np();
#else
    //return (long int)syscall(__NR_gettid);
    return pthread_self();
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