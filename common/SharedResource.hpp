#pragma once

#include "CommonRely.hpp"
#include "SpinLock.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <exception>


namespace common
{

template<class T, typename... Args>
class SharedResource
{
public:
	using CreateFunc = std::function<T(Args&&...)>;
private:
	using inner_type = typename T::inner_type;
	using handle_type = std::weak_ptr<inner_type>;
	handle_type hres;
	CreateFunc creator;
	SpinLocker Locker;
public:
	SharedResource(CreateFunc cfunc) : creator(cfunc) {}
	~SharedResource() {}
	T get(Args... args)
	{
		T res = hres.lock();
		if (res)
			return res;
		//may need initialize
		{
			Locker.Lock();//enter critical section
			//check again, someone may has created it
			res = hres.lock();
			if (res)
				return res;
			//need to create it
			res = creator(std::forward<Args>(args)...);
			hres = res.weakRef();
			return res;
		}
	}
};

}