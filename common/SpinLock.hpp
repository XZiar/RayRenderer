#pragma once

#include "CommonRely.hpp"
#include <atomic>

#if COMMON_ARCH_X86
#   include "simd/SIMD.hpp"
#   if COMMON_SIMD_LV >= 20
#       define COMMON_PAUSE() _mm_pause()
#   elif (COMMON_COMPILER_CLANG && COMMON_CLANG_VER >= 30800) || (COMMON_COMPILER_GCC && COMMON_GCC_VER >= 40701)
#       define COMMON_PAUSE() __builtin_ia32_pause()
#   elif COMMON_COMPILER_MSVC
#       define COMMON_PAUSE() __nop()
#   else
#       define COMMON_PAUSE() asm volatile ("pause")
#   endif
#elif COMMON_ARCH_ARM
#   if COMMON_OSBIT == 64
#       if COMMON_COMPILER_MSVC
#           include <arm64intr.h>
#           define COMMON_PAUSE() __isb(_ARM64_BARRIER_SY)
#       elif defined(__ARM_ACLE)
#           include <arm_acle.h>
#           define COMMON_PAUSE() __isb(0xf)
#       else
#           define COMMON_PAUSE() asm volatile ("yield")
#       endif
#   else
#       if COMMON_COMPILER_MSVC
#           define COMMON_PAUSE() __yield()
#       else
#           define COMMON_PAUSE() asm volatile ("yield")
#       endif
#   endif
#else
#   define COMMON_PAUSE() do{} while(0)
#endif

