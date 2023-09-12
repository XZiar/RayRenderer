#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef OCLU_EXPORT
#   define OCLUAPI _declspec(dllexport)
# else
#   define OCLUAPI _declspec(dllimport)
# endif
#else
# define OCLUAPI [[gnu::visibility("default")]]
#endif

#include "XComputeBase/XCompRely.h"
#include "ImageUtil/ImageCore.h"
#include "ImageUtil/TexFormat.h"
#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/PromiseTask.h"
#include "SystemCommon/Delegate.h"

#include "common/CommonRely.hpp"
#include "common/ContainerEx.hpp"
#include "common/FrozenDenseSet.hpp"
#include "common/EnumEx.hpp"

#include <cstdio>
#include <memory>
#include <functional>
#include <type_traits>
#include <assert.h>
#include <string>
#include <string_view>
#include <cstring>
#include <vector>
#include <set>
#include <map>
#include <tuple>
#include <optional>
#include <variant>


namespace oclu
{

template<typename T>
struct CLHandle
{
    uintptr_t Pointer;
    constexpr CLHandle(uintptr_t ptr = 0) noexcept : Pointer(ptr) {}
    CLHandle(void* ptr) noexcept : CLHandle(reinterpret_cast<uintptr_t>(ptr)) {}
    auto operator*() const noexcept
    {
        return reinterpret_cast<typename T::RealType>(Pointer);
    }
    auto operator&() const noexcept
    {
        return reinterpret_cast<std::add_pointer_t<const typename T::RealType>>(&Pointer);
    }
    auto operator&() noexcept
    {
        return reinterpret_cast<std::add_pointer_t<typename T::RealType>>(&Pointer);
    }
};

class oclUtil;
class oclMapPtr_;
class GLInterop;
class NLCLExecutor;
class NLCLRawExecutor;
class NLCLRuntime;
struct KernelContext;
class NLCLContext;
class NLCLProcessor;

class oclPlatform_;
class oclCmdQue_;
using oclCmdQue = std::shared_ptr<const oclCmdQue_>;

enum class Vendors { Other = 0, NVIDIA, Intel, AMD, ARM, Qualcomm };


namespace detail
{
struct PlatFuncs;
struct CLPlatform;
struct CLDevice;
struct CLContext;
struct CLProgram;
struct CLKernel;
struct CLCmdQue;
struct CLEvent;
struct CLMem;

struct oclCommon
{
protected:
    const PlatFuncs* Funcs;
    oclCommon(const PlatFuncs* funcs = nullptr) noexcept : Funcs(funcs) {}
};

}


class OCLUAPI OCLException : public common::BaseException
{
public:
    enum class CLComponent { Compiler, Driver, Accellarator, OCLU };
private:
    COMMON_EXCEPTION_PREPARE(OCLException, BaseException,
        const CLComponent Component;
        template<typename T>
        ExceptionInfo(T&& msg, const CLComponent source)
            : ExceptionInfo(TYPENAME, std::forward<T>(msg), source)
        { }
    protected:
        template<typename T>
        ExceptionInfo(const char* type, T&& msg, const CLComponent source)
            : TPInfo(type, std::forward<T>(msg)), Component(source)
        { }
    );
    OCLException(const CLComponent source, const std::u16string_view msg)
        : BaseException(T_<ExceptionInfo>{}, msg, source)
    { }
    OCLException(const CLComponent source, int32_t errcode, std::u16string msg);
};

}

