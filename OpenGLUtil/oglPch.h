#pragma once
#include "oglRely.h"
#include "GLFuncWrapper.h"
#include "SystemCommon/Format.h"
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
#include "common/EasyIterator.hpp"
#include "common/ContainerEx.hpp"
#include "common/math/VecSIMD.hpp"
#include "common/math/MatSIMD.hpp"

#include <algorithm>


#define FMTSTR2(syntax, ...) common::str::Formatter<char16_t>{}.FormatStatic(FmtString(syntax), __VA_ARGS__)
#define DEFINE_FUNC(func, name) using T_P##name = decltype(&func); static constexpr auto N_##name = #func ""sv
#define DEFINE_FUNC2(type, func, name) using T_P##name = type; static constexpr auto N_##name = #func ""sv
#define DECLARE_FUNC(name) T_P##name name = nullptr
#define LOAD_FUNC(lib, name) name = Lib##lib.GetFunction<T_P##name>(N_##name)
#define TrLd_FUNC(lib, name) name = Lib##lib.TryGetFunction<T_P##name>(N_##name)


namespace oglu
{
namespace msimd = common::math::simd;

namespace detail
{

template<typename T = int32_t>
class AttribList
{
private:
    std::vector<T> Attribs;
    T Ending;
    [[nodiscard]] std::pair<T, T> GetKVPair(size_t idx) const noexcept
    {
        return { Attribs[idx * 2], Attribs[idx * 2 + 1] };
    }
public:
    AttribList(T ending = 0) noexcept : Attribs{ ending }, Ending(ending) {}
    bool Set(T key, T val) noexcept
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
    [[nodiscard]] const T* Data() const noexcept
    {
        return Attribs.data();
    }
    using KVIter = ::common::container::IndirectIterator<const AttribList, std::pair<T, T>, &AttribList::GetKVPair>;
    friend KVIter;
    constexpr KVIter begin() const noexcept { return { this, 0 }; }
              KVIter end()   const noexcept { return { this, Attribs.size() / 2 }; }
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
