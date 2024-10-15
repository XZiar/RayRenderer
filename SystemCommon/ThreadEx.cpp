#include "SystemCommonPch.h"
#include "ThreadEx.h"
#include "DynamicLibrary.h"
#include "ErrorCodeHelper.h"
#include "common/FrozenDenseSet.hpp"
#include "common/SpinLock.hpp"
#include "3rdParty/cpuinfo/include/cpuinfo.h"
#if COMMON_ARCH_X86
#   include "3rdParty/cpuinfo/src/x86/cpuid.h"
#endif
#if COMMON_OS_WIN
#   include <ProcessThreadsApi.h>
#   pragma comment(lib, "onecore.lib") // comparehanldes
static_assert(common::detail::is_little_endian, "Only Little Endian on Windows");
#elif COMMON_OS_UNIX
#   include <pthread.h>
#   if COMMON_OS_DARWIN
#   else
#       include <sched.h>
#   endif
#   ifdef __GLIBC__
#       define COMMON_GLIBC_VER (__GLIBC__ * 100 + __GLIBC_MINOR__)
#   endif
#endif
#if COMMON_OS_LINUX && COMMON_ARCH_ARM
#   include <sys/auxv.h>
#   include <asm/hwcap.h>
#endif
#if COMMON_OS_ANDROID
#   include <sys/prctl.h>
#   if __ANDROID_API__ < 21
#       error Need Android API >= 21
#   endif
typedef int (*GetThreadNamePtr)(pthread_t __pthread, char* __buf, size_t __n);
#endif
#if COMMON_OS_DARWIN
#   include <pthread/qos.h>
#endif
#include <map>
#include <inttypes.h>

