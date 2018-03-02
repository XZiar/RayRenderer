#pragma once

#include "CommonRely.hpp"
#include <cstdint>
#include <string>
#include <memory>
#include <list>
#include <tuple>
#ifndef _M_CEE
#   include <thread>
#endif

namespace common
{

bool __cdecl SetThreadName(const std::string& threadName);

struct COMMONAPI ThreadExitor : public NonCopyable
{
private:
    std::list<std::tuple<const void*, std::function<void(void)>>> Funcs;
public:
    static ThreadExitor& __cdecl GetThreadExitor();
    __declspec(noinline) ~ThreadExitor();
    static void __cdecl Add(const void * const uid, const std::function<void(void)>& callback)
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
    static void __cdecl Remove(const void * const uid)
    {
        ThreadExitor& exitor = GetThreadExitor();
        exitor.Funcs.remove_if([=](const auto& pair) { return std::get<0>(pair) == uid; });
    }
};


class COMMONAPI ThreadObject : public NonCopyable
{
protected:
    void *Handle = nullptr;
    ThreadObject(void *handle) : Handle(handle) { }
public:
    static ThreadObject __cdecl GetCurrentThreadObject();
#ifndef _M_CEE
    static ThreadObject __cdecl GetThreadObject(std::thread& thr);
#endif
    constexpr ThreadObject() noexcept { }
    ThreadObject(ThreadObject&& other) noexcept
    {
        Handle = other.Handle;
        other.Handle = nullptr;
    }
    ThreadObject& operator=(ThreadObject&& other) noexcept
    {
        std::swap(Handle, other.Handle);
        return *this;
    }
    ~ThreadObject();
    bool IsAlive() const;
    bool IsCurrent() const;
};

}