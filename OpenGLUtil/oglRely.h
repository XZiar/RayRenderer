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
#include "common/AlignedContainer.hpp"
#include "common/CopyEx.hpp"
#include "common/Wrapper.hpp"
#include "common/ContainerEx.hpp"
#include "common/SpinLock.hpp"
#include "common/StringEx.hpp"
#include "common/FileEx.hpp"
#include "common/TimeUtil.hpp"
#include "common/Exceptions.hpp"
#include "common/PromiseTask.hpp"
#include "common/ThreadEx.h"
#include "common/Linq.hpp"
#include "ImageUtil/ImageUtil.h"

#define GLEW_STATIC
#include "glew/glew.h"


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
#include "half/half.hpp"
#if COMPILER_CLANG
#   pragma clang diagnostic pop
#endif

namespace oclu
{
namespace detail
{
class _oclGLBuffer;
class _oclPlatform;
class _oclGLImage;
class GLInterOP;
}
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
using common::vectorEx;
using common::PromiseResult;
using common::BaseException;
using common::FileException;
using common::linq::Linq;

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
    const std::weak_ptr<CtxType> Context;
    static std::weak_ptr<CtxType> GetCtx();
protected:
    oglCtxObject() : Context(GetCtx()) { }
    bool EnsureValid();
public:
    void CheckCurrent() const;
};

}
}

#ifdef OGLU_EXPORT
#include "common/miniLogger/miniLogger.h"
namespace oglu
{
common::mlog::MiniLogger<false>& oglLog();
}
#endif