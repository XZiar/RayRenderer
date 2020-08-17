#include "dxPch.h"
#include "dxCmdQue.h"
#include "ProxyStruct.h"

namespace dxu
{
MAKE_ENABLER_IMPL(DXCopyCmdList_);
MAKE_ENABLER_IMPL(DXComputeCmdList_);
MAKE_ENABLER_IMPL(DXDirectCmdList_);
MAKE_ENABLER_IMPL(DXCopyCmdQue_);
MAKE_ENABLER_IMPL(DXComputeCmdQue_);
MAKE_ENABLER_IMPL(DXDirectCmdQue_);


DXCmdList_::DXCmdList_(DXDevice device, ListType type, const detail::IIDPPVPair& thelist)
{
    D3D12_COMMAND_LIST_TYPE listType = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
    switch (type)
    {
    case ListType::Copy:    listType = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;    break;
    case ListType::Compute: listType = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE; break;
    case ListType::Direct:  listType = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;  break;
    default:                COMMON_THROW(DXException, u"Unrecognized command list type");
    }
    THROW_HR(device->Device->CreateCommandAllocator(listType, IID_PPV_ARGS(&CmdAllocator)), u"Failed to create command allocator");
    THROW_HR(device->Device->CreateCommandList(0, listType, CmdAllocator, nullptr, thelist.TheIID, thelist.PtrObj), u"Failed to create command list");
}
DXCmdList_::DXCmdList_(DXDevice device, ListType type) : DXCmdList_(device, type, { IID_PPV_ARGS(&CmdList) })
{ }
DXCmdList_::~DXCmdList_()
{
    if (CmdList)
        CmdList->Release();
    if (CmdAllocator)
        CmdAllocator->Release();
}

DXCopyCmdList DXCopyCmdList_::Create(DXDevice device)
{
    return MAKE_ENABLER_SHARED(DXCopyCmdList_, (device, ListType::Copy));
}

DXComputeCmdList DXComputeCmdList_::Create(DXDevice device)
{
    return MAKE_ENABLER_SHARED(DXComputeCmdList_, (device, ListType::Compute));
}

DXDirectCmdList_::DXDirectCmdList_(DXDevice device) : DXComputeCmdList_(device, ListType::Direct, 
    { IID_PPV_ARGS(&CmdList.AsDerive<GraphicsCmdListProxy>()) })
{ }

DXDirectCmdList DXDirectCmdList_::Create(DXDevice device)
{
    return MAKE_ENABLER_SHARED(DXDirectCmdList_, (device));
}


DXCopyCmdQue_::DXCopyCmdQue_(DXDevice device, QueType type, bool diableTimeout)
{ 
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Flags = diableTimeout ? D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT : D3D12_COMMAND_QUEUE_FLAG_NONE;
    switch (type)
    {
    case QueType::Copy:     desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;    break;
    case QueType::Compute:  desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE; break;
    case QueType::Direct:   desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;  break;
    default:                COMMON_THROW(DXException, u"Unrecognized command que type");
    }
    THROW_HR(device->Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&CmdQue)), u"Failed to create command que");
}
DXCopyCmdQue_::~DXCopyCmdQue_()
{
    if (CmdQue)
        CmdQue->Release();
}

void DXCopyCmdQue_::ExecuteList(const DXCmdList_& list) const
{
    CmdQue->ExecuteCommandLists(1, &list.CmdList);
}

DXCopyCmdQue DXCopyCmdQue_::Create(DXDevice device, bool diableTimeout)
{
    return MAKE_ENABLER_SHARED(DXCopyCmdQue_, (device, QueType::Copy, diableTimeout));
}

DXComputeCmdQue DXComputeCmdQue_::Create(DXDevice device, bool diableTimeout)
{
    return MAKE_ENABLER_SHARED(DXComputeCmdQue_, (device, QueType::Compute, diableTimeout));
}

DXDirectCmdQue DXDirectCmdQue_::Create(DXDevice device, bool diableTimeout)
{
    return MAKE_ENABLER_SHARED(DXDirectCmdQue_, (device, QueType::Direct, diableTimeout));
}

}
