#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef OGLU_EXPORT
#   define OGLUAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define OGLUAPI _declspec(dllimport)
# endif
#else
# ifdef OGLU_EXPORT
#   define OGLUAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define OGLUAPI
# endif
#endif



#include "ImageUtil/ImageCore.h"
#include "common/EnumEx.hpp"
#include "common/SpinLock.hpp"
#include "common/TimeUtil.hpp"
#include "common/Exceptions.hpp"
#include "common/PromiseTask.hpp"
#include "common/CommonRely.hpp"


//#if COMMON_OS_WIN
//#   define APIENTRY __stdcall
//#   define WINGDIAPI _declspec(dllimport)
//#   include <GL/gl.h>
//#   include "GL/glext.h"
//#elif COMMON_OS_UNIX
//#   define APIENTRY
//#   include <GL/gl.h>
//#   include "GL/glext.h"
//#endif
//#ifdef WINGDIAPI
//#   undef WINGDIAPI
//#endif

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
#if COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmismatched-tags"
#endif
#include "3rdParty/half/half.hpp"
#if COMPILER_CLANG
#   pragma clang diagnostic pop
#endif


#if COMPILER_MSVC
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


class oglWorker;
struct DSAFuncs;
class oglContext_;


namespace detail
{
class TextureManager;
class TexImgManager;
class UBOManager;
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

}
}


#if COMPILER_MSVC
#   pragma warning(pop)
#endif