#pragma once

#include "CommonRely.hpp"
#include <cstdint>
#include <string>
#include <memory>
#include <list>
#include <tuple>

namespace std
{
class thread;
}

namespace common
{

bool CDECLCALL SetThreadName(const std::string& threadName);
bool CDECLCALL SetThreadName(const std::u16string& threadName);

struct COMMONAPI ThreadExitor : public NonCopyable
{
private:
    std::list<std::tuple<const void*, std::function<void(void)>>> Funcs;
public:
    static ThreadExitor& CDECLCALL GetThreadExitor();
    forcenoinline ~ThreadExitor();
    static void CDECLCALL Add(const void * const uid, const std::function<void(void)>& callback)
    {
        ThreadExitor& exitor = GetThreadExitor();
        for (auto& funcPair : exitor.Funcs)
        {
            if (std::get<0>(funcPair) == uid)
            {
                std::get<1>(funcPair) = callback;
                return;
            }
        }
        exitor.Funcs.push_front(std::make_tuple(uid, callback));
    }
    static void CDECLCALL Remove(const void * const uid)
    {
        ThreadExitor& exitor = GetThreadExitor();
        exitor.Funcs.remove_if([=](const auto& pair) { return std::get<0>(pair) == uid; });
    }
};


class COMMONAPI ThreadObject : public NonCopyable
{
protected:
    uintptr_t Handle;
    ThreadObject(const uintptr_t handle) noexcept : Handle(handle) { }
public:
    static ThreadObject CDECLCALL GetCurrentThreadObject();
    static uint32_t CDECLCALL GetCurrentThreadId();
    static ThreadObject CDECLCALL GetThreadObject(std::thread& thr);
    constexpr ThreadObject() noexcept : Handle(0) { }
    ThreadObject(ThreadObject&& other) noexcept
    {
        Handle = other.Handle;
        other.Handle = 0;
    }
    ThreadObject& operator=(ThreadObject&& other) noexcept
    {
        std::swap(Handle, other.Handle);
        return *this;
    }
    ~ThreadObject();
    bool IsAlive() const;
    bool IsCurrent() const;
    uint32_t GetId() const;
};

}