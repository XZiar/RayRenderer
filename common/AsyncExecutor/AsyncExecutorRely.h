#pragma once

#ifdef ASYEXE_EXPORT
#   define ASYEXEAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define ASYEXEAPI _declspec(dllimport)
#endif

#include <cstdint>
#include <string>
#include <cstring>
#include <memory>

#include "../Exceptions.hpp"
#include "../PromiseTask.h"

namespace common
{
namespace asyexe
{

class AsyncAgent;
class AsyncManager;

using PmsCore = std::shared_ptr<::common::detail::PromiseResultCore>;
using AsyncTaskFunc = std::function<void(const AsyncAgent&)>;


}
}