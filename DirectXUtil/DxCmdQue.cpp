#include "DxPch.h"
#include "DxCmdQue.h"

namespace dxu
{
MAKE_ENABLER_IMPL(DxCopyCmdList_);
MAKE_ENABLER_IMPL(DxComputeCmdList_);
MAKE_ENABLER_IMPL(DxDirectCmdList_);
MAKE_ENABLER_IMPL(DxCopyCmdQue_);
MAKE_ENABLER_IMPL(DxComputeCmdQue_);
MAKE_ENABLER_IMPL(DxDirectCmdQue_);


DxCmdList_::DxCmdList_(DxDevice device, ListType type, const DxCmdList_* prevList) : HasClosed{}
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
    THROW_HR(device->Device->CreateCommandList(0, listType, CmdAllocator, nullptr, IID_PPV_ARGS(&CmdList)),
        u"Failed to create command list");
    if (prevList)
    {
        ResStateTable.reserve(prevList->ResStateTable.size());
        for (const auto [res, state] : ResStateTable)
        {
            if (state != ResourceState::Common)
            {
                ResStateTable.emplace_back(res, state);
            }
        }
    }
}
DxCmdList_::~DxCmdList_()
{
}

std::optional<ResourceState> DxCmdList_::UpdateResState(void* res, const ResourceState state)
{
    for (auto& item : ResStateTable)
    {
        if (item.first == res)
        {
            const auto oldStete = item.second;
            item.second = state;
            return oldStete;
        }
    }
    ResStateTable.emplace_back(res, state);
    return {};
}

void DxCmdList_::EnsureClosed()
{
    if (!HasClosed.test_and_set())
        THROW_HR(CmdList->Close(), u"Failed to close CmdList");
}

void DxCmdList_::Reset(const bool resetResState)
{
    THROW_HR(CmdList->Reset(CmdAllocator, nullptr), u"Failed to reset CmdList");
    HasClosed.clear();
    if (resetResState)
        ResStateTable.clear();
}


DxCopyCmdList DxCopyCmdList_::Create(DxDevice device, const DxCmdList& prevList)
{
    return MAKE_ENABLER_SHARED(DxCopyCmdList_,    (device, ListType::Copy,    prevList.get()));
}

DxComputeCmdList DxComputeCmdList_::Create(DxDevice device, const DxCmdList& prevList)
{
    return MAKE_ENABLER_SHARED(DxComputeCmdList_, (device, ListType::Compute, prevList.get()));
}

DxDirectCmdList DxDirectCmdList_::Create(DxDevice device, const DxCmdList& prevList)
{
    return MAKE_ENABLER_SHARED(DxDirectCmdList_,  (device, ListType::Direct,  prevList.get()));
}


DxCmdQue_::DxCmdQue_(DxDevice device, QueType type, bool diableTimeout) : FenceNum(0), Device(device)
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
}

void DxCmdQue_::ExecuteList(DxCmdList_& list) const
{
    list.EnsureClosed();
    ID3D12CommandList* ptr = list.CmdList;
    CmdQue->ExecuteCommandLists(1, &ptr);
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

DxCmdList DxCmdQue_::CreateList(const DxCmdList& prevList) const
{
    const auto type = GetType();
    switch (type)
    {
    case QueType::Copy:     return DxCopyCmdList_::   Create(Device, prevList);
    case QueType::Compute:  return DxComputeCmdList_::Create(Device, prevList);
    case QueType::Direct:   return DxDirectCmdList_:: Create(Device, prevList);
    default:                COMMON_THROW(DxException, u"Unrecognized command que type");
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
