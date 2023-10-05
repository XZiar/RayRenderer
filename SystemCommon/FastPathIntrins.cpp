#include "SystemCommonPch.h"
#include "CopyEx.h"
#include "MiscIntrins.h"
#include "SpinLock.h"
#include "RuntimeFastPath.h"

#if COMMON_ARCH_X86
DEFINE_FASTPATH_SCOPE(AVX512)
{
    return common::CheckCPUFeature("avx512f");
}
DEFINE_FASTPATH_SCOPE(AVX2)
{
    return common::CheckCPUFeature("avx2");
}
DEFINE_FASTPATH_SCOPE(SSE42)
{
    return true;
}
DECLARE_FASTPATH_PARTIALS(CopyManager, AVX512, AVX2, SSE42)
DECLARE_FASTPATH_PARTIALS(MiscIntrins, AVX512, AVX2, SSE42)
DECLARE_FASTPATH_PARTIALS(DigestFuncs, AVX2, SSE42)
#elif COMMON_ARCH_ARM
#   if COMMON_OSBIT == 64
DEFINE_FASTPATH_SCOPE(A64)
{
    return common::CheckCPUFeature("asimd");
}
DECLARE_FASTPATH_PARTIALS(CopyManager, A64)
DECLARE_FASTPATH_PARTIALS(MiscIntrins, A64)
DECLARE_FASTPATH_PARTIALS(DigestFuncs, A64)
#   else
DEFINE_FASTPATH_SCOPE(A32)
{
    return common::CheckCPUFeature("asimd");
}
DECLARE_FASTPATH_PARTIALS(CopyManager, A32)
DECLARE_FASTPATH_PARTIALS(MiscIntrins, A32)
DECLARE_FASTPATH_PARTIALS(DigestFuncs, A32)
#   endif
#endif

namespace common
{

DEFINE_FASTPATH_BASIC(CopyManager,
    Broadcast2, Broadcast4,
    ZExtCopy12, ZExtCopy14, ZExtCopy24, ZExtCopy28, ZExtCopy48,
    SExtCopy12, SExtCopy14, SExtCopy24, SExtCopy28, SExtCopy48,
    TruncCopy21, TruncCopy41, TruncCopy42, TruncCopy82, TruncCopy84,
    CvtI32F32, CvtI16F32, CvtI8F32, CvtU32F32, CvtU16F32, CvtU8F32,
    CvtF32I32, CvtF32I16, CvtF32I8, CvtF32U16, CvtF32U8,
    CvtF16F32, CvtF32F16, CvtF32F64, CvtF64F32)
const CopyManager CopyEx;


DEFINE_FASTPATH_BASIC(MiscIntrins,
    LeadZero32, LeadZero64, TailZero32, TailZero64, PopCount32, PopCount64, PopCounts, Hex2Str, PauseCycles)
const MiscIntrins MiscIntrin;


DEFINE_FASTPATH_BASIC(DigestFuncs, Sha256)
const DigestFuncs DigestFunc;


namespace spinlock
{

template<typename FL, typename FF> 
forceinline void WaitFramework(FL&& lock, FF&& fix)
{
    for (uint32_t i = 0; i < 16; ++i)
    {
        IF_LIKELY(lock()) return;
        fix();
        COMMON_PAUSE();
    }
    while (true)
    {
        uint32_t delays[2] = { 512, 256 };
        for (uint32_t i = 0; i < 8; ++i)
        {
            IF_LIKELY(lock()) return;
            fix();
            MiscIntrin.Pause(delays[0]);
            delays[0] <<= 1;
            IF_LIKELY(lock()) return;
            fix();
            MiscIntrin.Pause(delays[1]);
            delays[1] <<= 1;
        }
        std::this_thread::yield();
    }
}

void SpinLocker::Lock() noexcept
{
    WaitFramework([&]() { return !Flag.test_and_set(); }, []() {});
}

void PreferSpinLock::LockWeak() noexcept
{
    uint32_t expected = Flag.load() & 0x0000ffff; // assume no strong
    WaitFramework([&]() { return Flag.load() == expected && Flag.compare_exchange_strong(expected, expected + 1); },
        [&]() { expected &= 0x0000ffff; });
}

void PreferSpinLock::LockStrong() noexcept
{
    Flag.fetch_add(0x00010000);
    // loop until no weak
    WaitFramework([&]() { return (Flag.load() & 0x0000ffff) == 0; }, []() {});
}

void WRSpinLock::LockRead() noexcept
{
    uint32_t expected = Flag.load() & 0x7fffffffu; // assume no writer
    WaitFramework([&]() { return Flag.load() == expected && Flag.compare_exchange_weak(expected, expected + 1); },
        [&]() { expected &= 0x7fffffffu; });
}

void WRSpinLock::LockWrite() noexcept
{
    uint32_t expected = Flag.load() & 0x7fffffffu;
    // assume no other writer
    WaitFramework([&]() { return Flag.load() == expected && Flag.compare_exchange_weak(expected, expected + 0x80000000u); },
        [&]() { expected &= 0x7fffffffu; });
    // loop until no reader
    WaitFramework([&]() { return (Flag.load() & 0x7fffffffu) == 0; }, []() {});
}

void RWSpinLock::LockRead() noexcept
{
    Flag++;
    // loop until no writer
    WaitFramework([&]() { return (Flag.load() & 0x80000000u) == 0; }, []() {});
}

void RWSpinLock::LockWrite() noexcept
{
    uint32_t expected = 0; // assume no other locker
    WaitFramework([&]() { return Flag.load() == expected && Flag.compare_exchange_weak(expected, 0x80000000u); },
        [&]() { expected = 0; });
}

}

}