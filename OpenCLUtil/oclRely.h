#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef OCLU_EXPORT
#   define OCLUAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define OCLUAPI _declspec(dllimport)
# endif
#else
# ifdef OCLU_EXPORT
#   define OCLUAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define OCLUAPI
# endif
#endif

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "common/Exceptions.hpp"
#include "common/ContainerEx.hpp"
#include "common/StringEx.hpp"
#include "common/PromiseTask.hpp"
#include "common/Linq.hpp"

#include "ImageUtil/ImageCore.h"
#include "StringCharset/Convert.h"
#include "OpenGLUtil/oglBuffer.h"
#include "OpenGLUtil/oglTexture.h"
#include "OpenGLUtil/oglContext.h"

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


#define CL_TARGET_OPENCL_VERSION 220
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#include "3rdParty/CL/opencl.h"
#include "3rdParty/CL/cl_ext_intel.h"

namespace oclu
{
namespace str = common::str;
namespace fs = common::fs;
using std::string;
using std::wstring;
using std::u16string;
using std::string_view;
using std::u16string_view;
using std::wstring_view;
using std::tuple;
using std::set;
using std::map;
using std::vector;
using std::function;
using std::optional;
using common::min;
using common::max;
using common::Wrapper;
using common::SimpleTimer;
using common::NonCopyable;
using common::NonMovable;
using common::PromiseResult;
using common::BaseException;
using common::file::FileException;
using common::linq::Linq;

class oclUtil;
using MessageCallBack = std::function<void(const u16string&)>;

enum class Vendor { Other = 0, NVIDIA, Intel, AMD };


class oclPlatform_;
using oclPlatform = std::shared_ptr<oclPlatform_>;
class oclProgram_;
using oclProgram = std::shared_ptr<oclProgram_>;
class oclKernel_;
using oclKernel = std::shared_ptr<oclKernel_>;
class oclDevice_;
using oclDevice = std::shared_ptr<const oclDevice_>;
class oclCmdQue_;
using oclCmdQue = std::shared_ptr<oclCmdQue_>;
class oclContext_;
using oclContext = std::shared_ptr<oclContext_>;
class oclBuffer_;
using oclBuffer = std::shared_ptr<oclBuffer_>;
class oclGLBuffer_;
using oclGLBuffer = std::shared_ptr<oclGLBuffer_>;
class oclGLInterBuf_;
using oclGLInterBuf = std::shared_ptr<oclGLInterBuf_>;
class oclImage_;
using oclImage = std::shared_ptr<oclImage_>;
class oclImage2D_;
using oclImg2D = std::shared_ptr<oclImage2D_>;
class oclImage3D_;
using oclImg3D = std::shared_ptr<oclImage3D_>;
class oclGLInterImg2D_;
using oclGLInterImg2D = std::shared_ptr<oclGLInterImg2D_>;
class oclGLInterImg3D_;
using oclGLInterImg3D = std::shared_ptr<oclGLInterImg3D_>;
class oclMem_;
class oclMapPtr_;
class GLResLocker;
class GLInterOP;
class GLSharedCore;

namespace detail
{
template<typename T>
bool owner_equals(const std::shared_ptr<T>& lhs, const std::weak_ptr<T>& rhs)
{
    return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}
template<typename T>
bool owner_equals(const std::weak_ptr<T>& lhs, const std::shared_ptr<T>& rhs)
{
    return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}
}

}

#ifdef OCLU_EXPORT
#include "MiniLogger/MiniLogger.h"
namespace oclu
{
common::mlog::MiniLogger<false>& oclLog();
}
#endif


#define MAKE_ENABLER() struct make_enabler

#define MAKE_ENABLER_IMPL(clz)                  \
struct clz::make_enabler : public clz           \
{                                               \
    template<typename... Args>                  \
    make_enabler(Args... args) :                \
        clz(std::forward<Args>(args)...) { }    \
};                                              \

#define MAKE_ENABLER_SHARED(clz, ...) std::static_pointer_cast<clz>(std::make_shared<clz::make_enabler>(__VA_ARGS__))
#define MAKE_ENABLER_SHARED_CONST(clz, ...) std::static_pointer_cast<const clz>(std::make_shared<clz::make_enabler>(__VA_ARGS__))

#define MAKE_ENABLER_UNIQUE(clz, ...) std::make_unique<clz::make_enabler>(__VA_ARGS__)
