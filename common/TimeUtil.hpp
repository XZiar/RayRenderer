#pragma once

#include <cstdint>
#include <chrono>
#include <string>
#include <ctime>


#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
#   define CM_NODISCARD [[nodiscard]]
#else
#   define CM_NODISCARD
#endif


namespace common
{


struct SimpleTimer
{
private:
    std::chrono::high_resolution_clock::time_point from, to;
public:
    CM_NODISCARD static uint64_t getCurTime() noexcept
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    }
    CM_NODISCARD static uint64_t getCurTimeUS() noexcept
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    }
    CM_NODISCARD static uint64_t getCurTimeNS() noexcept
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    }
    CM_NODISCARD static auto getCurLocalTime()
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
    SimpleTimer() noexcept { Start(); }
    uint64_t Start() noexcept
    {
        from = std::chrono::high_resolution_clock::now();
        return from.time_since_epoch().count();
    }
    uint64_t Stop() noexcept
    {
        to = std::chrono::high_resolution_clock::now();
        return to.time_since_epoch().count();
    }
    CM_NODISCARD constexpr uint64_t ElapseNs() const noexcept
    {
        return (to - from).count();
    }
    CM_NODISCARD constexpr uint64_t ElapseUs() const noexcept
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(to - from).count();
    }
    CM_NODISCARD constexpr uint64_t ElapseMs() const noexcept
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(to - from).count();
    }
};


}

#undef CM_NODISCARD

