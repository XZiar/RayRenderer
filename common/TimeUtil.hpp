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
    std::chrono::high_resolution_clock::time_point From, To;
public:
    [[nodiscard]] static uint64_t getCurTime() noexcept
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    }
    [[nodiscard]] static uint64_t getCurTimeUS() noexcept
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    }
    [[nodiscard]] static uint64_t getCurTimeNS() noexcept
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    }
    [[nodiscard]] static auto getCurLocalTime()
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
        From = std::chrono::high_resolution_clock::now();
        return From.time_since_epoch().count();
    }
    uint64_t Stop() noexcept
    {
        To = std::chrono::high_resolution_clock::now();
        return To.time_since_epoch().count();
    }
    [[nodiscard]] constexpr uint64_t ElapseNs() const noexcept
    {
        return (To - From).count();
    }
    template<bool Float = false>
    [[nodiscard]] constexpr std::conditional_t<Float, float, uint64_t> ElapseUs() const noexcept
    {
        if constexpr (!Float)
            return std::chrono::duration_cast<std::chrono::microseconds>(To - From).count();
        else
            return static_cast<float>(ElapseNs()) / 1000.f;
    }
    template<bool Float = false>
    [[nodiscard]] constexpr std::conditional_t<Float, float, uint64_t> ElapseMs() const noexcept
    {
        if constexpr (!Float)
            return std::chrono::duration_cast<std::chrono::milliseconds>(To - From).count();
        else
            return static_cast<float>(ElapseNs()) / 1e6f;
    }
};


}


