#pragma once

#include "TimeUtil.h"
#include <cstdint>
#include <chrono>
#include <thread>

namespace common
{


void sleepMS(uint32_t ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}



uint64_t SimpleTimer::getCurTime()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
uint64_t SimpleTimer::getCurTimeNS()
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}
SimpleTimer::SimpleTimer() { to = Start(); };
uint64_t SimpleTimer::Start() { return from = getCurTimeNS(); }
uint64_t SimpleTimer::Stop() { return to = getCurTimeNS(); }
uint64_t SimpleTimer::ElapseNs() const { return to - from; }
uint64_t SimpleTimer::ElapseUs() const { return ElapseNs() / 1000; }
uint64_t SimpleTimer::ElapseMs() const { return ElapseNs() / 1000000; }


}