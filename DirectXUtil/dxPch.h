#pragma once
#include "dxRely.h"
#include "StringUtil/Convert.h"
#include "SystemCommon/HResultHelper.h"
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
#include <d3d12.h>
#include <dxgi1_6.h>




#define THROW_HR(eval, msg) if (const long hr = eval; hr < 0) COMMON_THROWEX(DXException, hr, msg)



//forceinline IDXGIAdapter1* GetAdapter(uintptr_t adapter) noexcept
//{
//    return reinterpret_cast<IDXGIAdapter1*>(adapter);
//}
//forceinline ID3D12Device* GetDevice(uintptr_t device) noexcept
//{
//    return reinterpret_cast<ID3D12Device*>(device);
//}

namespace dxu
{

common::mlog::MiniLogger<false>& dxLog();

}
