#pragma once

#include "SystemCommonRely.h"
#include "common/SpinLock.hpp"


namespace common
{

namespace spinlock
{

struct SpinLocker : private common::SpinLocker
{
public:
    using common::SpinLocker::SpinLocker;
    using common::SpinLocker::TryLock;
    SYSCOMMONAPI void Lock() noexcept;
    void Unlock() noexcept
    {
        common::SpinLocker::Unlock();
    }
    using ScopeType = detail::LockScope<SpinLocker, &SpinLocker::Lock, &SpinLocker::Unlock>;
    ScopeType LockScope() noexcept
    {
        return ScopeType(this);
    }
};


struct PreferSpinLock : private common::PreferSpinLock //Strong-first
{
    using common::PreferSpinLock::PreferSpinLock;
    SYSCOMMONAPI void LockWeak() noexcept;
    void UnlockWeak() noexcept
    {
        common::PreferSpinLock::UnlockWeak();
    }
    SYSCOMMONAPI void LockStrong() noexcept;
    void UnlockStrong() noexcept
    {
        common::PreferSpinLock::UnlockStrong();
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

struct WRSpinLock : private common::WRSpinLock //Writer-first
{
    using common::WRSpinLock::WRSpinLock;
    SYSCOMMONAPI void LockRead() noexcept;
    void UnlockRead() noexcept
    {
        common::WRSpinLock::UnlockRead();
    }
    SYSCOMMONAPI void LockWrite() noexcept;
    void UnlockWrite() noexcept
    {
        common::WRSpinLock::UnlockWrite();
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

struct RWSpinLock : private common::RWSpinLock //Reader-first
{
    using common::RWSpinLock::RWSpinLock;
    SYSCOMMONAPI void LockRead() noexcept;
    void UnlockRead() noexcept
    {
        common::RWSpinLock::UnlockRead();
    }
    SYSCOMMONAPI void LockWrite() noexcept;
    void UnlockWrite() noexcept
    {
        common::RWSpinLock::UnlockWrite();
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
    SYSCOMMONAPI void DowngradeToRead() noexcept;
};


}

}
