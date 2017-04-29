#pragma once
#include "CommonRely.hpp"
#include "Wrapper.hpp"
#include <atomic>
#include <functional>
#include <exception>


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
		{ }
	}
	~SpinLocker()
	{
		lock.clear();
	}
};

template<class T>
class SharedResource
{
public:
	using CreateFunc = std::function<T(void)>;
private:
	using inner_type = typename T::inner_type;
	using handle_type = std::weak_ptr<inner_type>;
	handle_type hres;
	CreateFunc creator;
	std::atomic_flag lockObj = ATOMIC_FLAG_INIT;
public:
	SharedResource(CreateFunc cfunc) : creator(cfunc) {}
	~SharedResource() {}
	T get()
	{
		T res = hres.lock();
		if (res)
			return res;
		//may need initialize
		{
			SpinLocker locker(lockObj);//enter critical section
			//check again, someone may has created it
			res = hres.lock();
			if (res)
				return res;
			//need to create it
			res = creator();
			hres = res.weakRef();
			return res;
		}
	}
};

}