namespace common
{
using namespace std::string_view_literals;


static void EnsureCPUInfoInited()
{
    [[maybe_unused]] static const auto Dummy = []() 
    {
        cpuinfo_initialize();
        return true;
    }();
}


struct ThreadExHost final : public TopologyInfo
{
    std::vector<uint32_t> Groups;
    uint32_t ProcessorCount = 0;
#if COMMON_OS_WIN
    DynLib Kernel32 = L"kernel32.dll";
#   define DECLARE_FUNC(name) decltype(&name) name = nullptr
#   if NTDDI_VERSION >= NTDDI_WIN10_FE
    DECLARE_FUNC(GetThreadSelectedCpuSetMasks);
    DECLARE_FUNC(SetThreadSelectedCpuSetMasks);
    DECLARE_FUNC(GetProcessDefaultCpuSetMasks);
    bool SupportWin11ThreadGroups() const noexcept
    {
        return GetWinBuildNumber() >= 20348u // win11+ || winserver2022+
            && this->GetThreadSelectedCpuSetMasks && this->SetThreadSelectedCpuSetMasks && this->GetProcessDefaultCpuSetMasks;
    }
#   endif
#   undef DECLARE_FUNC
    // Win11 ThreadGroups seems not to be a hard request, nedd to use old method as well when available
    std::optional<std::pair<uint32_t, uint32_t>> DecideSingleGroup(const ThreadAffinity& affinity) const noexcept
    {
        const auto bytePerGroup = BitsPerGroup / 8;
#   if NTDDI_VERSION >= NTDDI_WIN10_FE
        if (Groups.size() > 1 && SupportWin11ThreadGroups()) // only when need multiple group and also avaialble
        {
            if (affinity.GetCount() == 0) return {};
            std::optional<std::pair<uint32_t, uint32_t>> lastSetGroup;
            for (uint32_t i = 0; i < Groups.size(); ++i)
            {
                if (const auto setCount = MiscIntrin.PopCountRange<std::byte>({ affinity.GetRawData(i), bytePerGroup }); setCount > 0)
                {
                    if (lastSetGroup) return {};// multiple group
                    lastSetGroup.emplace(i, setCount);
                }
            }
            Ensures(lastSetGroup);
            return lastSetGroup;
        }
#   endif
        if (Groups.size() == 1) return std::pair{ 0u, affinity.GetCount() };
        if (affinity.GetCount() == 0) // pick group with most cores
        {
            uint16_t maxGroup = 0;
            for (uint16_t i = 1; i < Groups.size(); ++i)
            {
                if (Groups[i] > Groups[maxGroup])
                    maxGroup = i;
            }
            return std::pair{ maxGroup, 0u };
        }
        else
        {
            uint32_t maxGroup = 0, maxBind = 0;
            for (uint32_t i = 0; i < Groups.size(); ++i)
            {
                const auto setBits = MiscIntrin.PopCountRange<std::byte>({ affinity.GetRawData(i), bytePerGroup });
                if (setBits > maxBind)
                {
                    maxGroup = i;
                    maxBind = setBits;
                }
            }
            return std::pair{ maxGroup, maxBind };
        }
    }
#else
#   if defined(COMMON_GLIBC_VER) && COMMON_GLIBC_VER >= 230
    decltype(&gettid) GetCurrentTid = nullptr;
#   endif
#   if COMMON_OS_ANDROID
    GetThreadNamePtr GetThreadName = nullptr;
#   endif
#endif
    ThreadExHost()
    {
        EnsureCPUInfoInited();
        ProcessorCount = cpuinfo_get_processors_count();
#if COMMON_OS_WIN
        BitsPerGroup = gsl::narrow_cast<uint32_t>(sizeof(KAFFINITY) * 8);
        for (const auto& proc : span<const cpuinfo_processor>{ cpuinfo_get_processors(), cpuinfo_get_processors_count() })
        {
            if (proc.windows_group_id >= Groups.size())
                Groups.resize(proc.windows_group_id + 1);
            auto& maxCount = Groups[proc.windows_group_id];
            maxCount = gsl::narrow_cast<uint8_t>(std::max<uint32_t>(maxCount, proc.windows_processor_id + 1));
            Ensures(maxCount < BitsPerGroup);
        }
#   if NTDDI_VERSION >= NTDDI_WIN10_FE
#       define LOAD_FUNC(name) this->name = Kernel32.TryGetFunction<decltype(name)>(#name)
        LOAD_FUNC(GetThreadSelectedCpuSetMasks);
        LOAD_FUNC(SetThreadSelectedCpuSetMasks);
        LOAD_FUNC(GetProcessDefaultCpuSetMasks);
        this->GetProcessDefaultCpuSetMasks = Kernel32.TryGetFunction<decltype(GetProcessDefaultCpuSetMasks)>("GetProcessDefaultCpuSetMasks");
#       undef LOAD_FUNC
#   endif
#else
        Expects(ProcessorCount < sizeof(cpu_set_t) * 8);
        BitsPerGroup = (ProcessorCount + 8 - 1) / 8 * 8;
        Groups.push_back(ProcessorCount);
#   if defined(COMMON_GLIBC_VER) && COMMON_GLIBC_VER >= 230
        GetCurrentTid = reinterpret_cast<decltype(&gettid)>(dlsym(RTLD_DEFAULT, "gettid"));
#   endif
#   if COMMON_OS_ANDROID
        GetThreadName = reinterpret_cast<GetThreadNamePtr>(dlsym(RTLD_DEFAULT, "pthread_getname_np"));
#   endif
#endif
        Ensures(ProcessorCount > 0);
        Ensures(BitsPerGroup % 8 == 0);
    }
    uint32_t GetTotalProcessorCount() const noexcept final { return ProcessorCount; }
    uint32_t GetGroupCount() const noexcept final { return static_cast<uint32_t>(Groups.size()); }
    uint32_t GetCountInGroup(uint32_t group) const noexcept final { return Groups[group]; }
    static const ThreadExHost& Get() noexcept
    {
        static const ThreadExHost Info;
        return Info;
    }
};

const TopologyInfo& TopologyInfo::Get() noexcept
{
    return ThreadExHost::Get();
}

void TopologyInfo::PrintTopology()
{
    EnsureCPUInfoInited();
    span<const cpuinfo_package> pkgs{ cpuinfo_get_packages(), cpuinfo_get_packages_count() };
    for (const auto& pkg : pkgs)
    {
        printf("package[%s]: cluster[%2u~%2u] core[%2u~%2u] proc[%2u~%2u]\n", pkg.name,
            pkg.cluster_start, pkg.cluster_start + pkg.cluster_count,
            pkg.core_start, pkg.core_start + pkg.core_count,
            pkg.processor_start, pkg.processor_start + pkg.processor_count);
    }
    printf("\n");
    span<const cpuinfo_cluster> clss{ cpuinfo_get_clusters(), cpuinfo_get_clusters_count() };
    for (const auto& cls : clss)
    {
        printf("cluster[%u]: vendor[%u] uarch[%08x] freq[%f] reg[%08x] core[%2u~%2u] proc[%2u~%2u]\n", cls.cluster_id,
            static_cast<uint32_t>(cls.vendor), static_cast<uint32_t>(cls.uarch), cls.frequency / 10e9,
#if COMMON_ARCH_X86
            cls.cpuid,
#elif COMMON_ARCH_ARM
            cls.midr,
#endif
            cls.core_start, cls.core_start + cls.core_count,
            cls.processor_start, cls.processor_start + cls.processor_count);
    }
    printf("\n");
    span<const cpuinfo_core> cores{ cpuinfo_get_cores(), cpuinfo_get_cores_count() };
    for (const auto& core : cores)
    {
        printf("core[%u]: freq[%f] reg[%08x] proc[%2u~%2u]\n", core.core_id, core.frequency / 10e9, 
#if COMMON_ARCH_X86
            core.cpuid,
#elif COMMON_ARCH_ARM
            core.midr,
#endif
            core.processor_start, core.processor_start + core.processor_count);
    }
    printf("\n");
    span<const cpuinfo_processor> procs{ cpuinfo_get_processors(), cpuinfo_get_processors_count() };
    for (const auto& proc : procs)
    {
        printf("proc[%u]: "
#if COMMON_OS_WIN
            "gid[%u] pid[%u] "
#else
            "sysid[%u] "
#endif
#if COMMON_ARCH_X86
            "apic[%u] "
#endif
            "\n", proc.smt_id
#if COMMON_OS_WIN
            , proc.windows_group_id, proc.windows_processor_id
#else
            , proc.linux_id
#endif
#if COMMON_ARCH_X86
            , proc.apic_id
#endif
        );
        printf("\n");
        constexpr auto PrintCache = [](const char* name, const cpuinfo_cache* theCache)
        {
            if (!theCache) return;
            const auto& cache = *theCache;
            printf("--%s [%u]B [%2uWay%2uSet] [%2uB Line] [%08x] in proc[%2u~%2u]\n", name, cache.size, cache.associativity, cache.sets,
                cache.line_size, cache.flags, cache.processor_start, cache.processor_start + cache.processor_count);
        };
        PrintCache("L1I", proc.cache.l1i);
        PrintCache("L1D", proc.cache.l1d);
        PrintCache("L2", proc.cache.l2);
        PrintCache("L3", proc.cache.l3);
        PrintCache("L4", proc.cache.l4);
        printf("\n");
    }
}


std::string ThreadAffinity::ToString(std::pair<char, char> ch) const noexcept
{
    std::string ret;
    const auto& info = TopologyInfo::Get();
    for (uint32_t i = 0; i < info.GetGroupCount(); ++i)
    {
        ret.push_back('[');
        for (uint32_t j = 0; j < info.GetCountInGroup(i); ++j)
            ret.push_back(Get(i, j) ? ch.first : ch.second);
        ret.push_back(']');
    }
    return ret;
}


bool ThreadObject::CompareHandle(uintptr_t lhs, uintptr_t rhs) noexcept
{
#if COMMON_OS_WIN
    return ::CompareObjectHandles((HANDLE)lhs, (HANDLE)rhs) == TRUE;
#else
    return pthread_equal((pthread_t)lhs, (pthread_t)rhs) != 0;
#endif
}

#if COMMON_OS_WIN
forceinline static uintptr_t CopyThreadHandle(void* src)
{
    HANDLE Handle = nullptr;
    DuplicateHandle(GetCurrentProcess(), (HANDLE)src, GetCurrentProcess(), &Handle,
        SYNCHRONIZE | THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION | THREAD_SET_INFORMATION | THREAD_SET_LIMITED_INFORMATION, false, 0);
    return reinterpret_cast<uintptr_t>(Handle);
}
#elif !COMMON_OS_ANDROID && !COMMON_OS_DARWIN
class ThreadIdMap
{
private:
    std::map<uintptr_t, uint64_t> IdMap;
    pthread_key_t Key = {};
    RWSpinLock Locker;
    static ThreadIdMap& Get() noexcept
    {
        static ThreadIdMap Host;
        return Host;
    }
    static void OnThreadExit(void* handle) noexcept
    {
        auto& self = Get();
        const auto key = reinterpret_cast<uintptr_t>(handle);
        //printf("~~Exit and remove [%08" PRIx64 "]\n", static_cast<uint64_t>(key));
        const auto locker = self.Locker.WriteScope();
        self.IdMap.erase(key);
    }
    ThreadIdMap() noexcept
    {
        pthread_key_create(&Key, OnThreadExit);
    }
public:
    static uint64_t Register(uint64_t tid) noexcept
    {
        auto& self = Get();
        if (pthread_getspecific(self.Key) == nullptr)
        {
            const uintptr_t key = pthread_self();
            pthread_setspecific(self.Key, reinterpret_cast<void*>(key));
            //printf("~~Register [%08" PRIx64 "] -> [%" PRIu64 "]\n", static_cast<uint64_t>(key), tid);
            {
                const auto locker = self.Locker.WriteScope();
                self.IdMap.insert_or_assign(key, tid);
            }
        }
        return tid;
    }
    static uint64_t QueryId(uintptr_t key) noexcept
    {
        auto& self = Get();
        const auto locker = self.Locker.ReadScope();
        const auto it = self.IdMap.find(key);
        return it != self.IdMap.end() ? it->second : 0u;
    }
};
#endif

uint64_t ThreadObject::GetCurrentThreadId()
{
#if COMMON_OS_WIN
    return ::GetCurrentThreadId();
#elif COMMON_OS_DARWIN
    uint64_t tid = 0;
    pthread_threadid_np(nullptr, &tid);
    return tid;
#elif COMMON_OS_ANDROID
    return pthread_gettid_np(pthread_self()); // introduced in SDK Level 21
#else
#   if defined(COMMON_GLIBC_VER) && COMMON_GLIBC_VER >= 230
    const auto& info = ThreadExHost::Get();
    if (info.GetCurrentTid)
        return ThreadIdMap::Register(info.GetCurrentTid());
#   endif
    return ThreadIdMap::Register(syscall(SYS_gettid));
#endif
    
}

template<bool NeedOwnership = true>
uintptr_t RetriveCurrentThread() noexcept
{
#if COMMON_OS_WIN
    if constexpr (NeedOwnership)
        return CopyThreadHandle(GetCurrentThread());
    else
        return reinterpret_cast<uintptr_t>(GetCurrentThread());
#elif COMMON_OS_DARWIN
    return reinterpret_cast<uintptr_t>(pthread_self());
#else
    const auto handle = pthread_self();
#   if !COMMON_OS_ANDROID
    ThreadObject::GetCurrentThreadId(); // force register it
#   endif
    return handle;
#endif
}

static uint64_t GetThreadMappingId(uintptr_t handle)
{
#if COMMON_OS_WIN
    return ::GetThreadId((HANDLE)handle);
#elif COMMON_OS_DARWIN
    uint64_t tid = 0;
    pthread_threadid_np((pthread_t)handle, &tid);
    return tid;
#elif COMMON_OS_ANDROID
    return pthread_gettid_np((pthread_t)handle); // introduced in SDK Level 21
#else
    return ThreadIdMap::QueryId(handle);
#endif
}

ThreadObject::~ThreadObject()
{
#if COMMON_OS_WIN
    if (Handle != 0)
        ::CloseHandle((HANDLE)Handle);
#endif
}

ThreadObject ThreadObject::Duplicate() const
{
#if COMMON_OS_WIN
    return { CopyThreadHandle(reinterpret_cast<void*>(Handle)), TId };
#else
    return { Handle, TId };
#endif
}

ThreadObject ThreadObject::GetCurrentThreadObject()
{
    const auto handle = RetriveCurrentThread();
    const auto tid = ThreadObject::GetCurrentThreadId();
    //printf("get thread [%08" PRIx64 "] tid [%" PRIu64 "]\n", handle, tid);
    return { handle, tid };
}

ThreadObject ThreadObject::GetThreadObject(std::thread& thr)
{
    const auto& handle = thr.native_handle();
#if COMMON_OS_WIN
    return { CopyThreadHandle(handle), GetThreadMappingId(reinterpret_cast<uintptr_t>(handle)) };
#elif COMMON_OS_DARWIN
    const auto handle_ = reinterpret_cast<uintptr_t>(handle);
    return { handle_, GetThreadMappingId(handle_) };
#else
    const auto handle_ = static_cast<uintptr_t>(handle);
    return { handle_, GetThreadMappingId(handle_) };
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
    // return TId == ThreadObject::GetCurrentThreadId();
    return CompareHandle(RetriveCurrentThread<false>(), Handle);
}

#if COMMON_OS_UNIX
static void GetThreadName(pthread_t handle, [[maybe_unused]] const ThreadObject& self, char(&buf)[16]) noexcept
{
# if COMMON_OS_ANDROID
    const auto& info = ThreadExHost::Get();
    if (info.GetThreadName)
        info.GetThreadName(handle, buf, sizeof(buf));
    else if (self.IsCurrent())
#   ifndef PR_GET_NAME
#     define PR_GET_NAME 16
#   endif
        prctl(PR_GET_NAME, reinterpret_cast<unsigned long>(buf), 0, 0, 0);
# else
    pthread_getname_np(handle, buf, sizeof(buf));
# endif
}
#endif
std::u16string ThreadObject::GetName() const
{
#if COMMON_OS_WIN
    // introduced in Win10 1607
    PWSTR ret;
    ::GetThreadDescription((HANDLE)Handle, &ret);
    return std::u16string(reinterpret_cast<const char16_t*>(ret));
#else
    char tmp[16] = { 0 };
    GetThreadName((pthread_t)Handle, *this, tmp);
    return str::to_u16string(&tmp[0], str::Encoding::UTF8);
#endif
}

bool ThreadObject::SetName(const std::u16string_view name) const
{
#if COMMON_OS_WIN
    // introduced in Win10 1607
    std::u16string name_(name);
    const HResultHolder ret = ::SetThreadDescription((HANDLE)Handle, (PCWSTR)name_.data());
    if (!ret)
    {
        std::string msg = "In [SetThreadDescription]: " + str::to_string(ret.ToStr());
        detail::InitMessage::Enqueue(mlog::LogLevel::Error, "ThreadObject", msg);
    }
    return static_cast<bool>(ret);
#else
    auto name_ = str::to_u8string(name, str::Encoding::UTF8);
    if (name_.size() > 15) // TODO: need safe trunc
        name_.resize(15);
    const auto buf = reinterpret_cast<const char*>(name_.c_str());
#   if COMMON_OS_DARWIN
    if (!IsCurrent()) return false;
    return pthread_setname_np(buf) == 0;
#   else
    return pthread_setname_np((pthread_t)Handle, buf) == 0;
#   endif
#endif
}

std::optional<ThreadAffinity> ThreadObject::GetAffinity() const
{
    const auto& info = ThreadExHost::Get();
    const auto groupCount = info.GetGroupCount();
    ThreadAffinity affinity;
#if COMMON_OS_WIN
    const auto bytePerGroup = info.BitsPerGroup / 8;
    const auto CopyToAffinity = [&](const GROUP_AFFINITY& mask)
    {
        Expects(mask.Group < groupCount);
        // little endian can direct copy
        memcpy_s(affinity.GetRawData(mask.Group), bytePerGroup, &mask.Mask, sizeof(mask.Mask));
    };
#   if NTDDI_VERSION >= NTDDI_WIN10_FE
    if (info.SupportWin11ThreadGroups())
    {
        std::vector<GROUP_AFFINITY> masks(groupCount);
        USHORT askCount = 0;
        const auto TryFunc = [&](auto&& func, HANDLE handle, std::string_view name)
        {
            auto ret = func(handle, masks.data(), static_cast<USHORT>(groupCount), &askCount);
            if (ret == 0 && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                masks.resize(askCount);
                std::string msg = "In [";
                msg.append(name).append("]: group request[").append(std::to_string(askCount)).append("] more than [").append(std::to_string(groupCount));
                detail::InitMessage::Enqueue(mlog::LogLevel::Error, "ThreadObject", msg);
                ret = func(handle, masks.data(), askCount, &askCount);
            }
            if (ret == 0)
            {
                const auto error = Win32ErrorHolder::GetLastError();
                std::string msg = "In [";
                msg.append(name).append("]: ").append(str::to_string(error.ToStr()));
                detail::InitMessage::Enqueue(mlog::LogLevel::Error, "ThreadObject", msg);
                return false;
            }
            return true;
        };
        if (!TryFunc(info.GetThreadSelectedCpuSetMasks, (HANDLE)Handle, "GetThreadSelectedCpuSetMasks"))
            return std::nullopt;
        if (askCount == 0) // not set, try process default
        {
            const auto procHandle = ::GetCurrentProcess();
            if (!TryFunc(info.GetProcessDefaultCpuSetMasks, procHandle, "GetProcessDefaultCpuSetMasks"))
                return std::nullopt;
        }
        if (askCount == 0) // not set, assume all related
        {
            Ensures(affinity.GetCount() == 0);
        }
        else
        {
            for (uint32_t i = 0; i < askCount; ++i)
                CopyToAffinity(masks[i]);
        }
        return affinity;
    }
#   endif
    GROUP_AFFINITY mask{};
    auto ret = GetThreadGroupAffinity((HANDLE)Handle, &mask);
    if (ret == 0)
    {
        const auto error = Win32ErrorHolder::GetLastError();
        std::string msg = "In [GetThreadGroupAffinity]: " + str::to_string(error.ToStr());
        detail::InitMessage::Enqueue(mlog::LogLevel::Error, "ThreadObject", msg);
        return std::nullopt;
    }
    CopyToAffinity(mask);
    return affinity;
#else
    cpu_set_t sets;
    CPU_ZERO(&sets);
#   if COMMON_OS_ANDROID
    constexpr std::string_view funcname = "sched_getaffinity";
    const ErrnoHolder ret = sched_getaffinity(static_cast<pid_t>(TId), sizeof(sets), &sets);
#   else
    constexpr std::string_view funcname = "pthread_getaffinity_np";
    const ErrnoHolder ret = pthread_getaffinity_np((pthread_t)Handle, sizeof(sets), &sets);
#   endif
    if (!ret)
    {
        std::string msg = "In [";
        msg.append(funcname).append("]: ").append(str::to_string(ret.ToStr()));
        detail::InitMessage::Enqueue(mlog::LogLevel::Error, "ThreadObject", msg);
        return std::nullopt;
    }
    Expects(groupCount == 1);
    for (uint32_t i = 0; i < info.GetCountInGroup(0); ++i)
        affinity.Set(0, i, CPU_ISSET(i, &sets));
    return affinity;
#endif
}

bool ThreadObject::SetAffinity(const ThreadAffinity& affinity) const
{
    const auto& info = ThreadExHost::Get();
    const auto groupCount = info.GetGroupCount();
#if COMMON_OS_WIN
    const auto bytePerGroup = info.BitsPerGroup / 8;
    const auto group = info.DecideSingleGroup(affinity);
#   if NTDDI_VERSION >= NTDDI_WIN10_FE
    if (info.SupportWin11ThreadGroups())
    {
        const auto TryFunc = [&](auto&& func, span<GROUP_AFFINITY> masks, std::string_view name)
        {
            auto ret = func((HANDLE)Handle, masks.data(), static_cast<USHORT>(masks.size()));
            if (ret == 0)
            {
                const auto error = Win32ErrorHolder::GetLastError();
                std::string msg = "In [";
                msg.append(name).append("]: ").append(str::to_string(error.ToStr()));
                detail::InitMessage::Enqueue(mlog::LogLevel::Error, "ThreadObject", msg);
                return false;
            }
            return true;
        };
        bool success = false;
        if (affinity.GetCount() > 0)
        {
            std::vector<GROUP_AFFINITY> masks(groupCount);
            for (uint32_t i = 0; i < groupCount; ++i)
            {
                masks[i].Group = static_cast<WORD>(i);
                memcpy_s(&masks[i].Mask, sizeof(masks[i].Mask), affinity.GetRawData(i), bytePerGroup);
            }
            success = TryFunc(info.SetThreadSelectedCpuSetMasks, masks, "SetThreadSelectedCpuSetMasks");
        }
        else
            success = TryFunc(info.SetThreadSelectedCpuSetMasks, {}, "SetThreadSelectedCpuSetMasks");
        if (!group) return success;
    }
#   endif
    Ensures(group); // should always get a decision
    GROUP_AFFINITY mask{};
    mask.Group = static_cast<WORD>(group->first);
    if (group->second == 0)
        memset(&mask.Mask, 0xff, sizeof(mask.Mask));
    else
        memcpy_s(&mask.Mask, sizeof(mask.Mask), affinity.GetRawData(group->first), bytePerGroup);
    auto ret = SetThreadGroupAffinity((HANDLE)Handle, &mask, nullptr);
    if (ret == 0)
    {
        const auto error = Win32ErrorHolder::GetLastError();
        std::string msg = "In [SetThreadGroupAffinity]: " + str::to_string(error.ToStr());
        detail::InitMessage::Enqueue(mlog::LogLevel::Error, "ThreadObject", msg);
        return false;
    }
    return true;
#else
    cpu_set_t sets;
    CPU_ZERO(&sets);
    Expects(groupCount == 1);
    for (uint32_t i = 0; i < info.GetCountInGroup(0); ++i)
    {
        if (affinity.Get(0, i))
            CPU_SET(i, &sets);
    }
#   if COMMON_OS_ANDROID
    constexpr std::string_view funcname = "sched_setaffinity";
    const ErrnoHolder ret = sched_setaffinity(static_cast<pid_t>(TId), sizeof(sets), &sets);
#   else
    constexpr std::string_view funcname = "pthread_setaffinity_np";
    const ErrnoHolder ret = pthread_setaffinity_np((pthread_t)Handle, sizeof(sets), &sets);
#   endif
    if (!ret)
    {
        std::string msg = "In [";
        msg.append(funcname).append("]: ").append(str::to_string(ret.ToStr()));
        detail::InitMessage::Enqueue(mlog::LogLevel::Error, "ThreadObject", msg);
        return false;
    }
    return true;
#endif
}

std::optional<ThreadQoS> ThreadObject::GetQoS() const
{
    std::optional<ThreadQoS> actualQos;
#if COMMON_OS_WIN
    // no way to query
#elif COMMON_OS_DARWIN
    qos_class_t val = QOS_CLASS_DEFAULT;
    if (pthread_get_qos_class_np(reinterpret_cast<pthread_t>(Handle), &val, nullptr) == 0)
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

std::optional<ThreadQoS> ThreadObject::SetQoS(ThreadQoS qos) const
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
        powerThrottling.StateMask = 0;
        actualQos = ThreadQoS::High;
        break;
    case ThreadQoS::Utility:
    case ThreadQoS::Background:
        powerThrottling.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        powerThrottling.StateMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        actualQos = ThreadQoS::Background;
        break;
    default:
    case ThreadQoS::Default:
        powerThrottling.ControlMask = 0;
        powerThrottling.StateMask = 0;
        actualQos = ThreadQoS::Default;
        break;
    }
    const auto ret = SetThreadInformation((HANDLE)Handle, ThreadPowerThrottling, &powerThrottling, sizeof(powerThrottling));
    if (ret == 0)
    {
        const auto error = Win32ErrorHolder::GetLastError();
        std::string msg = "In [SetThreadInformation]: " + str::to_string(error.ToStr());
        detail::InitMessage::Enqueue(mlog::LogLevel::Error, "ThreadObject", msg);
        return {};
    }
#elif COMMON_OS_DARWIN
    if (IsCurrent())
    {
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
    }
#else
#endif
    return actualQos;
}


