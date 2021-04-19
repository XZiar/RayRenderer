#pragma once

#include "SystemCommonRely.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <memory>
#include <optional>

#if !defined(_MANAGED) && !defined(_M_CEE)
#   include <thread>
#endif

namespace common
{

SYSCOMMONAPI bool SetThreadName(const std::string_view threadName);
SYSCOMMONAPI bool SetThreadName(const std::u16string_view threadName);
SYSCOMMONAPI std::u16string GetThreadName();


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
class SYSCOMMONAPI ThreadObject : public NonCopyable
{
protected:
    uintptr_t Handle;
    uint64_t TId;
    ThreadObject(const uintptr_t handle) noexcept : Handle(handle), TId(GetId()) { }
public:
    static ThreadObject GetCurrentThreadObject();
    static uint64_t GetCurrentThreadId();
#if !defined(_MANAGED) && !defined(_M_CEE)
    static ThreadObject GetThreadObject(std::thread& thr);
#endif
    constexpr ThreadObject() noexcept : Handle(0), TId(0) { }
    ThreadObject(ThreadObject&& other) noexcept
    {
        Handle = other.Handle;
        other.Handle = 0;
        TId = other.TId;
    }
    ThreadObject& operator=(ThreadObject&& other) noexcept
    {
        std::swap(Handle, other.Handle);
        TId = other.TId;
        return *this;
    }
    ~ThreadObject();
    std::optional<bool> IsAlive() const;
    bool IsCurrent() const;
    uint64_t GetId() const;
};

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

}