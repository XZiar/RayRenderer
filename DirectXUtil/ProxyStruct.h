#pragma once
#include "dxPch.h"
#include "dxDevice.h"
#include "dxCmdQue.h"


namespace dxu
{

struct DXDevice_::AdapterProxy : public IDXGIAdapter1
{ };
struct DXDevice_::DeviceProxy : public ID3D12Device
{ };
struct DXCmdQue_::CmdQueProxy : public ID3D12CommandQueue
{ };

}