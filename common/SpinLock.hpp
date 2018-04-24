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


struct SpinLocker : public NonCopyable
{
private:
    std::atomic_flag& lock;
public:
    SpinLocker(std::atomic_flag& flag) : lock(flag)
    {
        Lock(lock);
    }
    ~SpinLocker()
    {
        Unlock(lock);
    }
    static bool TryLock(std::atomic_flag& flag)
    {
        return !flag.test_and_set(/*std::memory_order_acquire*/);
    }
    static void Lock(std::atomic_flag& flag)
    {
        for (uint32_t i = 0; flag.test_and_set(/*std::memory_order_acquire*/); ++i)
        {
            if (i > 16)
                COMMON_PAUSE();
        }
    }
    static void Unlock(std::atomic_flag& flag)
    {
        flag.clear();
    }
};

struct PreferSpinLock : public NonCopyable, public NonMovable //Strong-first
{
private:
    std::atomic<uint32_t> Flag { 0 }; //strong on half 16bit, weak on lower 16bit
public:
    void LockWeak()
    {
        uint32_t expected = Flag.load() & 0x0000ffff; //assume no writer
        while (!Flag.compare_exchange_strong(expected, expected + 1))
        {
            expected &= 0x0000ffff; //assume no writer
            COMMON_PAUSE();
        }
    }
    void UnlockWeak()
    {
        Flag--;
    }
    void LockStrong()
    {
        Flag.fetch_add(0x00010000);
        while((Flag.load() & 0x0000ffff) != 0) //loop until no reader
        {
            COMMON_PAUSE();
        }
    }
    void UnlockStrong()
    {
        Flag.fetch_sub(0x00010000);
    }
};

struct WRSpinLock : public NonCopyable, public NonMovable //Writer-first
{
private:
    std::atomic<uint32_t> Flag { 0 }; //writer on most siginificant bit, reader on lower bits
public:
    void LockRead()
    {
        uint32_t expected = Flag.load() & 0x7fffffff; //assume no writer
        while (!Flag.compare_exchange_weak(expected, expected + 1))
        {
            expected &= 0x7fffffff; //assume no writer
            COMMON_PAUSE();
        }
    }
    void UnlockRead()
    {
        Flag--;
    }
    void LockWrite()
    {
        uint32_t expected = Flag.load() & 0x7fffffff;
        while (!Flag.compare_exchange_weak(expected, expected + 0x80000000))
        {
            expected &= 0x7fffffff; //assume no other writer
        }
        while ((Flag.load() & 0x7fffffff) != 0) //loop until no reader
        {
            COMMON_PAUSE();
        }
    }
    void UnlockWrite()
    {
        uint32_t expected = Flag.load() | 0x80000000;
        while (!Flag.compare_exchange_weak(expected, expected - 0x80000000))
        {
            expected |= 0x80000000; //ensure there's a writer
        }
    }
};

struct RWSpinLock : public NonCopyable, public NonMovable //Reader-first
{
private:
    std::atomic<uint32_t> Flag { 0 }; //writer on most siginificant bit, reader on lower bits
public:
    void LockRead()
    {
        Flag++;
        while ((Flag.load() & 0x80000000) != 0) //loop until no writer
        {
            COMMON_PAUSE();
        }
    }
    void UnlockRead()
    {
        Flag--;
    }
    void LockWrite()
    {
        uint32_t expected = 0;
        while (!Flag.compare_exchange_weak(expected, 0x80000000))
        {
            expected = 0; //assume no other locker
            COMMON_PAUSE();
        }
    }
    void UnlockWrite()
    {
        uint32_t expected = Flag.load() | 0x80000000;
        while (!Flag.compare_exchange_weak(expected, expected & 0x7fffffff))
        {
            expected |= 0x80000000; //ensure there's a writer
        }
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
    void DowngradeToRead()
    {
        uint32_t expected = (Flag.load() | 0x80000000) + 1;
        while (!Flag.compare_exchange_weak(expected, expected & 0x7fffffff))
        {
            expected = (expected | 0x80000000) + 1; //ensure there's a writer
        }
    }
};

}
