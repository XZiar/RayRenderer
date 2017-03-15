#pragma once

#include <cstdint>
#include <chrono>
#include <thread>

namespace common
{


inline void sleepMS(uint32_t ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

struct SimpleTimer
{
private:
	uint64_t from, to;
public:
	static uint64_t getCurTime() 
	{ 
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); 
	}
	static uint64_t getCurTimeNS() 
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	}
	SimpleTimer() { to = Start(); };
	uint64_t Start() { return from = getCurTimeNS(); }
	uint64_t Stop() { return to = getCurTimeNS(); }
	uint64_t ElapseNs() const { return to - from; }
	uint64_t ElapseUs() const { return ElapseNs() / 1000; }
	uint64_t ElapseMs() const { return ElapseNs() / 1000000; }
};


}