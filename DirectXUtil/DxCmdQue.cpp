#include "DxPch.h"
#include "DxCmdQue.h"
#include "ProxyStruct.h"

namespace dxu
{
MAKE_ENABLER_IMPL(DxCopyCmdList_);
MAKE_ENABLER_IMPL(DxComputeCmdList_);
MAKE_ENABLER_IMPL(DxDirectCmdList_);
MAKE_ENABLER_IMPL(DxCopyCmdQue_);
MAKE_ENABLER_IMPL(DxComputeCmdQue_);
MAKE_ENABLER_IMPL(DxDirectCmdQue_);


DxCmdList_::DxCmdList_(DxDevice device, ListType type, const detail::IIDPPVPair& thelist)
{
    D3D12_COMMAND_LIST_TYPE listType = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
    switch (type)
    {
    case ListType::Copy:    listType = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;    break;
    case ListType::Compute: listType = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE; break;
    case ListType::Direct:  listType = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;  break;
    default:                COMMON_THROW(DxException, u"Unrecognized command list type");
    }
    THROW_HR(device->Device->CreateCommandAllocator(listType, IID_PPV_ARGS(&CmdAllocator)), u"Failed to create command allocator");
    THROW_HR(device->Device->CreateCommandList(0, listType, CmdAllocator, nullptr, thelist.TheIID, thelist.PtrObj), u"Failed to create command list");
}
DxCmdList_::DxCmdList_(DxDevice device, ListType type) : DxCmdList_(device, type, { IID_PPV_ARGS(&CmdList) })
{ }
DxCmdList_::~DxCmdList_()
{
    if (CmdList)
        CmdList->Release();
    if (CmdAllocator)
        CmdAllocator->Release();
}

DxCopyCmdList DxCopyCmdList_::Create(DxDevice device)
{
    return MAKE_ENABLER_SHARED(DxCopyCmdList_, (device, ListType::Copy));
}

DxComputeCmdList DxComputeCmdList_::Create(DxDevice device)
{
    return MAKE_ENABLER_SHARED(DxComputeCmdList_, (device, ListType::Compute));
}

DxDirectCmdList_::DxDirectCmdList_(DxDevice device) : DxCmdList_(device, ListType::Direct,
    { IID_PPV_ARGS(&CmdList.AsDerive<GraphicsCmdListProxy>()) })
{ }

DxDirectCmdList DxDirectCmdList_::Create(DxDevice device)
{
    return MAKE_ENABLER_SHARED(DxDirectCmdList_, (device));
}


DxCmdQue_::DxCmdQue_(DxDevice device, QueType type, bool diableTimeout) : FenceNum(0)
{ 
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Flags = diableTimeout ? D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT : D3D12_COMMAND_QUEUE_FLAG_NONE;
    switch (type)
    {
    case QueType::Copy:     desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;    break;
    case QueType::Compute:  desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE; break;
    case QueType::Direct:   desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;  break;
    default:                COMMON_THROW(DxException, u"Unrecognized command que type");
    }
    THROW_HR(device->Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&CmdQue)), u"Failed to create command que");
    THROW_HR(device->Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)), u"Failed to create fence");
}
DxCmdQue_::~DxCmdQue_()
{
    if (CmdQue)
        CmdQue->Release();
    if (Fence)
        Fence->Release();
}

void DxCmdQue_::ExecuteList(const DxCmdList_& list) const
{
    CmdQue->ExecuteCommandLists(1, &list.CmdList);
}

common::PromiseResult<void> DxCmdQue_::Signal() const
{
    auto num = FenceNum++;
    if (const common::HResultHolder hr = CmdQue->Signal(Fence, num); !hr)
        return DxPromise<void>::CreateError(CREATE_EXCEPTION(DxException, hr, u""));
    return DxPromise<void>::Create(*this, num);
}

void DxCmdQue_::Wait(const common::PromiseProvider& pms) const
{
    if (const auto dxPms = dynamic_cast<const DxPromiseCore*>(&pms); dxPms)
    {
        if (dxPms->CmdQue.get() == this)
        {
            Expects(FenceNum >= dxPms->Num);
            // https://stackoverflow.com/questions/52385998/in-dx12-what-ordering-guarantees-do-multiple-executecommandlists-calls-provide?rq=1
            // multiple calls to ExecuteCommandLists is ensured to be serailized, no need to wait
        }
        else
        {
            Expects(dxPms->CmdQue->FenceNum >= dxPms->Num);
            CmdQue->Wait(dxPms->CmdQue->Fence, dxPms->Num);
        }
    }
    else
    {
        dxLog().warning(u"Non-dx promise detected to be wait for, will be ignored!\n");
    }
}

DxCopyCmdQue DxCopyCmdQue_::Create(DxDevice device, bool diableTimeout)
{
    return MAKE_ENABLER_SHARED(DxCopyCmdQue_, (device, QueType::Copy, diableTimeout));
}

DxComputeCmdQue DxComputeCmdQue_::Create(DxDevice device, bool diableTimeout)
{
    return MAKE_ENABLER_SHARED(DxComputeCmdQue_, (device, QueType::Compute, diableTimeout));
}

DxDirectCmdQue DxDirectCmdQue_::Create(DxDevice device, bool diableTimeout)
{
    return MAKE_ENABLER_SHARED(DxDirectCmdQue_, (device, QueType::Direct, diableTimeout));
}

}
