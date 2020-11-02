#pragma once
#include "DxPch.h"
#include "DxDevice.h"
#include "DxCmdQue.h"
#include "DxResource.h"
#include "DxShader.h"


namespace dxu
{

#define DxProxy(clz, type, dxtype)  \
struct clz::type : public dxtype    \
{                                   \
    using RealType = dxtype;        \
}                                   \


DxProxy(DxDevice_,          AdapterProxy,           IDXGIAdapter1);
DxProxy(DxDevice_,          DeviceProxy,            ID3D12Device);

DxProxy(DxCmdQue_,          CmdQueProxy,            ID3D12CommandQueue);
DxProxy(DxCmdQue_,          FenceProxy,             ID3D12Fence);

DxProxy(DxCmdList_,         CmdAllocatorProxy,      ID3D12CommandAllocator);
// althought have different list types, they all called "GraphicsCommandList"
DxProxy(DxCmdList_,         CmdListProxy,           ID3D12GraphicsCommandList);

DxProxy(DxResource_,        ResDesc,                D3D12_RESOURCE_DESC);
DxProxy(DxResource_,        ResProxy,               ID3D12Resource);


}