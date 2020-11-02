#pragma once
#include "DxRely.h"
#include "StringUtil/Convert.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/ThreadEx.h"
#include "MiniLogger/MiniLogger.h"
#include "ImageUtil/ImageUtil.h"
#include "ImageUtil/TexFormat.h"
#include "common/Exceptions.hpp"
#include "common/Linq2.hpp"
#include "common/StringEx.hpp"
#include "common/StringLinq.hpp"
#include "common/CopyEx.hpp"
#include "common/ContainerEx.hpp"

#include "boost.stacktrace/stacktrace.h"

#include <algorithm>

#define COM_NO_WINDOWS_H
#define NOMINMAX 1
#include <d3d12.h>
#include <d3d12shader.h>
#include <dxgi1_6.h>
#include "d3dx12.h"




#define THROW_HR(eval, msg) if (const common::HResultHolder hr___ = eval; !hr___) COMMON_THROWEX(DxException, hr___, msg)



namespace dxu
{

namespace detail
{

struct IIDPPVPair
{
    REFIID TheIID;
    void** PtrObj;
};

struct IIDData
{
    REFIID TheIID;
};


template<typename T>
struct OptRet
{
    T Data;
    common::HResultHolder HResult;
    constexpr explicit operator bool() const noexcept { return (bool)HResult; }
          T* operator->() noexcept
    {
        return &Data;
    }
    const T* operator->() const noexcept
    {
        return &Data;
    }
};

}

common::mlog::MiniLogger<false>& dxLog();

}
