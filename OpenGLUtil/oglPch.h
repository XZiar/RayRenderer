#pragma once
#include "oglRely.h"
#include "GLFuncWrapper.h"
#include "SystemCommon/StringFormat.h"
#include "SystemCommon/StringConvert.h"
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


namespace oglu
{
namespace msimd = common::math::simd;

namespace detail
{
class AttribList
{
    std::vector<int32_t> Attribs;
public:
    AttribList(int32_t ending = 0) noexcept : Attribs{ ending } {}
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
        Attribs.push_back(0);
        return true;
    }
    const int32_t* Data() const noexcept
    {
        return Attribs.data();
    }
};
}

common::mlog::MiniLogger<false>& oglLog();
}
