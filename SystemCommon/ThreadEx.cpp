#include "SystemCommonPch.h"
#include "ThreadEx.h"
#if COMMON_OS_ANDROID
#   include <sys/prctl.h>
#endif
#if COMMON_OS_DARWIN
#   include <pthread/qos.h>
#endif

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
        return ::SetThreadDescription(::GetCurrentThread(), (PCWSTR)u16ThrName.data()) == S_OK;
    }
    else
    {
        SetThreadNameImpl(std::string(threadName), ::GetCurrentThreadId());
        return true;
    }
#else
    char buf[16] = {0}; // pthread limit name to 16 bytes(including null)
    for (size_t i = 0; i < threadName.length() && i < 15; ++i)
        buf[i] = threadName[i];
# if COMMON_OS_DARWIN
    return pthread_setname_np(buf) == 0;
# elif !COMMON_OS_ANDROID || (__ANDROID_API__ >= 9)
    return pthread_setname_np(pthread_self(), buf) == 0;
# elif COMMON_OS_ANDROID
#   ifndef PR_SET_NAME
#     define PR_SET_NAME 15
#   endif
    return prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(buf), 0, 0, 0) == 0;
# endif
#endif
}

bool SetThreadName(const std::u16string_view threadName)
{
#if COMMON_OS_WIN
    if (GetWinBuildNumber() >= 14393) // supported since Win10 1607
    {
        return ::SetThreadDescription(::GetCurrentThread(), (PCWSTR)threadName.data()) == S_OK;
    }    
    else
    {
        const auto asciiThreadName = str::to_string(threadName, str::Encoding::ASCII, str::Encoding::UTF16LE);
        SetThreadNameImpl(asciiThreadName, ::GetCurrentThreadId());
        return true;
    }
#else
    const auto u8TName = str::to_string(threadName, str::Encoding::UTF8, str::Encoding::UTF16LE);
    return SetThreadName(u8TName);
#endif
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
# if !COMMON_OS_ANDROID || (__ANDROID_API__ >= 26)
    pthread_getname_np(pthread_self(), tmp, sizeof(tmp));
# elif COMMON_OS_ANDROID
#   ifndef PR_GET_NAME
#     define PR_GET_NAME 16
#   endif
    prctl(PR_GET_NAME, reinterpret_cast<unsigned long>(tmp), 0, 0, 0);
# endif
    return str::to_u16string(&tmp[0], str::Encoding::UTF8);
#endif
}


std::optional<ThreadQoS> SetThreadQoS(ThreadQoS qos)
{
    std::optional<ThreadQoS> actualQos;
#if COMMON_OS_WIN
    THREAD_POWER_THROTTLING_STATE powerThrottling;
    ZeroMemory(&powerThrottling, sizeof(powerThrottling));
    powerThrottling.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
    switch (qos)
    {
    case ThreadQoS::High:
    case ThreadQoS::Burst:
        powerThrottling.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        powerThrottling.StateMask   = 0;
        actualQos = ThreadQoS::High;
        break;
    case ThreadQoS::Utility:
    case ThreadQoS::Background:
        powerThrottling.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        powerThrottling.StateMask   = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        actualQos = ThreadQoS::Background;
        break;
    default:
    case ThreadQoS::Default:
        powerThrottling.ControlMask = 0;
        powerThrottling.StateMask   = 0;
        actualQos = ThreadQoS::Default;
        break;
    }
    if (0 == SetThreadInformation(GetCurrentThread(),
        ThreadPowerThrottling, &powerThrottling, sizeof(powerThrottling)))
        return {};
#elif COMMON_OS_DARWIN
    qos_class_t val = QOS_CLASS_DEFAULT;
    switch (qos)
    {
    case ThreadQoS::High:       val = QOS_CLASS_USER_INTERACTIVE; break;
    case ThreadQoS::Burst:      val = QOS_CLASS_USER_INITIATED;   break;
    case ThreadQoS::Utility:    val = QOS_CLASS_UTILITY;          break;
    case ThreadQoS::Background: val = QOS_CLASS_BACKGROUND;       break;
    default:
    case ThreadQoS::Default:    val = QOS_CLASS_DEFAULT;          break;
    }
    if (pthread_set_qos_class_self_np(val, 0) == 0)
        return qos;
#else
#endif
    return actualQos;
}

std::optional<ThreadQoS> GetThreadQoS()
{
    std::optional<ThreadQoS> actualQos;
#if COMMON_OS_WIN
    // no way to query
#elif COMMON_OS_DARWIN
    qos_class_t val = QOS_CLASS_DEFAULT;
    if (pthread_get_qos_class_np(pthread_self(), &val, nullptr) == 0)
    {
        switch (val)
        {
        case QOS_CLASS_USER_INTERACTIVE: return ThreadQoS::High;
        case QOS_CLASS_USER_INITIATED:   return ThreadQoS::Burst;
        case QOS_CLASS_UTILITY:          return ThreadQoS::Utility;
        case QOS_CLASS_BACKGROUND:       return ThreadQoS::Background;
        case QOS_CLASS_DEFAULT:          return ThreadQoS::Default;
        default: break;
        }
    }
#else
#endif
    return actualQos;
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
#else
    if (Handle == 0)
        return false;
#   if COMMON_OS_ANDROID || COMMON_OS_DARWIN
    return {};
#   else
    const auto ret = pthread_tryjoin_np((pthread_t)Handle, nullptr); // only suitable for joinable thread
    return ret == EBUSY;
#   endif
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
#elif COMMON_OS_DARWIN
    uint64_t tid = 0;
    pthread_threadid_np((pthread_t)Handle, &tid);
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
#elif COMMON_OS_DARWIN
    uint64_t tid = 0;
    pthread_threadid_np(nullptr, &tid);
    return tid;
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
#elif COMMON_OS_DARWIN
    return ThreadObject(reinterpret_cast<uintptr_t>(pthread_self()));
#else
    return ThreadObject(pthread_self());
#endif
}
ThreadObject ThreadObject::GetThreadObject(std::thread& thr)
{
#if COMMON_OS_WIN
    return ThreadObject(CopyThreadHandle(thr.native_handle()));
#elif COMMON_OS_DARWIN
    return ThreadObject(reinterpret_cast<uintptr_t>(thr.native_handle()));
#else
    return ThreadObject(thr.native_handle());
#endif
}


}