namespace common
{
namespace detail
{
template<typename T, void(T::*Lock)(), void(T::*Unlock)()>
struct [[nodiscard]] LockScope
{
private:
    T* Locker;
public:
    constexpr LockScope(T* locker) noexcept : Locker(locker)
    {
        (Locker->*Lock)();
    }
    constexpr LockScope(LockScope&& locker) noexcept : Locker(locker.Locker)
    {
        locker.Locker = nullptr;
    }
    constexpr LockScope(const LockScope&) noexcept = delete;
    ~LockScope() noexcept
    {
        if (Locker)
            (Locker->*Unlock)();
    }
};
}


struct SpinLocker
{
protected:
    std::atomic_flag Flag = ATOMIC_FLAG_INIT;
public:
    constexpr SpinLocker() noexcept { }
    COMMON_NO_COPY(SpinLocker)
    COMMON_NO_MOVE(SpinLocker)
    bool TryLock() noexcept
    {
        return !Flag.test_and_set();
    }
    void Lock() noexcept
    {
        while (Flag.test_and_set())
        {
            COMMON_PAUSE();
        }
    }
    void Unlock() noexcept
    {
        Flag.clear();
    }
    using ScopeType = detail::LockScope<SpinLocker, &SpinLocker::Lock, &SpinLocker::Unlock>;
    ScopeType LockScope() noexcept
    {
        return ScopeType(this);
    }
};


struct PreferSpinLock //Strong-first
{
protected:
    std::atomic<uint32_t> Flag; //strong on high 16bit, weak on low 16bit
public:
    constexpr PreferSpinLock() noexcept : Flag(0) { }
    explicit PreferSpinLock(PreferSpinLock&& other) noexcept 
        : Flag(other.Flag.exchange(0)) { }
    COMMON_NO_COPY(PreferSpinLock)
    void LockWeak() noexcept
    {
        uint32_t expected = Flag.load() & 0x0000ffffu; //assume no strong
        while (!Flag.compare_exchange_strong(expected, expected + 1))
        {
            expected &= 0x0000ffffu; //assume no strong
            COMMON_PAUSE();
        }
    }
    void UnlockWeak() noexcept
    {
        Flag--;
    }
    void LockStrong() noexcept
    {
        Flag.fetch_add(0x00010000u);
        while((Flag.load() & 0x0000ffffu) != 0) //loop until no weak
        {
            COMMON_PAUSE();
        }
    }
    void UnlockStrong() noexcept
    {
        Flag.fetch_sub(0x00010000u);
    }
    using WeakScopeType = detail::LockScope<PreferSpinLock, &PreferSpinLock::LockWeak, &PreferSpinLock::UnlockWeak>;
    WeakScopeType WeakScope() noexcept
    {
        return WeakScopeType(this);
    }
    using StrongScopeType = detail::LockScope<PreferSpinLock, &PreferSpinLock::LockStrong, &PreferSpinLock::UnlockStrong>;
    StrongScopeType StrongScope() noexcept
    {
        return StrongScopeType(this);
    }
};

struct WRSpinLock //Writer-first
{
protected:
    std::atomic<uint32_t> Flag; //writer on most siginificant bit, reader on lower bits
public:
    constexpr WRSpinLock() noexcept : Flag(0) { }
    explicit WRSpinLock(WRSpinLock&& other) noexcept 
        : Flag(other.Flag.exchange(0)) { }
    COMMON_NO_COPY(WRSpinLock)
    void LockRead() noexcept
    {
        uint32_t expected = Flag.load() & 0x7fffffffu; //assume no writer
        while (!Flag.compare_exchange_weak(expected, expected + 1))
        {
            expected &= 0x7fffffffu; //assume no writer
            COMMON_PAUSE();
        }
    }
    void UnlockRead() noexcept
    {
        Flag--;
    }
    void LockWrite() noexcept
    {
        uint32_t expected = Flag.load() & 0x7fffffffu;
        while (!Flag.compare_exchange_weak(expected, expected + 0x80000000u))
        {
            expected &= 0x7fffffffu; //assume no other writer
            COMMON_PAUSE();
        }
        while ((Flag.load() & 0x7fffffffu) != 0) //loop until no reader
        {
            COMMON_PAUSE();
        }
    }
    void UnlockWrite() noexcept
    {
        Flag -= 0x80000000u;
        //uint32_t expected = Flag.load() | 0x80000000;
        //while (!Flag.compare_exchange_weak(expected, expected - 0x80000000))
        //{
        //    expected |= 0x80000000; //ensure there's a writer
        //}
    }
    using ReadScopeType = detail::LockScope<WRSpinLock, &WRSpinLock::LockRead, &WRSpinLock::UnlockRead>;
    ReadScopeType ReadScope() noexcept
    {
        return ReadScopeType(this);
    }
    using WriteScopeType = detail::LockScope<WRSpinLock, &WRSpinLock::LockWrite, &WRSpinLock::UnlockWrite>;
    WriteScopeType WriteScope() noexcept
    {
        return WriteScopeType(this);
    }
};

struct RWSpinLock //Reader-first
{
protected:
    std::atomic<uint32_t> Flag; //writer on most siginificant bit, reader on lower bits
public:
    constexpr RWSpinLock() : Flag(0) { }
    explicit RWSpinLock(RWSpinLock&& other) noexcept
        : Flag(other.Flag.exchange(0)) { }
    COMMON_NO_COPY(RWSpinLock)
    void LockRead() noexcept
    {
        Flag++;
        while ((Flag.load() & 0x80000000u) != 0) //loop until no writer
        {
            COMMON_PAUSE();
        }
    }
    void UnlockRead() noexcept
    {
        Flag--;
    }
    void LockWrite() noexcept
    {
        uint32_t expected = 0;
        while (!Flag.compare_exchange_weak(expected, 0x80000000u))
        {
            expected = 0; //assume no other locker
            COMMON_PAUSE();
        }
    }
    void UnlockWrite() noexcept
    {
        Flag -= 0x80000000u;
        //uint32_t expected = Flag.load() | 0x80000000;
        //while (!Flag.compare_exchange_weak(expected, expected & 0x7fffffff))
        //{
        //    expected |= 0x80000000; //ensure there's a writer
        //}
    }
    using ReadScopeType = detail::LockScope<RWSpinLock, &RWSpinLock::LockRead, &RWSpinLock::UnlockRead>;
    ReadScopeType ReadScope() noexcept
    {
        return ReadScopeType(this);
    }
    using WriteScopeType = detail::LockScope<RWSpinLock, &RWSpinLock::LockWrite, &RWSpinLock::UnlockWrite>;
    WriteScopeType WriteScope() noexcept
    {
        return WriteScopeType(this);
    }
    //unsuported, may cause deadlock
    //void UpgradeToWrite()
    //{
    //    uint32_t expected = 1;
    //    while (!Flag.compare_exchange_weak(expected, 0x80000000))
    //    {
    //        expected = 1; //assume only self as locker
    //    }
    //}
    void DowngradeToRead() noexcept
    {
        uint32_t expected = (Flag.load() | 0x80000000u) + 1;
        while (!Flag.compare_exchange_weak(expected, expected & 0x7fffffffu))
        {
            expected = (expected | 0x80000000u) + 1; //ensure there's a writer
        }
    }
};

}
