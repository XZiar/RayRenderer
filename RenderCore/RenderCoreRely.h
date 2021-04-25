#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef RAYCORE_EXPORT
#   define RAYCOREAPI _declspec(dllexport)
# else
#   define RAYCOREAPI _declspec(dllimport)
# endif
#else
# define RAYCOREAPI [[gnu::visibility("default")]]
#endif


#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenCLUtil/oclContext.h"
#include "OpenCLUtil/oclCmdQue.h"
#include "ImageUtil/ImageCore.h"
#include "ResourcePackager/SerializeUtil.h"
#include "SystemCommon/PromiseTask.h"
#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "common/AtomicUtil.hpp"
#include "common/Controllable.hpp"
#include "common/ContainerEx.hpp"
#include "common/Exceptions.hpp"
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

namespace rayr
{
namespace fs = common::fs;
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
