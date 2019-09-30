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

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "common/CopyEx.hpp"
#include "common/Wrapper.hpp"
#include "common/ContainerEx.hpp"
#include "common/SpinLock.hpp"
#include "common/StringEx.hpp"
#include "common/FileEx.hpp"
#include "common/TimeUtil.hpp"
#include "common/Exceptions.hpp"
#include "common/PromiseTask.hpp"
#include "common/Linq.hpp"
#include "StringCharset/Convert.h"
#include "SystemCommon/ThreadEx.h"
#include "ImageUtil/ImageUtil.h"

#define GLEW_STATIC
#include "3rdParty/glew/glew.h"


#if COMPILER_MSVC && !defined(_ENABLE_EXTENDED_ALIGNED_STORAGE)
#   error "require aligned storage fix"
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
#include <algorithm>
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
namespace str = common::str;
namespace fs = common::fs;
using std::string;
using std::string_view;
using std::wstring;
using std::u16string;
using std::u16string_view;
using std::byte;
using std::tuple;
using std::pair;
using std::vector;
using std::array;
using std::map;
using std::set;
using std::function;
using std::optional;
using std::variant;
using common::Wrapper;
using common::SimpleTimer;
using common::NonCopyable;
using common::NonMovable;
using common::PromiseResult;
using common::BaseException;
using common::file::FileException;
using common::linq::Linq;
using common::str::Charset;


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


class oglWorker;
struct DSAFuncs;


namespace detail
{
class TextureManager;
class TexImgManager;
class UBOManager;
struct SharedContextCore;
class _oglContext;
class _oglBuffer;

template<bool IsShared>
class OGLUAPI oglCtxObject : public common::NonCopyable
{
private:
    using CtxType = std::conditional_t<IsShared, SharedContextCore, _oglContext>;
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

#ifdef OGLU_EXPORT
#include "MiniLogger/MiniLogger.h"
namespace oglu
{
common::mlog::MiniLogger<false>& oglLog();
}
#endif


#if COMPILER_MSVC
#   pragma warning(pop)
#endif