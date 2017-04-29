#pragma once

#include "TimeUtil.h"
#include <cstdint>
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
uint64_t SimpleTimer::getCurTimeUS()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}
uint64_t SimpleTimer::getCurTimeNS()
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}
SimpleTimer::SimpleTimer() { Start(); };
uint64_t SimpleTimer::Start() 
{ 
	from = std::chrono::high_resolution_clock::now();
	return from.time_since_epoch().count();
}
uint64_t SimpleTimer::Stop() 
{ 
	to = std::chrono::high_resolution_clock::now();
	return to.time_since_epoch().count();
}
uint64_t SimpleTimer::ElapseNs() const 
{ 
	auto period = to - from;
	return period.count();
}
uint64_t SimpleTimer::ElapseUs() const { return ElapseNs() / 1000ul; }
uint64_t SimpleTimer::ElapseMs() const { return ElapseNs() / 1000000ul; }


}