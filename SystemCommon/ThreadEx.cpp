#include "SystemCommonPch.h"
#include "ThreadEx.h"


namespace common
{


#if COMMON_OS_WIN
static void SetThreadNameImpl(const std::string& name, const DWORD tid)
{
#   pragma pack(push,8) 
    struct THREADNAME_INFO
    {
        DWORD dwType = 0x1000;  // Must be 0x1000.
        LPCSTR szName;  // Pointer to name (in user addr space).
        DWORD dwThreadID;  // Thread ID (-1=caller thread).
        DWORD dwFlags;  // Reserved for future use, must be zero.
    };
#   pragma pack(pop)
    constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;
    __try
    {
        THREADNAME_INFO info;
        info.szName = name.c_str();
        info.dwThreadID = tid;
        info.dwFlags = 0;
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    { }
}
#endif


bool SetThreadName(const std::string_view threadName)
{
#if COMMON_OS_WIN
    if (GetWinBuildNumber() >= 14393) // supported since Win10 1607
    {
        const std::u16string u16ThrName(threadName.cbegin(), threadName.cend());
        ::SetThreadDescription(::GetCurrentThread(), (PCWSTR)u16ThrName.data());
    }
    else
    {
        SetThreadNameImpl(std::string(threadName), ::GetCurrentThreadId());
    }
#else
    if (threadName.length() >= 16) // pthread limit name to 16 bytes(including null)
        return SetThreadName(std::string(threadName.substr(0, 15)));
# if COMMON_OS_MACOS
    pthread_setname_np(threadName.data());
# else
    pthread_setname_np(pthread_self(), threadName.data());
# endif
#endif
    return true;
}


bool SetThreadName(const std::u16string_view threadName)
{
#if COMMON_OS_WIN
    if (GetWinBuildNumber() >= 14393) // supported since Win10 1607
        ::SetThreadDescription(::GetCurrentThread(), (PCWSTR)threadName.data()); 
    else
    {
        const auto asciiThreadName = str::to_string(threadName, str::Charset::ASCII, str::Charset::UTF16LE);
        SetThreadNameImpl(asciiThreadName, ::GetCurrentThreadId());
    }
#else
    const auto u8TName = str::to_u8string(threadName, str::Charset::UTF16LE);
    if (u8TName.length() >= 16) // pthread limit name to 16 bytes(including null)
        return SetThreadName(u8TName.substr(0, 15));
# if COMMON_OS_MACOS
    pthread_setname_np(u8TName.data());
# else
    pthread_setname_np(pthread_self(), u8TName.data());
# endif
#endif
    return true;
}

std::u16string GetThreadName()
{
#if COMMON_OS_WIN
    if (GetWinBuildNumber() >= 14393) // supported since Win10 1607
    {
        PWSTR ret;
        ::GetThreadDescription(::GetCurrentThread(), &ret);
        return std::u16string(reinterpret_cast<const char16_t*>(ret));
    }
    else
    {
        return u"";
    }
#else
    char tmp[16] = {0};
# if COMMON_OS_MACOS
    pthread_getname_np(tmp, sizeof(tmp));
# elif !COMMON_OS_ANDROID || (__ANDROID_API__ >= 26)
    pthread_getname_np(pthread_self(), tmp, sizeof(tmp));
# elif COMMON_OS_ANDROID
#   ifndef PR_GET_NAME
#     define PR_GET_NAME 16
#   endif
    prctl(PR_GET_NAME, reinterpret_cast<unsigned long>(tmp), 0, 0, 0);
# endif
    return str::to_u16string(&tmp[0], str::Charset::UTF8);
#endif
}


ThreadObject::~ThreadObject()
{
#if COMMON_OS_WIN
    if (Handle != 0)
        ::CloseHandle((HANDLE)Handle);
#endif
}
std::optional<bool> ThreadObject::IsAlive() const
{
#if COMMON_OS_WIN
    if (Handle == (uintptr_t)INVALID_HANDLE_VALUE)
        return false;
    const auto rc = ::WaitForSingleObject((HANDLE)Handle, 0);
    return rc != WAIT_OBJECT_0;
#elif COMMON_OS_ANDROID
    return {};
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
#if COMMON_OS_WIN
    return ::GetThreadId((HANDLE)Handle);
#elif COMMON_OS_MACOS
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
#if COMMON_OS_WIN
    return ::GetCurrentThreadId();
#elif COMMON_OS_MACOS
    return pthread_getthreadid_np();
#else
    //return (long int)syscall(__NR_gettid);
    return pthread_self();
#endif
}

#if COMMON_OS_WIN
forceinline static uintptr_t CopyThreadHandle(void *src)
{
    HANDLE Handle = nullptr;
    DuplicateHandle(GetCurrentProcess(), (HANDLE)src, GetCurrentProcess(), &Handle, SYNCHRONIZE | THREAD_QUERY_INFORMATION, false, 0);
    return reinterpret_cast<uintptr_t>(Handle);
}
#endif

ThreadObject ThreadObject::GetCurrentThreadObject()
{
#if COMMON_OS_WIN
    return ThreadObject(CopyThreadHandle(GetCurrentThread()));
#else
    return ThreadObject(pthread_self());
#endif
}
ThreadObject ThreadObject::GetThreadObject(std::thread& thr)
{
#if COMMON_OS_WIN
    return ThreadObject(CopyThreadHandle(thr.native_handle()));
#else
    return ThreadObject(thr.native_handle());
#endif
}


}