#include "DxPch.h"
#include "DxCmdQue.h"
#include "DxResource.h"

namespace dxu
{
MAKE_ENABLER_IMPL(DxQueryProvider);
MAKE_ENABLER_IMPL(DxCopyCmdList_);
MAKE_ENABLER_IMPL(DxComputeCmdList_);
MAKE_ENABLER_IMPL(DxDirectCmdList_);
MAKE_ENABLER_IMPL(DxCopyCmdQue_);
MAKE_ENABLER_IMPL(DxComputeCmdQue_);
MAKE_ENABLER_IMPL(DxDirectCmdQue_);
using common::enum_cast;


namespace detail
{
struct DebugEventHolder::Range
{
    std::shared_ptr<DebugEventHolder> Host;
    std::shared_ptr<Range> Previous;
    Range(std::shared_ptr<DebugEventHolder> host) :
        Host(std::move(host)), Previous(Host->CurrentRange.lock())
    { }
    ~Range()
    {
        Host->EndEvent();
    }
};

DebugEventHolder::~DebugEventHolder()
{}
bool DebugEventHolder::CheckRangeEmpty() const noexcept
{
    return CurrentRange.expired();
}
std::shared_ptr<void> DebugEventHolder::DeclareRange(std::u16string msg)
{
    BeginEvent(msg);
    auto range = std::make_shared<Range>(shared_from_this());
    CurrentRange = range;
    return range;
}
}


void ResStateList::AddState(const DxResource_* res, ResourceState state)
{
    for (auto& record : Records)
    {
        if (record.Resource == res)
        {
            record.State = state;
            return;
        }
    }
    Records.push_back({ res, state });
}


constexpr static bool IsReadOnlyState(const ResourceState state)
{
    return !HAS_FIELD(state, ~ResourceState::Read);
}


DxCmdList_::ResStateRecord::ResStateRecord(const DxResource_* res, const ResourceState state) noexcept :
    Resource(res), State(state), FromState(ResourceState::Invalid), IsPromote(state == ResourceState::Common), IsBufOrSATex(res->IsBufOrSATex())
{ }


DxCmdList_::DxCmdList_(DxDevice device, ListType type) : Device(device), Type(type), HasClosed{false}
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
}
DxCmdList_::~DxCmdList_()
{
}

void* DxCmdList_::GetD3D12Object() const noexcept
{
    return static_cast<ID3D12Object*>(CmdList.Ptr());
}
void DxCmdList_::BeginEvent(std::u16string_view msg) const
{
    detail::pix::BeginEvent(CmdList.Ptr(), 0, msg);
}
void DxCmdList_::EndEvent() const
{
    detail::pix::EndEvent(CmdList.Ptr());
}
void DxCmdList_::AddMarker(std::u16string name) const
{
    detail::pix::SetMarker(CmdList.Ptr(), 0, name);
}

void DxCmdList_::InitResTable(const ResStateList& list)
{
    ResStateTable.reserve(list.Records.size());
    for (const auto& record : list.Records)
    {
        ResStateTable.emplace_back(record.Resource, record.State);
    }
}

void DxCmdList_::InitResTable(const DxCmdList_& list)
{
    if (!list.IsClosed())
        COMMON_THROW(DxException, u"PrevList not closed");
    ResStateTable = list.ResStateTable;
}

DxQueryProvider& DxCmdList_::GetQueryProvider()
{
    if (!QueryProvider)
        QueryProvider = MAKE_ENABLER_SHARED(DxQueryProvider, (Device));
    return *QueryProvider;
}

