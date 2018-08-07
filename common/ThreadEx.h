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

bool SetThreadName(const std::string& threadName);
bool SetThreadName(const std::u16string& threadName);

struct COMMONAPI ThreadExitor : public NonCopyable
{
private:
    std::list<std::tuple<const void*, std::function<void(void)>>> Funcs;
public:
    static ThreadExitor& GetThreadExitor();
    forcenoinline ~ThreadExitor();
    static void Add(const void * const uid, const std::function<void(void)>& callback)
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
    static void Remove(const void * const uid)
    {
        ThreadExitor& exitor = GetThreadExitor();
        exitor.Funcs.remove_if([=](const auto& pair) { return std::get<0>(pair) == uid; });
    }
};


class COMMONAPI ThreadObject : public NonCopyable
{
protected:
    uintptr_t Handle;
    uint64_t TId;
    ThreadObject(const uintptr_t handle) noexcept : Handle(handle), TId(GetId()) { }
public:
    static ThreadObject GetCurrentThreadObject();
    static uint64_t GetCurrentThreadId();
    static ThreadObject GetThreadObject(std::thread& thr);
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

}