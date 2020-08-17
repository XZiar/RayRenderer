#pragma once
#include "dxPch.h"
#include "dxDevice.h"
#include "dxCmdQue.h"
#include "dxResource.h"


namespace dxu
{

#define DxProxy(clz, type, dxtype)  \
struct clz::type : public dxtype    \
{                                   \
    using RealType = dxtype;        \
}                                   \


DxProxy(DXDevice_,          AdapterProxy,           IDXGIAdapter1);
DxProxy(DXDevice_,          DeviceProxy,            ID3D12Device);

DxProxy(DXCopyCmdQue_,      CmdQueProxy,            ID3D12CommandQueue);

DxProxy(DXCmdList_,         CmdAllocatorProxy,      ID3D12CommandAllocator);
DxProxy(DXCmdList_,         CmdListProxy,           ID3D12CommandList);
DxProxy(DXDirectCmdList_,   GraphicsCmdListProxy,   ID3D12GraphicsCommandList);

DxProxy(DXResource_,        ResDesc,                D3D12_RESOURCE_DESC);
DxProxy(DXResource_,        ResProxy,               ID3D12Resource);


#undef DxProxy


}