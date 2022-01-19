#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef RENDERCORE_EXPORT
#   define RENDERCOREAPI _declspec(dllexport)
# else
#   define RENDERCOREAPI _declspec(dllimport)
# endif
#else
# define RENDERCOREAPI [[gnu::visibility("default")]]
#endif


#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenCLUtil/oclContext.h"
#include "OpenCLUtil/oclCmdQue.h"
#include "ImageUtil/ImageCore.h"
#include "ResourcePackager/SerializeUtil.h"
#include "SystemCommon/PromiseTask.h"
#include "common/CommonRely.hpp"
#include "common/math/VecBase.hpp"
#include "common/math/MatBase.hpp"
#include "common/math/3DUtil.hpp"
#include "common/EnumEx.hpp"
#include "common/AtomicUtil.hpp"
#include "common/Controllable.hpp"
#include "common/ContainerEx.hpp"
#include "SystemCommon/Exceptions.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <array>
#include <map>
#include <variant>
#include <set>
#include <typeindex>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace dizz
{
namespace fs = common::fs;
namespace math = common::math;
namespace mbase = common::math::base;
using std::wstring;
using std::string;
using std::u16string;
using std::string_view;
using std::u16string_view;
using common::BaseException;

template<class T, class... Args>
using CallbackInvoke = std::function<void(std::function<T(Args...)>)>;


}

namespace oglu::texutil
{

class TexMipmap;
class TexResizer;
class TexUtilWorker;

}
