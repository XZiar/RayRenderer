#pragma once

#include <atomic>

namespace common
{

struct SpinLocker
{
private:
	std::atomic_flag& lock;
public:
	SpinLocker(std::atomic_flag& flag) : lock(flag)
	{
		while (!lock.test_and_set())
		{
		}
	}
	~SpinLocker()
	{
		lock.clear();
	}
};

}
