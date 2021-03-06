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
using GLboolean  = uint8_t;
using GLbyte     = int8_t;
using GLubyte    = uint8_t;
using GLshort    = int16_t;
using GLushort   = uint16_t;
using GLint      = int32_t;
using GLuint     = uint32_t;
using GLfixed    = uint32_t;
using GLint64    = int64_t;
using GLuint64   = uint64_t;
using GLsizei    = int32_t;
using GLenum     = uint32_t;
using GLintptr   = ptrdiff_t;
using GLsizeiptr = ptrdiff_t;
using GLbitfield = uint32_t;
using GLhalf     = uint16_t;
using GLfloat    = float;
using GLclampf   = float;
using GLdouble   = double;
using GLclampd   = double;
using GLchar     = char;
using GLsync     = void*;

inline constexpr GLenum GLInvalidEnum  = UINT32_MAX;
inline constexpr GLint  GLInvalidIndex = -1;
inline constexpr GLuint OGLUMsgIdMin = 0xdeafbeef;
inline constexpr GLuint OGLUMsgIdMax = 0xdeafcafe;


class oglWorker;
class oglContext_;


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
    virtual void BindAll(const GLuint progId, const std::map<GLuint, std::shared_ptr<T>>&, std::vector<GLint>&) = 0;
    virtual void ReleaseRes(const T* res) = 0;
};
template<typename T>
class CachedResManager;

}


struct ContextCapability
{
    common::container::FrozenDenseSet<std::string_view> Extensions;
    std::u16string VendorString;
    std::u16string VersionString;
    uint32_t Version            = 0;
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


}


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif