#include "oclPch.h"

#if COMMON_OS_WIN
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include <Windows.h>
#elif COMMON_OS_UNIX
#   include <dlfcn.h>
#else
#   error "unknown os"
#endif

#pragma message("Compiling miniBLAS with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace oclu
{

using namespace common::mlog;
MiniLogger<false>& oclLog()
{
    static MiniLogger<false> ocllog(u"OpenCLUtil", { GetConsoleBackend() });
    return ocllog;
}


std::pair<uint32_t, uint32_t> ParseVersionString(std::u16string_view str, const size_t verPos)
{
    std::pair<uint32_t, uint32_t> version { 0, 0 };
    const auto ver = common::str::SplitStream(str, u' ', false).Skip(verPos).TryGetFirst();
    if (ver.has_value())
    {
        common::str::SplitStream(ver.value(), u'.', false)
            .ForEach([&](const auto part, const auto idx)
                {
                    if (idx < 2)
                    {
                        uint32_t& target = idx == 0 ? version.first : version.second;
                        target = part[0] - u'0';
                    }
                });
    }
    return version;
}


namespace detail
{

PlatFuncs::PlatFuncs()
{
#define SET_FUNC_PTR(f) this->f = &::f; // use icd's export
    SET_FUNC_PTR(clGetExtensionFunctionAddress)
    SET_FUNC_PTR(clGetExtensionFunctionAddressForPlatform)
    PLATFUNCS_EACH(SET_FUNC_PTR)
#undef SET_FUNC_PTR
    InitExtensionFunc();
}

PlatFuncs::PlatFuncs(std::string_view dllName)
{
#if COMMON_OS_WIN
    Library = LoadLibraryExA(dllName.data(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
    Library = dlopen(dllName.data(), RTLD_LAZY);
#endif
    if (!Library) return;
#define SET_FUNC_PTR(f) f = reinterpret_cast<decltype(&::f)>(TryLoadFunc(#f));
    SET_FUNC_PTR(clGetExtensionFunctionAddress)
    SET_FUNC_PTR(clGetExtensionFunctionAddressForPlatform)
    PLATFUNCS_EACH(SET_FUNC_PTR)
#undef SET_FUNC_PTR
    InitExtensionFunc();
}

void* PlatFuncs::TryLoadFunc(const char* fname) const noexcept
{
    void* func = nullptr;
    if (this->clGetExtensionFunctionAddress)
        func = this->clGetExtensionFunctionAddress(fname);
    if (!func && Library)
#if COMMON_OS_WIN
        func = GetProcAddress((HMODULE)Library, fname);
#else
        func = dlsym(Library, fname);
#endif
    return func;
}

void PlatFuncs::InitExtensionFunc() noexcept
{
#define SET_FUNC_PTR(f) f = reinterpret_cast<decltype(&::f)>(TryLoadFunc(#f));
    SET_FUNC_PTR(clGetGLContextInfoKHR)
    SET_FUNC_PTR(clGetKernelSubGroupInfoKHR)
#undef SET_FUNC_PTR
}

PlatFuncs::~PlatFuncs()
{
    if (Library)
#if COMMON_OS_WIN
        FreeLibrary((HMODULE)Library);
#else
        dlclose(Library);
#endif
}

}


}