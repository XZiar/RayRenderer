#include "SystemCommonPch.h"
#include "DynamicLibrary.h"
#include "Format.h"
#include "Exceptions.h"
#include "StackTrace.h"
#include "StringConvert.h"
#if COMMON_OS_WIN
#   include "ErrorCodeHelper.h"
#endif

namespace common
{

#if COMMON_OS_WIN
#define THROW_ERR(syntax, ...) do                                   \
{                                                                   \
    const auto err = Win32ErrorHolder::GetLastError();              \
	COMMON_THROWEX(BaseException, str::Formatter<char16_t>{}        \
        .FormatStatic(FmtString(syntax), __VA_ARGS__))              \
        .Attach("errcode", err).Attach("detail", err.ToStr());      \
} while(0)
#else
#define THROW_ERR(syntax, ...) COMMON_THROWEX(BaseException, str::Formatter<char16_t>{}    \
    .FormatStatic(FmtString(syntax), __VA_ARGS__)).Attach("detail", str::to_u16string(dlerror()))
#endif

void* DynLib::TryOpen(const char* path) noexcept
{
#if COMMON_OS_WIN
	return LoadLibraryA(path);
#else
	return dlopen(path, RTLD_LAZY);
#endif
}
#if COMMON_OS_WIN
void* DynLib::TryOpen(const wchar_t* path) noexcept
{
    return LoadLibraryW(path);
}
#endif

void DynLib::Validate() const
{
    if (!Handle)
        THROW_ERR(u"Cannot load library [{}]", Name);
}

DynLib::~DynLib()
{
    if (Handle)
#if COMMON_OS_WIN
        FreeLibrary((HMODULE)Handle);
#else
        dlclose(Handle);
#endif
}

void* DynLib::GetFunction(std::string_view name) const
{
	const auto ptr = TryGetFunction(name);
	if (!ptr)
		THROW_ERR(u"Cannot load function [{}]", name);
	return ptr;
}

void* DynLib::TryGetFunction(std::string_view name) const noexcept
{
	Expects(*(&name.back() + 1) == '\0');
#if COMMON_OS_WIN
	return GetProcAddress((HMODULE)Handle, name.data());
#else
	return dlsym(Handle, name.data());
#endif
}

void* DynLib::FindLoaded(const fs::path& path) noexcept
{
    auto dllpath = path.lexically_normal();
    if (dllpath.has_parent_path())
    {
        dllpath = std::filesystem::canonical(dllpath);
    }
    dllpath.make_preferred();
#if COMMON_OS_WIN
    return GetModuleHandleW(dllpath.c_str());
#else
    return dlopen(path.c_str(), RTLD_NOLOAD);
#endif
}


}
