#pragma once

#include "CommonRely.hpp"
#include "SIMD.hpp"
#include <atomic>

#if COMMON_SIMD_LV >= 20
#   define COMMON_PAUSE() _mm_pause()
#elif defined(_WIN32)
#   define COMMON_PAUSE() __nop()
#else
#   define COMMON_PAUSE() __nnop()
#endif

namespace common
{

namespace detail
{
template<typename T, void(T::*Lock)(), void(T::*Unlock)()>
struct LockScope
{
private:
    T& Locker;
public:
    constexpr LockScope(T& locker) noexcept : Locker(locker)
    {
        (Locker.*Lock)();
    }
    ~LockScope() noexcept
    {
        (Locker.*Unlock)();
    }
};
}

struct EmptyLock
{
    constexpr void lock() noexcept {}
    constexpr void unlock() noexcept {}
};

struct SpinLocker : public NonCopyable
{
private:
    std::atomic_flag Flag = ATOMIC_FLAG_INIT;
public:
    constexpr SpinLocker() noexcept { }
    bool TryLock() noexcept
    {
        return !Flag.test_and_set();
    }
    void Lock() noexcept
    {
        for (uint32_t i = 0; Flag.test_and_set(); ++i)
        {
            COMMON_PAUSE();
            //if (i > 16)
        }
    }
    void Unlock() noexcept
    {
        Flag.clear();
    }
    using ScopeType = detail::LockScope<SpinLocker, &SpinLocker::Lock, &SpinLocker::Unlock>;
    ScopeType LockScope() noexcept
    {
        return ScopeType(*this);
    }
};


struct PreferSpinLock : public NonCopyable //Strong-first
{
private:
    std::atomic<uint32_t> Flag; //strong on high 16bit, weak on low 16bit
public:
    constexpr PreferSpinLock() noexcept : Flag(0) { }
    explicit PreferSpinLock(PreferSpinLock&& other) noexcept 
        : Flag(other.Flag.exchange(0)) { }
    void LockWeak() noexcept
    {
        uint32_t expected = Flag.load() & 0x0000ffff; //assume no strong
        while (!Flag.compare_exchange_strong(expected, expected + 1))
        {
            expected &= 0x0000ffff; //assume no strong
            COMMON_PAUSE();
        }
    }
    void UnlockWeak() noexcept
    {
        Flag--;
    }
    void LockStrong() noexcept
    {
        Flag.fetch_add(0x00010000);
        while((Flag.load() & 0x0000ffff) != 0) //loop until no weak
        {
            COMMON_PAUSE();
        }
    }
    void UnlockStrong() noexcept
    {
        Flag.fetch_sub(0x00010000);
    }
    using WeakScopeType = detail::LockScope<PreferSpinLock, &PreferSpinLock::LockWeak, &PreferSpinLock::UnlockWeak>;
    WeakScopeType WeakScope() noexcept
    {
        return WeakScopeType(*this);
    }
    using StrongScopeType = detail::LockScope<PreferSpinLock, &PreferSpinLock::LockStrong, &PreferSpinLock::UnlockStrong>;
    StrongScopeType StrongScope() noexcept
    {
        return StrongScopeType(*this);
    }
};

struct WRSpinLock : public NonCopyable //Writer-first
{
private:
    std::atomic<uint32_t> Flag; //writer on most siginificant bit, reader on lower bits
public:
    constexpr WRSpinLock() noexcept : Flag(0) { }
    explicit WRSpinLock(WRSpinLock&& other) noexcept 
        : Flag(other.Flag.exchange(0)) { }
    void LockRead() noexcept
    {
        uint32_t expected = Flag.load() & 0x7fffffff; //assume no writer
        while (!Flag.compare_exchange_weak(expected, expected + 1))
        {
            expected &= 0x7fffffff; //assume no writer
            COMMON_PAUSE();
        }
    }
    void UnlockRead() noexcept
    {
        Flag--;
    }
    void LockWrite() noexcept
    {
        uint32_t expected = Flag.load() & 0x7fffffff;
        while (!Flag.compare_exchange_weak(expected, expected + 0x80000000))
        {
            expected &= 0x7fffffff; //assume no other writer
            COMMON_PAUSE();
        }
        while ((Flag.load() & 0x7fffffff) != 0) //loop until no reader
        {
            COMMON_PAUSE();
        }
    }
    void UnlockWrite() noexcept
    {
        Flag -= 0x80000000;
        //uint32_t expected = Flag.load() | 0x80000000;
        //while (!Flag.compare_exchange_weak(expected, expected - 0x80000000))
        //{
        //    expected |= 0x80000000; //ensure there's a writer
        //}
    }
    using ReadScopeType = detail::LockScope<WRSpinLock, &WRSpinLock::LockRead, &WRSpinLock::UnlockRead>;
    ReadScopeType ReadScope() noexcept
    {
        return ReadScopeType(*this);
    }
    using WriteScopeType = detail::LockScope<WRSpinLock, &WRSpinLock::LockWrite, &WRSpinLock::UnlockWrite>;
    WriteScopeType WriteScope() noexcept
    {
        return WriteScopeType(*this);
    }
};

struct RWSpinLock : public NonCopyable, public NonMovable //Reader-first
{
private:
    std::atomic<uint32_t> Flag; //writer on most siginificant bit, reader on lower bits
public:
    constexpr RWSpinLock() : Flag(0) { }
    explicit RWSpinLock(RWSpinLock&& other) noexcept
        : Flag(other.Flag.exchange(0)) { }
    void LockRead() noexcept
    {
        Flag++;
        while ((Flag.load() & 0x80000000) != 0) //loop until no writer
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
        while (!Flag.compare_exchange_weak(expected, 0x80000000))
        {
            expected = 0; //assume no other locker
            COMMON_PAUSE();
        }
    }
    void UnlockWrite() noexcept
    {
        Flag -= 0x80000000;
        //uint32_t expected = Flag.load() | 0x80000000;
        //while (!Flag.compare_exchange_weak(expected, expected & 0x7fffffff))
        //{
        //    expected |= 0x80000000; //ensure there's a writer
        //}
    }
    using ReadScopeType = detail::LockScope<RWSpinLock, &RWSpinLock::LockRead, &RWSpinLock::UnlockRead>;
    ReadScopeType ReadScope() noexcept
    {
        return ReadScopeType(*this);
    }
    using WriteScopeType = detail::LockScope<RWSpinLock, &RWSpinLock::LockWrite, &RWSpinLock::UnlockWrite>;
    WriteScopeType WriteScope() noexcept
    {
        return WriteScopeType(*this);
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
        uint32_t expected = (Flag.load() | 0x80000000) + 1;
        while (!Flag.compare_exchange_weak(expected, expected & 0x7fffffff))
        {
            expected = (expected | 0x80000000) + 1; //ensure there's a writer
        }
    }
};

}
