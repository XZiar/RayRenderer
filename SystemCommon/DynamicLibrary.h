#pragma once
#include "SystemCommonRely.h"
#include "common/FileBase.hpp"

namespace common
{
class DynLib
{
private:
	void* Handle;
    std::u16string Name;

    explicit DynLib(void* handle, std::u16string&& name) noexcept : Handle(handle), Name(std::move(name)) {}
	SYSCOMMONAPI void* GetFunction(std::string_view name) const;
	SYSCOMMONAPI void* TryGetFunction(std::string_view name) const noexcept;
	
    SYSCOMMONAPI static void* TryOpen(const char* path) noexcept;
#if COMMON_OS_WIN
    SYSCOMMONAPI static void* TryOpen(const wchar_t* path) noexcept;
#endif
    static void* TryOpen(const fs::path& path) noexcept
    {
        return TryOpen(path.c_str());
    }
    SYSCOMMONAPI void Validate() const;
public:
    template<typename T>
    DynLib(const T& path) : Handle(TryOpen(path))
    {
        if constexpr (std::is_same_v<T, fs::path>)
            Name = path.u16string();
        else
            Name = str::to_u16string(path);
        Validate();
    }
	SYSCOMMONAPI ~DynLib();
	DynLib(const DynLib&) = delete;
	DynLib(DynLib&& other) noexcept : Handle(other.Handle), Name(std::move(other.Name)) 
    { 
        other.Handle = nullptr;
    }
	DynLib& operator=(const DynLib&) = delete;
	DynLib& operator=(DynLib&&) = delete;

    constexpr bool operator==(const DynLib& other) const noexcept { return Handle == other.Handle; }
    constexpr bool operator==(const void* handle) const noexcept { return Handle == handle; }

	constexpr explicit operator bool() const noexcept { return Handle; }
	template<typename T>
	T GetFunction(std::string_view name) const
	{
		return reinterpret_cast<T>(GetFunction(name));
	}
	template<typename T>
	T TryGetFunction(std::string_view name) const noexcept
	{
		return reinterpret_cast<T>(TryGetFunction(name));
	}
	static DynLib TryCreate(const fs::path& path) noexcept
	{
        return DynLib{ TryOpen(path), path.u16string() };
	}
    SYSCOMMONAPI static void* FindLoaded(const fs::path& path) noexcept;
};
}