static void AssignMask(ThreadAffinity& affinity, uint32_t procId)
{
    const auto& proc = *cpuinfo_get_processor(procId);
#if COMMON_OS_WIN
    affinity.Set(proc.windows_group_id, proc.windows_processor_id, true);
#else
    affinity.Set(proc.linux_id, true);
#endif
}
static void AssignPartition(CPUPartition& partition, span<const uint32_t> procIds)
{
    for (const auto procId : procIds)
        AssignMask(partition.Affinity, procId);
}
static void AssignPartition(CPUPartition& partition, uint32_t idStart, uint32_t count)
{
    while (count--)
        AssignMask(partition.Affinity, idStart++);
}
static void BuildPartition(std::vector<CPUPartition>& partitions) noexcept
{
    auto thisThread = ThreadObject::GetCurrentThreadObject();
    char tmp[128] = { '\0' };
    uint32_t clusterIdx = 0;
    for (const auto& cluster : span<const cpuinfo_cluster>{ cpuinfo_get_clusters(), cpuinfo_get_clusters_count() })
    {
        const auto pkg = *cluster.package;
        CPUPartition part{ pkg.name };
        snprintf(tmp, sizeof(tmp), "cluster[%u]", clusterIdx++);
        part.PartitionName = tmp;
#if COMMON_ARCH_X86
        // try to identify hybrid arch
        const auto GetHybridType = [&](uint32_t procId) -> std::optional<std::pair<uint32_t, bool>>
        {
            ThreadAffinity affinity;
            AssignMask(affinity, procId);
            thisThread.SetAffinity(affinity);
            if (const uint32_t max_base_index = cpuid(0).eax; max_base_index >= 0x1au)
            {
                const auto reg = cpuidex(0x1au, 0);
                if (reg.eax != 0) // it's hybrid
                {
                    // LP-E has no L3, see https://community.intel.com/t5/Processors/Detecting-LP-E-Cores-on-Meteor-Lake-in-software/m-p/1584555/highlight/true#M70732
                    bool hasL3 = false;
                    for (uint32_t i = 0; i < UINT32_MAX; ++i)
                    {
                        const auto cacheReg = cpuidex(0x4u, i);
                        const auto cacheType = cacheReg.eax & 0x1fu;
                        const auto cacheLevel = (cacheReg.eax & 0xe0u) >> 5;
                        if (cacheType == 0)
                            break;
                        if (cacheType == 3 /*unifed cache*/ && cacheLevel == 3)
                        {
                            hasL3 = true;
                            break;
                        }
                    }
                    return std::pair{ reg.eax, hasL3 };
                }
            }
            return {};
        };
        const auto type = GetHybridType(cluster.processor_start);
        if (type.has_value())
        {
            std::map<std::pair<uint32_t, bool>, std::vector<uint32_t>> typeMap;
            typeMap[*type].push_back(cluster.processor_start);
            for (uint32_t i = 1; i < cluster.processor_count; ++i)
            {
                const auto id = cluster.processor_start + i;
                const auto type_ = GetHybridType(id);
                typeMap[type_.value_or(std::pair{0u, false})].push_back(id);
            }
            std::vector<const std::pair<const std::pair<uint32_t, bool>, std::vector<uint32_t>>*> list;
            for (const auto& p : typeMap)
                list.push_back(&p);
            // sequential add, sort with first id
            std::sort(list.begin(), list.end(), [](const auto lhs, const auto rhs) { return lhs->second.front() < rhs->second.front(); });
            for (const auto p : list)
            {
                const auto [key, hasL3] = p->first;
                CPUPartition part_ = part;
                AssignPartition(part_, p->second);
                std::string_view coreType;
                switch (key >> 24)
                {
                case 0x20u: coreType = hasL3 ? "Intel Atom E-Core" : "Intel Atom LP E-Core"; break;
                case 0x40u: coreType = "Intel Core P-Core"; break;
                default: break;
                }
                snprintf(tmp, sizeof(tmp), " %s %08x", coreType.empty() ? "" : coreType.data(), key);
                part_.PartitionName += tmp;
                partitions.push_back(part_);
            }
        }
        else
        {
            AssignPartition(part, cluster.processor_start, cluster.processor_count);
            partitions.push_back(part);
        }
#else
        AssignPartition(part, cluster.processor_start, cluster.processor_count);
        partitions.push_back(part);
#endif
    }
}
span<const CPUPartition> CPUPartition::GetPartitions() noexcept
{
    static const std::vector<CPUPartition> Partitions = []()
    {
        std::vector<CPUPartition> partitions;
        std::thread(BuildPartition, std::ref(partitions)).join();
        return partitions;
    }();
    return Partitions;
}