bool DxCmdList_::UpdateResState(const DxResource_* res, const ResourceState newState, bool fromInitState)
{
    static constexpr auto CopyAllowState    = ResourceState::CopyDst | ResourceState::CopySrc;
    static constexpr auto ComputeAllowState = ResourceState::ConstBuffer |
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
        const auto state = fromInitState ? res->InitState : [](const DxResource_& res) 
        {
            switch (res.HeapInfo.Type)
            {
            case HeapType::Upload:      return ResourceState::Read;
            case HeapType::Readback:    return ResourceState::CopyDst;
            default:                    return ResourceState::Common;
            }
        }(*res);
        record = &ResStateTable.emplace_back(res, state);
    }
    const auto oldState = record->State;
    if (oldState == newState) // fast skip
        return true;

    if (record->FromState != ResourceState::Invalid) // already transisted
    {
        dxLog().warning(FMT_STRING(u"Resource [{}] was repeatly transit from [{:0X}]->...->[{:0X}]->[{:0X}]."),
            res->GetName(), enum_cast(record->FromState), enum_cast(oldState), enum_cast(newState));
    }
    else
    {
        record->FromState = oldState;
    }

    bool isPromote = false;
    if (oldState == ResourceState::Common)
    { // Resources can only be "promoted" out of D3D12_RESOURCE_STATE_COMMON
        static constexpr auto BSATState = ResourceState::ConstBuffer | ResourceState::IndexBuffer | ResourceState::RenderTarget |
            ResourceState::UnorderAccess | ResourceState::NonPSRes | ResourceState::PsRes | ResourceState::StreamOut |
            ResourceState::IndirectArg | ResourceState::CopyDst | ResourceState::CopySrc |
            ResourceState::ResolveSrc | ResourceState::ResolveDst | ResourceState::Pred;
        static constexpr auto NonBSATState = ResourceState::NonPSRes | ResourceState::PsRes | ResourceState::CopyDst | ResourceState::CopySrc;
        if (const auto pmtState = record->IsBufOrSATex ? BSATState : NonBSATState; !HAS_FIELD(newState, ~pmtState))
        { 
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
    /*if (!isPromote)
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
    }*/
    return false;
}

void DxCmdList_::FlushResourceState()
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    for (auto& record : ResStateTable)
    {
        if (record.FromState != ResourceState::Invalid && !record.IsPromote)
        {
            auto& barrier = barriers.emplace_back();
            barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource   = record.Resource->Resource;
            barrier.Transition.Subresource = 0;
            barrier.Transition.StateBefore = static_cast<D3D12_RESOURCE_STATES>(record.FromState);
            barrier.Transition.StateAfter  = static_cast<D3D12_RESOURCE_STATES>(record.State);
        }
        record.FromState = ResourceState::Invalid;
    }
    if (barriers.size() > 0)
    {
        CmdList->ResourceBarrier(gsl::narrow_cast<uint32_t>(barriers.size()), barriers.data());
    }
}

ResStateList DxCmdList_::GenerateStateList() const
{
    if (!IsClosed())
        COMMON_THROW(DxException, u"CmdList not closed");
    ResStateList list;
    for (const auto& record : ResStateTable)
    {
        list.AddState(record.Resource, record.State);
    }
    return list;
}

void DxCmdList_::Close()
{
    if (!HasClosed)
    {
        if (!CheckRangeEmpty())
            dxLog().error(u"some range not closed with cmdlist [{}]", GetName());
        THROW_HR(CmdList->Close(), u"Failed to close CmdList");
        if (QueryProvider)
            QueryProvider->Finish();
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
        HasClosed = true;
    }
}

void DxCmdList_::Reset(const bool resetResState)
{
    THROW_HR(CmdList->Reset(CmdAllocator, nullptr), u"Failed to reset CmdList");
    HasClosed = false;
    QueryProvider.reset();
    if (resetResState)
        ResStateTable.clear();
}


DxCopyCmdList DxCopyCmdList_::Create(DxDevice device)
{
    return MAKE_ENABLER_SHARED(DxCopyCmdList_,    (device, ListType::Copy));
}

DxComputeCmdList DxComputeCmdList_::Create(DxDevice device)
{
    return MAKE_ENABLER_SHARED(DxComputeCmdList_, (device, ListType::Compute));
}

DxDirectCmdList DxDirectCmdList_::Create(DxDevice device)
{
    return MAKE_ENABLER_SHARED(DxDirectCmdList_,  (device, ListType::Direct));
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
void DxCmdQue_::BeginEvent(std::u16string_view msg) const
{
    detail::pix::BeginEvent(CmdQue.Ptr(), 0, msg);
}
void DxCmdQue_::EndEvent() const
{
    detail::pix::EndEvent(CmdQue.Ptr());
}
void DxCmdQue_::AddMarker(std::u16string name) const
{
    detail::pix::SetMarker(CmdQue.Ptr(), 0, name);
}

void DxCmdQue_::EnsureClosed(const DxCmdList_& list)
{
    if (!list.IsClosed())
        COMMON_THROWEX(DxException, u"Cmdlist has not been closed");
}

void DxCmdQue_::ExecuteList(const DxCmdList_& list) const
{
    EnsureClosed(list);
    ID3D12CommandList* ptr = list.CmdList;
    CmdQue->ExecuteCommandLists(1, &ptr);
}

std::variant<uint64_t, DxException> DxCmdQue_::SignalNum() const
{
    auto num = ++FenceNum;
    if (const common::HResultHolder hr = CmdQue->Signal(Fence, num); !hr)
        return CREATE_EXCEPTION(DxException, hr, u"Failed to Signal the cmdque");
    return num;
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

common::PromiseResult<DxQueryResolver> DxCmdQue_::ExecuteAnyListWithQuery(const DxCmdList& list) const
{
    EnsureClosed(*list);
    auto record = list->QueryProvider->GenerateResolve(*this);
    ExecuteList(*list);
    ExecuteList(*record.ResolveList);
    return common::StagedResult::TwoStage(
        Signal<DxQueryProvider::ResolveRecord>(std::move(record)), 
        [](auto record) -> DxQueryResolver { return record; });
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
