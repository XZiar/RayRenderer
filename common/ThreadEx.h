#pragma once

#include "CommonRely.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <memory>
#include <list>
#include <tuple>
#include <functional>

#if !defined(_MANAGED) && !defined(_M_CEE)
#   include <thread>
#endif

namespace common
{

bool SetThreadName(const std::string_view threadName);
bool SetThreadName(const std::u16string_view threadName);
std::u16string GetThreadName();


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif
class COMMONAPI ThreadObject : public NonCopyable
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
    bool IsAlive() const;
    bool IsCurrent() const;
    uint64_t GetId() const;
};

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

}