#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef OGLU_EXPORT
#   define OGLUAPI _declspec(dllexport)
# else
#   define OGLUAPI _declspec(dllimport)
# endif
#else
# define OGLUAPI [[gnu::visibility("default")]]
#endif


#include "XComputeBase/XCompRely.h"
#include "ImageUtil/ImageCore.h"
#include "SystemCommon/PromiseTask.h"
#include "SystemCommon/Exceptions.h"
#include "common/EnumEx.hpp"
#include "common/AtomicUtil.hpp"
#include "common/ContainerEx.hpp"
#include "common/FrozenDenseSet.hpp"
#include "common/FileBase.hpp"
#include "common/SpinLock.hpp"
#include "common/TimeUtil.hpp"
#include "common/math/VecBase.hpp"
#include "common/math/MatBase.hpp"
#include "common/CommonRely.hpp"

#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmismatched-tags"
#endif
#include "3rdParty/half/half.hpp"
#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic pop
#endif


#if COMMON_COMPILER_MSVC && !defined(_ENABLE_EXTENDED_ALIGNED_STORAGE)
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


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class GLInterop; 
class oclPlatform_;
class oclGLBuffer_;
class GLInterOP;
}

namespace oglu
{

namespace mbase = common::math::base;

inline constexpr uint32_t GLInvalidEnum  = UINT32_MAX;
inline constexpr int32_t  GLInvalidIndex = -1;
inline constexpr uint32_t OGLUMsgIdMin = 0xdeafbeef;
inline constexpr uint32_t OGLUMsgIdMax = 0xdeafcafe;


class oglLoader;
class oglWorker;
class oglContext_;
class CtxFuncs;
class GLHost;


enum class GLType : uint8_t { Desktop, ES };

struct ContextBaseInfo
{
    std::u16string RendererString;
    std::u16string VendorString;
    std::u16string VersionString;
    uint32_t Version = 0;
};
struct ContextCapability : public ContextBaseInfo
{
    common::container::FrozenDenseSet<std::string_view> Extensions;
    bool SupportDebug           = false;
    bool SupportSRGB            = false;
    bool SupportClipControl     = false;
    bool SupportGeometryShader  = false;
    bool SupportComputeShader   = false;
    bool SupportTessShader      = false;
    bool SupportBindlessTexture = false;
    bool SupportImageLoadStore  = false;
    bool SupportSubroutine      = false;
    bool SupportIndirectDraw    = false;
    bool SupportBaseInstance    = false;
    bool SupportVSMultiLayer    = false;

    OGLUAPI std::string GenerateSupportLog() const;
};


namespace detail
{
class BindlessTexManager;
class BindlessImgManager;
struct SharedContextCore;

template<bool IsShared>
class OGLUAPI oglCtxObject : public common::NonCopyable
{
private:
    using CtxType = std::conditional_t<IsShared, SharedContextCore, oglContext_>;
    static std::weak_ptr<CtxType> GetCtx();
    const std::weak_ptr<CtxType> Context;
protected:
    oglCtxObject() : Context(GetCtx()) { }
    bool EnsureValid();
public:
    void CheckCurrent() const;
};

template<typename T>
class ResourceBinder
{
public:
    virtual ~ResourceBinder() { }
    virtual void BindAll(const uint32_t progId, const std::map<uint32_t, std::shared_ptr<T>>&, std::vector<int32_t>&) = 0;
    virtual void ReleaseRes(const T* res) = 0;
};
template<typename T>
class CachedResManager;
}


}


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif