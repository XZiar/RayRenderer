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
#   define OCLUAPI [[gnu::visibility("default")]]
#   define COMMON_EXPORT
# else
#   define OCLUAPI
# endif
#endif

#include "MiniLogger/MiniLogger.h"
#include "ImageUtil/ImageCore.h"
#include "ImageUtil/TexFormat.h"

#include "common/CommonRely.hpp"
#include "common/ContainerEx.hpp"
#include "common/Delegate.hpp"
#include "common/EnumEx.hpp"
#include "common/Exceptions.hpp"
#include "common/PromiseTask.hpp"
#include "gsl/gsl_assert"

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
#include <variant>


#define CL_TARGET_OPENCL_VERSION 220
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#include "3rdParty/CL/opencl.h"
#include "3rdParty/CL/cl_ext_intel.h"

namespace oclu
{

class oclUtil;
class oclMapPtr_;
class GLInterop;

enum class Vendors { Other = 0, NVIDIA, Intel, AMD };


class oclPromiseStub
{
private:
    std::variant<
        std::reference_wrapper<const common::PromiseResult<void>>,
        std::reference_wrapper<const std::vector<common::PromiseResult<void>>>,
        std::reference_wrapper<const std::initializer_list<common::PromiseResult<void>>>
    > Promises;
public:
    constexpr oclPromiseStub(const common::PromiseResult<void>& promise) noexcept : 
        Promises(promise ) { }
    constexpr oclPromiseStub(const std::vector<common::PromiseResult<void>>& promises) noexcept :
        Promises(promises) { }
    constexpr oclPromiseStub(const std::initializer_list<common::PromiseResult<void>>& promises) noexcept :
        Promises(promises) { }

    constexpr size_t size() const noexcept
    {
        switch (Promises.index())
        {
        case 0: return 1;
        case 1: return std::get<1>(Promises).get().size();
        case 2: return std::get<2>(Promises).get().size();
        default:return 0;
        }
    }

    template<typename T>
    constexpr std::vector<std::shared_ptr<T>> FilterOut() const noexcept
    {
        std::vector<std::shared_ptr<T>> ret;
        switch (Promises.index())
        {
        case 0:
            if (auto pms = std::dynamic_pointer_cast<T>(std::get<0>(Promises).get()); pms)
                ret.push_back(pms);
            break;
        case 1:
            for (const auto& pms : std::get<1>(Promises).get())
                if (auto pms2 = std::dynamic_pointer_cast<T>(pms); pms2)
                    ret.push_back(pms2);
            break;
        case 2:
            for (const auto& pms : std::get<2>(Promises).get())
                if (auto pms2 = std::dynamic_pointer_cast<T>(pms); pms2)
                    ret.push_back(pms2);
            break;
        default:
            break;
        }
        return ret;
    }
};

}

