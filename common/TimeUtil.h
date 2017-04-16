#pragma once

#include <cstdint>

namespace common
{


void sleepMS(uint32_t ms);

struct SimpleTimer
{
private:
	uint64_t from, to;
public:
	static uint64_t getCurTime();
	static uint64_t getCurTimeNS();
	SimpleTimer();
	uint64_t Start();
	uint64_t Stop();
	uint64_t ElapseNs() const;
	uint64_t ElapseUs() const;
	uint64_t ElapseMs() const;
};


}