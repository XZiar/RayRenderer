#pragma once

#include "../3dBasic/3dElement.hpp"

#ifdef OGLU_EXPORT
#   define OGLUAPI _declspec(dllexport)
#   define OGLUTPL _declspec(dllexport) 
#else
#   define OGLUAPI _declspec(dllimport)
#   define OGLUTPL
#endif

#include <cstdio>
#include <string>

namespace oclu
{
class _oclMem;
}


namespace oglu
{

using std::string;

struct OGLUAPI NonCopyable
{
	NonCopyable() { }
	NonCopyable(const NonCopyable & other) = delete;
	NonCopyable(NonCopyable && other) = default;
	NonCopyable& operator= (const NonCopyable & other) = delete;
	NonCopyable& operator= (NonCopyable && other) = default;
};


template<class T, bool isNonCopy = std::is_base_of<NonCopyable, T>::value>
class OGLUTPL Wrapper;

template<class T>
class OGLUTPL Wrapper<T, true>
{
private:
	T *instance = nullptr;
public:
	Wrapper() { }
	template<class... ARGS>
	Wrapper(ARGS... args) : instance(new T(args...)) { }
	~Wrapper()
	{
		release();
	}
	Wrapper(const Wrapper<T>& other) = delete;
	Wrapper(Wrapper<T>&& other)
	{
		*this = std::move(other);
	}
	Wrapper& operator=(const Wrapper& other) = delete;
	Wrapper& operator=(Wrapper&& other)
	{
		release();
		instance = other.instance;
		other.instance = nullptr;
		return *this;
	}

	void release()
	{
		if (instance != nullptr)
		{
			delete instance;
			instance = nullptr;
		}
	}

	template<class... ARGS>
	void reset(ARGS... args)
	{
		release();
		instance = new T(args...);
	}
	void reset()
	{
		release();
		instance = new T();
	}

	T* operator-> ()
	{
		return instance;
	}
	const T* operator-> () const
	{
		return instance;
	}
	operator bool()
	{
		return instance != nullptr;
	}
	operator const bool() const
	{
		return instance != nullptr;
	}
};

template<class T>
class OGLUTPL Wrapper<T, false>
{
private:
	struct ControlBlock
	{
		T *instance = nullptr;;
		uint32_t count = 0;
		ControlBlock(T *dat) :instance(dat) { }
		~ControlBlock() { delete instance; }
	};
	ControlBlock *cb = nullptr;;
	void create(T* instance)
	{
		cb = new ControlBlock(instance);
	}
public:
	Wrapper() : cb(nullptr) { }
	template<class... ARGS>
	Wrapper(ARGS... args)
	{
		create(new T(args...));
	}
	~Wrapper()
	{
		release();
	}
	Wrapper(const Wrapper<T>& other)
	{
		*this = other;
	}
	Wrapper(Wrapper<T>&& other)
	{
		*this = std::move(other);
	}
	Wrapper& operator=(const Wrapper<T>& other)
	{
		release();
		cb = other.cb;
		if (cb != nullptr)
			cb->count++;
		return *this;
	}
	Wrapper& operator=(Wrapper<T>&& other)
	{
		release();
		cb = other.cb;
		other.cb = nullptr;
		return *this;
	}
	bool operator==(const Wrapper<T>& other) const
	{
		return cb == other.cb;
	}

	void release()
	{
		if (cb != nullptr)
		{
			if (!(--cb->count))
			{
				delete cb;
				cb = nullptr;
			}
		}
	}

	template<class... ARGS>
	void reset(ARGS... args)
	{
		release();
		create(new T(args...));
	}
	void reset()
	{
		release();
		create(new T());
	}

	T* operator-> ()
	{
		return cb->instance;
	}
	const T* operator-> () const
	{
		return cb->instance;
	}
	operator bool()
	{
		return cb != nullptr;
	}
	operator const bool() const
	{
		return cb != nullptr;
	}
};

template<class T = char>
class OGLUTPL OPResult
{
private:
	bool isSuc;
public:
	string msg;
	T data;
	OPResult(const bool isSuc_, const string& msg_ = "") :isSuc(isSuc_), msg(msg_) { }
	OPResult(const bool isSuc_, const T& dat_, const string& msg_ = "") :isSuc(isSuc_), msg(msg_), data(dat_) { }
	operator bool() { return isSuc; }
};

}