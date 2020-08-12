#pragma once
#include "dxPch.h"
#include "dxDevice.h"


namespace dxu
{

struct DXDevice_::AdapterProxy : public IDXGIAdapter1
{ };
struct DXDevice_::DeviceProxy : public ID3D12Device
{ };

}