struct CPUFeature
{
    std::vector<std::string_view> FeatureText;
    container::FrozenDenseStringSetSimple<char> FeatureLookup;
    void AppendFeature(std::string_view txt) noexcept
    {
        for (const auto& feat : FeatureText)
        {
            if (feat == txt) return;
        }
        FeatureText.push_back(txt);
    }
    void TryCPUInfo() noexcept
    {
        EnsureCPUInfoInited();
        detail::InitMessage::Enqueue(mlog::LogLevel::Verbose, "cpuinfo", std::string("detected cpu: ") + cpuinfo_get_package(0)->name + "\n");
#if COMMON_ARCH_X86
        const uint32_t max_base_index = cpuid(0).eax;
        if (max_base_index >= 7)
        {
            const auto reg70 = cpuidex(7, 0);
            if (reg70.ecx & (1u << 5))
                AppendFeature("waitpkg"sv);
        }
# define CHECK_FEATURE(en, name) if (cpuinfo_has_x86_##en()) AppendFeature(#name""sv)
        CHECK_FEATURE(sse,              sse);
        CHECK_FEATURE(sse2,             sse2);
        CHECK_FEATURE(sse3,             sse3);
        CHECK_FEATURE(ssse3,            ssse3);
        CHECK_FEATURE(sse4_1,           sse4_1);
        CHECK_FEATURE(sse4_2,           sse4_2);
        CHECK_FEATURE(avx,              avx);
        CHECK_FEATURE(fma3,             fma);
        CHECK_FEATURE(avx2,             avx2);
        CHECK_FEATURE(avx512f,          avx512f);
        CHECK_FEATURE(avx512dq,         avx512dq);
        CHECK_FEATURE(avx512pf,         avx512pf);
        CHECK_FEATURE(avx512er,         avx512er);
        CHECK_FEATURE(avx512cd,         avx512cd);
        CHECK_FEATURE(avx512bw,         avx512bw);
        CHECK_FEATURE(avx512vl,         avx512vl);
        CHECK_FEATURE(avx512vnni,       avx512vnni);
        CHECK_FEATURE(avx512vbmi,       avx512vbmi);
        CHECK_FEATURE(avx512vbmi2,      avx512vbmi2);
        CHECK_FEATURE(avx512bitalg,     avx512bitalg);
        CHECK_FEATURE(avx512vpopcntdq,  avx512vpopcntdq);
        CHECK_FEATURE(movbe,            movbe);
        CHECK_FEATURE(pclmulqdq,        pclmul);
        CHECK_FEATURE(popcnt,           popcnt);
        CHECK_FEATURE(aes,              aes);
        CHECK_FEATURE(lzcnt,            lzcnt);
        CHECK_FEATURE(f16c,             f16c);
        CHECK_FEATURE(bmi,              bmi1);
        CHECK_FEATURE(bmi2,             bmi2);
        CHECK_FEATURE(sha,              sha);
        CHECK_FEATURE(adx,              adx);
# undef CHECK_FEATURE
#elif COMMON_ARCH_ARM
        cpuinfo_has_arm_sha2();
# define CHECK_FEATURE(en, name) if (cpuinfo_has_arm_##en()) AppendFeature(#name""sv)
        CHECK_FEATURE(neon,         asimd);
        CHECK_FEATURE(aes,          aes);
        CHECK_FEATURE(pmull,        pmull);
        CHECK_FEATURE(sha1,         sha1);
        CHECK_FEATURE(sha2,         sha2);
        CHECK_FEATURE(crc32,        crc32);
# undef CHECK_FEATURE
#endif
        return;
    }
    void TryAUXVal() noexcept
    {
#if COMMON_OS_LINUX && COMMON_ARCH_ARM && (!COMMON_OS_ANDROID || __NDK_MAJOR__ >= 18)
        [[maybe_unused]] const auto cap1 = getauxval(AT_HWCAP), cap2 = getauxval(AT_HWCAP2);
        //printf("cap1&2: [%lx][%lx]\n", cap1, cap2);
# define PFX1(en) PPCAT(HWCAP_,  en)
# define PFX2(en) PPCAT(HWCAP2_, en)
# define CHECK_FEATURE(n, en, name) if (PPCAT(cap, n) & PPCAT(PFX, n)(en)) AppendFeature(#name""sv)
# if COMMON_ARCH_ARM
#   if defined(__aarch64__)
#     define CAPNUM 1
        CHECK_FEATURE(1, ASIMD, asimd);
        CHECK_FEATURE(CAPNUM, FP,       fp);
#   else
#     define CAPNUM 2
        CHECK_FEATURE(1, NEON, asimd);
#   endif
        CHECK_FEATURE(CAPNUM, AES,      aes);
        CHECK_FEATURE(CAPNUM, PMULL,    pmull);
        CHECK_FEATURE(CAPNUM, SHA1,     sha1);
        CHECK_FEATURE(CAPNUM, SHA2,     sha2);
        CHECK_FEATURE(CAPNUM, CRC32,    crc32);
# undef CAPNUM
# endif
# undef CHECK_FEATURE
# undef PFX1
# undef PFX2
#endif
    }
    void TrySysCtl() noexcept
    {
#if COMMON_OS_DARWIN && COMMON_ARCH_ARM
        constexpr auto SysCtl = [](const std::string_view name) -> bool 
        {
            char buf[100] = { 0 };
            size_t len = 100;
            if (sysctlbyname(name.data(), &buf, &len, nullptr, 0)) return false;
            return len >= 1 && buf[0] == 1;
        };
# define CHECK_FEATURE(name, feat) if (SysCtl("hw.optional."#name""sv)) AppendFeature(#feat""sv)
        CHECK_FEATURE(floatingpoint, fp);
#   if COMMON_OSBIT == 64
        AppendFeature("asimd"sv);
        AppendFeature("aes"sv);
        AppendFeature("pmull"sv);
        AppendFeature("sha1"sv);
        AppendFeature("sha2"sv);
#   else
        CHECK_FEATURE(neon, asimd);
#   endif
        CHECK_FEATURE(armv8_crc32, crc32);
        CHECK_FEATURE(armv8_2_sha512, sha512);
        CHECK_FEATURE(armv8_2_sha3, sha3);
# undef CHECK_FEATURE
#endif
    }

    CPUFeature() noexcept
    {
        TryCPUInfo();
        TryAUXVal();
        TrySysCtl();
        FeatureLookup = FeatureText;
    }
};
[[nodiscard]] static const CPUFeature& GetCPUFeatureHost() noexcept
{
    static const CPUFeature Host;
    return Host;
}


bool CheckCPUFeature(str::HashedStrView<char> feature) noexcept
{
    static const auto& features = GetCPUFeatureHost();
    return features.FeatureLookup.Has(feature);
}
span<const std::string_view> GetCPUFeatures() noexcept
{
    static const auto& features = GetCPUFeatureHost();
    return features.FeatureText;
}




}