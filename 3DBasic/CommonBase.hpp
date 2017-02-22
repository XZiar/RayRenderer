#pragma once

#include <cstdint>
#include <string>
#include <codecvt>

#ifdef COMMON_EXPORT
#   define COMMONAPI _declspec(dllexport) 
#   define COMMONTPL _declspec(dllexport) 
#else
#   define COMMONAPI _declspec(dllimport) 
#   define COMMONTPL
#endif

namespace common
{

struct COMMONAPI NonCopyable
{
	NonCopyable() = default;
	NonCopyable(const NonCopyable & other) = delete;
	NonCopyable(NonCopyable && other) = default;
	NonCopyable& operator= (const NonCopyable & other) = delete;
	NonCopyable& operator= (NonCopyable && other) = default;
};

struct COMMONAPI NonMovable
{
	NonMovable() = default;
	NonMovable(const NonMovable & other) = default;
	NonMovable(NonMovable && other) = delete;
	NonMovable& operator= (const NonMovable & other) = default;
	NonMovable& operator= (NonMovable && other) = delete;
};


template<class T, bool isUnique>
class COMMONTPL Wrapper;

template<class T>
class COMMONTPL Wrapper<T, true>
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
	Wrapper(const Wrapper<T, true>& other) = delete;
	Wrapper(Wrapper<T, true>&& other)
	{
		*this = std::move(other);
	}
	Wrapper& operator=(const Wrapper<T, true>& other) = delete;
	Wrapper& operator=(Wrapper<T, true>&& other)
	{
		release();
		instance = other.instance;
		other.instance = nullptr;
		return *this;
	}
	bool operator==(const Wrapper<T, true>& other) const
	{
		return instance == other.instance;
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
struct ControlBlock
{
	T *instance = nullptr;
	uint32_t count = 1;
	ControlBlock(T *dat) :instance(dat) { }
	~ControlBlock() { delete instance; }
};

template<class T>
class COMMONTPL Wrapper<T, false>
{
private:
	ControlBlock<T> *cb = nullptr;;
	void create(T* instance)
	{
		cb = new ControlBlock<T>(instance);
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
	Wrapper(const Wrapper<T, false>& other)
	{
		*this = other;
	}
	Wrapper(Wrapper<T, false>&& other)
	{
		*this = std::move(other);
	}
	template<class U, class = typename std::enable_if<std::is_base_of<T, U>::value>::type>
	Wrapper(Wrapper<U, false>&& other)
	{
		*this = std::move(*(Wrapper<T, false>*)&other);
	}
	Wrapper& operator=(const Wrapper<T, false>& other)
	{
		release();
		cb = other.cb;
		if (cb != nullptr)
			cb->count++;
		return *this;
	}
	Wrapper& operator=(Wrapper<T, false>&& other)
	{
		release();
		cb = other.cb;
		other.cb = nullptr;
		return *this;
	}
	bool operator==(const Wrapper<T, false>& other) const
	{
		return cb == other.cb;
	}

	void release()
	{
		if (cb != nullptr)
		{
			if (!(--cb->count))
				delete cb;
			cb = nullptr;
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
	T& operator* ()
	{
		return *cb->instance;
	}
	const T& operator* () const
	{
		return *cb->instance;
	}

	operator bool()
	{
		return cb != nullptr;
	}
	operator const bool() const
	{
		return cb != nullptr;
	}
	template<class U, class = typename std::enable_if<std::is_base_of<U, T>::value>::type>
	operator Wrapper<U, false>&()
	{
		return *(Wrapper<U, false>*)this;
	}
	template<class U, class = typename std::enable_if<std::is_base_of<U, T>::value>::type>
	operator const Wrapper<U, false>&() const
	{
		return *(Wrapper<U, false>*)this;
	}
};


template<class T = char>
class COMMONTPL OPResult
{
private:
	bool isSuc;
public:
	std::wstring msg;
	T data;
	OPResult(const bool isSuc_, const std::wstring& msg_) :isSuc(isSuc_), msg(msg_) { }
	OPResult(const bool isSuc_, const std::wstring& msg_, const T& dat_) :isSuc(isSuc_), msg(msg_), data(dat_) { }
	OPResult(const bool isSuc_, const std::string& msg_ = "")
		:OPResult(isSuc_, std::wstring(msg_.begin(), msg_.end())) { }
	OPResult(const bool isSuc_, const std::string& msg_, const T& dat_) 
		:OPResult(isSuc_, std::wstring(msg_.begin(), msg_.end()), dat_) { }
	operator bool() { return isSuc; }
};

}