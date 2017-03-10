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
	T *instance;
public:
	Wrapper() noexcept : instance(nullptr) { }
	Wrapper(T * const src) noexcept : instance(src) { }
	template<class... ARGS>
	Wrapper(ARGS... args) : instance(new T(args...)) { }
	~Wrapper()
	{
		release();
	}
	Wrapper(const Wrapper<T, true>& other) = delete;
	Wrapper(Wrapper<T, true>&& other) noexcept : instance(other.instance)
	{
		other.instance = nullptr;
	}
	Wrapper& operator= (const Wrapper<T, true>& other) = delete;
	Wrapper& operator= (Wrapper<T, true>&& other)
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

	const T* pointer() const noexcept
	{
		return instance;
	}
	T* operator-> () noexcept
	{
		return instance;
	}
	const T* operator-> () const noexcept
	{
		return instance;
	}
	T& operator* () noexcept
	{
		return *instance;
	}
	const T& operator* () const noexcept
	{
		return *instance;
	}

	operator const bool() const noexcept
	{
		return instance != nullptr;
	}
	bool operator== (const Wrapper<T, true>& other) const noexcept
	{
		return instance == other.instance;
	}
	bool operator== (const T *pointer) const noexcept
	{
		return instance == pointer;
	}
};


template<class T>
class COMMONTPL Wrapper<T, false>
{
private:
	struct ControlBlock
	{
		T * const instance;
		uint32_t count = 1;
		ControlBlock(T * const pointer) noexcept :instance(pointer) { }
		~ControlBlock() noexcept { delete instance; }
	};
	ControlBlock *cb;
	void create(T * const instance) noexcept
	{
		cb = new ControlBlock(instance);
	}
public:
	Wrapper() noexcept : cb(nullptr) { }
	Wrapper(T * const src) noexcept
	{
		if (src == nullptr)
			cb = nullptr;
		else
			create(src);
	}
	template<class... ARGS>
	Wrapper(ARGS... args)
	{
		create(new T(args...));
	}
	~Wrapper()
	{
		release();
	}
	Wrapper(const Wrapper<T, false>& other) noexcept : cb(other.cb)
	{
		if (cb != nullptr)
			cb->count++;
	}
	Wrapper(Wrapper<T, false>&& other) noexcept : cb(other.cb)
	{
		other.cb = nullptr;
	}
	template<class U, class = typename std::enable_if<std::is_base_of<T, U>::value>::type>
	Wrapper(Wrapper<U, false>&& other) noexcept : cb((ControlBlock*)other.cb)
	{
		other.cb = nullptr;
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

	void release()
	{
		if (cb != nullptr)
		{
			if (--cb->count == 0)
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

	uint32_t refCount() const noexcept
	{
		if (cb == nullptr)
			return 0;
		else
			return cb->count;
	}

	const T* pointer() const noexcept
	{
		return cb->instance;
	}
	T* operator-> () noexcept
	{
		return cb->instance;
	}
	const T* operator-> () const noexcept
	{
		return cb->instance;
	}
	T& operator* () noexcept
	{
		return *cb->instance;
	}
	const T& operator* () const noexcept
	{
		return *cb->instance;
	}

	operator const bool() const noexcept
	{
		return cb != nullptr;
	}
	bool operator== (const Wrapper<T, false>& other) const noexcept
	{
		return cb == other.cb;
	}
	bool operator== (const T *pointer) const noexcept
	{
		if (cb == nullptr)
			return pointer == nullptr;
		else
			return cb->instance == pointer;
	}

	template<class U, class = typename std::enable_if<std::is_base_of<U, T>::value>::type>
	operator Wrapper<U, false>&() noexcept
	{
		return *(Wrapper<U, false>*)this;
	}
	template<class U, class = typename std::enable_if<std::is_base_of<U, T>::value>::type>
	operator const Wrapper<U, false>&() const noexcept
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


/** @brief calculate simple hash for string, used for switch-string
** @param str std-string for the text
** @return uint64_t the hash
**/
inline uint64_t hash_(const std::string& str)
{
	uint64_t hash = 0;
	for (size_t a = 0; a < str.length(); ++a)
		hash = hash * 33 + str[a];
	return hash;
}
/** @brief calculate simple hash for string, used for switch-string
** @param str std-string for the text
** @return uint64_t the hash
**/
constexpr uint64_t hash_(const char *str, const uint64_t last = 0)
{
	return *str == '\0' ? last : hash_(str + 1, *str + last * 33);
}
/** @brief calculate simple hash for string, used for switch-string
** @return uint64_t the hash
**/
constexpr uint64_t operator "" _hash(const char *str, size_t)
{
	return hash_(str);
}

}