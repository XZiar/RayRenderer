#pragma once
#include "oglRely.h"
#include "GLFuncWrapper.h"
#include "SystemCommon/StringFormat.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/DynamicLibrary.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/ThreadEx.h"
#include "SystemCommon/StackTrace.h"
#include "SystemCommon/CopyEx.h"
#include "SystemCommon/MiniLogger.h"
#include "ImageUtil/ImageUtil.h"
#include "ImageUtil/TexFormat.h"
#include "common/Linq2.hpp"
#include "common/StringEx.hpp"
#include "common/StringLinq.hpp"
#include "common/ContainerEx.hpp"
#include "common/math/VecSIMD.hpp"
#include "common/math/MatSIMD.hpp"

#include <algorithm>


#define DEFINE_FUNC(func, name) using T_P##name = decltype(&func); static constexpr auto N_##name = #func ""sv
#define DEFINE_FUNC2(type, func, name) using T_P##name = type; static constexpr auto N_##name = #func ""sv
#define DECLARE_FUNC(name) T_P##name name = nullptr
#define LOAD_FUNC(lib, name) name = Lib##lib.GetFunction<T_P##name>(N_##name)


namespace oglu
{
namespace msimd = common::math::simd;

namespace detail
{

class AttribList
{
    std::vector<int32_t> Attribs;
    int32_t Ending;
public:
    AttribList(int32_t ending = 0) noexcept : Attribs{ ending }, Ending(ending) {}
    bool Set(int32_t key, int32_t val) noexcept
    {
        for (size_t i = 0; i + 1 < Attribs.size(); i += 2)
        {
            if (Attribs[i] == key)
            {
                Attribs[i + 1] = val;
                return false;
            }
        }
        Attribs.back() = key;
        Attribs.push_back(val);
        Attribs.push_back(Ending);
        return true;
    }
    const int32_t* Data() const noexcept
    {
        return Attribs.data();
    }
};

void RegisterLoader(std::string_view name, std::function<std::unique_ptr<oglLoader>()> creator) noexcept;
template<typename T>
inline bool RegisterLoader() noexcept
{
    static_assert(std::is_base_of_v<oglLoader, T>);
    RegisterLoader(T::LoaderName, []() {return std::make_unique<T>(); });
    return true;
}

//const common::container::FrozenDenseSet<std::string_view> GLBasicAPIs;

}

common::mlog::MiniLogger<false>& oglLog();
}
