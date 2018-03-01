#pragma once

#include <cstdint>
#include <chrono>
#include <string>
#ifndef NO_DATE_FORMATE
#   if _HAS_CXX17
#      define HAS_STRING_VIEW 1
#   endif
#   include "../3rdParty/date/date.h"
#endif

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
#ifndef NO_DATE_FORMATE
	static std::wstring getCurTimeTxt()
	{
		const auto str = date::format("%H:%M:%S", std::chrono::system_clock::now());
		return std::wstring(str.cbegin(), str.cend());
	}
#endif
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