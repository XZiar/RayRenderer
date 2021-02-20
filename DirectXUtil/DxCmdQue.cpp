#include "DxPch.h"
#include "DxCmdQue.h"
#include "DxResource.h"

namespace dxu
{
MAKE_ENABLER_IMPL(DxCopyCmdList_);
MAKE_ENABLER_IMPL(DxComputeCmdList_);
MAKE_ENABLER_IMPL(DxDirectCmdList_);
MAKE_ENABLER_IMPL(DxCopyCmdQue_);
MAKE_ENABLER_IMPL(DxComputeCmdQue_);
MAKE_ENABLER_IMPL(DxDirectCmdQue_);


constexpr static bool IsReadOnlyState(const ResourceState state)
{
    return !HAS_FIELD(state, ~ResourceState::Read);
}

DxCmdList_::ResStateRecord::ResStateRecord(const DxResource_* res, const ResourceState state) noexcept :
    Resource(res->Resource), State(state), IsPromote(state == ResourceState::Common), IsBufOrSATex(res->IsBufOrSATex())
{ }


DxCmdList_::DxCmdList_(DxDevice device, ListType type, const DxCmdList_* prevList) : Type(type), HasClosed{false}
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
        if (!prevList->IsClosed())
            COMMON_THROW(DxException, u"PrevList not closed");
        ResStateTable = prevList->ResStateTable;
    }
}
DxCmdList_::~DxCmdList_()
{
}

void* DxCmdList_::GetD3D12Object() const noexcept
{
    return static_cast<ID3D12Object*>(CmdList.Ptr());
}

bool DxCmdList_::UpdateResState(const DxResource_* res, const ResourceState newState, bool fromInitState)
{
    static constexpr auto CopyAllowState    = ResourceState::CopyDst | ResourceState::CopySrc;
    static constexpr auto ComputeAllowState =
        ResourceState::UnorderAccess | ResourceState::NonPSRes | ResourceState::CopyDst | ResourceState::CopySrc;
    // capability check
    switch (Type)
    {
    case ListType::Copy:
        if (HAS_FIELD(newState, ~CopyAllowState))
            COMMON_THROW(DxException, FMTSTR(u"Unsupported type [{}] for Copy List", common::enum_cast(newState)));
        break;
    case ListType::Compute:
        if (HAS_FIELD(newState, ~ComputeAllowState))
            COMMON_THROW(DxException, FMTSTR(u"Unsupported type [{}] for Compute List", common::enum_cast(newState)));
        break;
    default:
        break;
    }

    ResStateRecord* record = nullptr;
    for (auto& item : ResStateTable)
    {
        if (item.Resource == res)
        {
            record = &item;
            break;
        }
    }
    if (!record)
    {
        record = &ResStateTable.emplace_back(res, fromInitState ? res->InitState : ResourceState::Common);
    }
    const auto oldState = record->State;
    if (oldState == newState) // fast skip
        return true;

    bool isPromote = false;
    if (oldState == ResourceState::Common)
    {
        static constexpr auto BSATState = ResourceState::ConstBuffer | ResourceState::IndexBuffer | ResourceState::RenderTarget |
            ResourceState::UnorderAccess | ResourceState::NonPSRes | ResourceState::PsRes | ResourceState::StreamOut |
            ResourceState::IndirectArg | ResourceState::CopyDst | ResourceState::CopySrc |
            ResourceState::ResolveSrc | ResourceState::ResolveDst | ResourceState::Pred;
        static constexpr auto NonBSATState = ResourceState::NonPSRes | ResourceState::PsRes | ResourceState::CopyDst | ResourceState::CopySrc;
        if (const auto pmtState = record->IsBufOrSATex ? BSATState : NonBSATState; !HAS_FIELD(newState, ~pmtState))
        { // Resources can only be "promoted" out of D3D12_RESOURCE_STATE_COMMON
            isPromote = true;
        }
    }
    else if (record->IsPromote)
    {
        if (IsReadOnlyState(oldState) && IsReadOnlyState(newState))
        { // promotion from one promoted read state into multiple read state is valid
            isPromote = true;
        }
    }

    record->IsPromote = isPromote;
    record->State     = newState;
    if (!isPromote)
    {
        D3D12_RESOURCE_TRANSITION_BARRIER trans =
        {
            res->Resource,
            0,
            static_cast<D3D12_RESOURCE_STATES>(oldState),
            static_cast<D3D12_RESOURCE_STATES>(newState)
        };
        D3D12_RESOURCE_BARRIER barrier =
        {
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            D3D12_RESOURCE_BARRIER_FLAG_NONE,
            trans
        };
        CmdList->ResourceBarrier(1, &barrier);
    }
    return false;
}

bool DxCmdList_::IsClosed() const noexcept
{
    return HasClosed.load();
}

void DxCmdList_::EnsureClosed()
{
    if (!HasClosed.exchange(true))
    {
        THROW_HR(CmdList->Close(), u"Failed to close CmdList");
        // Decay states
        if (Type == ListType::Copy) // Resources being accessed on a Copy queue
            ResStateTable.clear();
        else
        {
            std::vector<ResStateRecord> newStates;
            for (const auto& record : ResStateTable)
            {
                if (record.IsBufOrSATex)
                    // Buffer resources on any queue type, or
                    // Texture resources on any queue type that have the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set
                    continue;
                if (record.IsPromote && IsReadOnlyState(record.State)) // Any resource implicitly promoted to a read-only state
                    continue;
                newStates.emplace_back(record);
            }
            ResStateTable.swap(newStates);
        }
    }
}

void DxCmdList_::Reset(const bool resetResState)
{
    THROW_HR(CmdList->Reset(CmdAllocator, nullptr), u"Failed to reset CmdList");
    HasClosed.store(false);
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

void* DxCmdQue_::GetD3D12Object() const noexcept
{
    return static_cast<ID3D12Object*>(CmdQue.Ptr());
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
