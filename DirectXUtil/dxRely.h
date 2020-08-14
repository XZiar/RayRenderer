#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef DXU_EXPORT
#   define DXUAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define DXUAPI _declspec(dllimport)
# endif
#else
# ifdef DXU_EXPORT
#   define DXUAPI [[gnu::visibility("default")]]
#   define COMMON_EXPORT
# else
#   define DXUAPI
# endif
#endif



#include "ImageUtil/ImageCore.h"
#include "SystemCommon/PromiseTask.h"
#include "common/EnumEx.hpp"
#include "common/AtomicUtil.hpp"
#include "common/ContainerEx.hpp"
#include "common/FrozenDenseSet.hpp"
#include "common/FileBase.hpp"
#include "common/SpinLock.hpp"
#include "common/TimeUtil.hpp"
#include "common/Exceptions.hpp"
#include "common/CommonRely.hpp"

#if COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmismatched-tags"
#endif
#include "3rdParty/half/half.hpp"
#if COMPILER_CLANG
#   pragma clang diagnostic pop
#endif


#if COMPILER_MSVC && !defined(_ENABLE_EXTENDED_ALIGNED_STORAGE)
#   error "require aligned storage fix"
#endif
#if !COMMON_OVER_ALIGNED
#   error require c++17 + aligned_new to support memory management for over-aligned SIMD type.
#endif


#include <cstddef>
#include <cstdio>
#include <memory>
#include <functional>
#include <type_traits>
#include <string.h>
#include <string>
#include <cstring>
#include <array>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <optional>
#include <variant>
#include <atomic>


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif



namespace dxu
{


class DXException : public common::BaseException
{
public:
    enum class CLComponent { Compiler, Driver, DXU };
private:
    PREPARE_EXCEPTION(DXException, BaseException,
        template<typename T>
        ExceptionInfo(T&& msg)
            : ExceptionInfo(TYPENAME, std::forward<T>(msg))
        { }
    protected:
        template<typename T>
        ExceptionInfo(const char* type, T&& msg)
            : TPInfo(type, std::forward<T>(msg))
        { }
    );
    DXException(std::u16string msg);
    DXException(int32_t hresult, std::u16string msg);
};


//struct ContextCapability
//{
//    common::container::FrozenDenseSet<std::string_view> Extensions;
//    std::u16string VendorString;
//    std::u16string VersionString;
//    uint32_t Version            = 0;
//    bool SupportDebug           = false;
//    bool SupportSRGB            = false;
//    bool SupportClipControl     = false;
//    bool SupportGeometryShader  = false;
//    bool SupportComputeShader   = false;
//    bool SupportTessShader      = false;
//    bool SupportBindlessTexture = false;
//    bool SupportImageLoadStore  = false;
//    bool SupportSubroutine      = false;
//    bool SupportIndirectDraw    = false;
//    bool SupportBaseInstance    = false;
//    bool SupportVSMultiLayer    = false;
//
//    DXUAPI std::string GenerateSupportLog() const;
//};


}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif