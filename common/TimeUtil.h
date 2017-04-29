#pragma once

#include <cstdint>
#include <chrono>

namespace common
{


void sleepMS(uint32_t ms);

struct SimpleTimer
{
private:
	std::chrono::high_resolution_clock::time_point from, to;
public:
	static uint64_t getCurTime();
	static uint64_t getCurTimeUS();
	static uint64_t getCurTimeNS();
	SimpleTimer();
	uint64_t Start();
	uint64_t Stop();
	uint64_t ElapseNs() const;
	uint64_t ElapseUs() const;
	uint64_t ElapseMs() const;
};


}