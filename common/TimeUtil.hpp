#pragma once

#include <cstdint>
#include <chrono>
#include <string>
#include <ctime>

namespace common
{


struct SimpleTimer
{
private:
    std::chrono::high_resolution_clock::time_point from, to;
public:
    static uint64_t getCurTime()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
    static uint64_t getCurTimeUS()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    static uint64_t getCurTimeNS()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    static auto getCurLocalTime()
    {
        const auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm tm;
    #if defined(_WIN32)
        localtime_s(&tm, &t);
    #else
        localtime_r(&t, &tm);
    #endif
        return tm;
    }
    SimpleTimer() { Start(); }
    uint64_t Start()
    {
        from = std::chrono::high_resolution_clock::now();
        return from.time_since_epoch().count();
    }
    uint64_t Stop()
    {
        to = std::chrono::high_resolution_clock::now();
        return to.time_since_epoch().count();
    }
    uint64_t ElapseNs() const
    {
        return (to - from).count();
    }
    uint64_t ElapseUs() const
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(to - from).count();
    }
    uint64_t ElapseMs() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(to - from).count();
    }
};


}