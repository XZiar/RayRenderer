#include "DxPch.h"
#include "DxQuery.h"
#include "DxCmdQue.h"
#include "DxBuffer.h"


namespace dxu
{
using common::enum_cast;


DxQueryProvider::DxQueryProvider(DxDevice dev) : Device(dev), TotalSize(0)
{ }
DxQueryProvider::~DxQueryProvider()
{
    for (auto& heap : Heaps)
    {
        Expects(heap.Heap);
        heap.Heap->Release();
        heap.Heap = nullptr;
    }
}

DxQueryProvider::QueryBlock* DxQueryProvider::GetQueryBlock(uint8_t type, uint32_t initSize)
{
    for (auto& heap : boost::adaptors::reverse(Heaps))
    {
        if (heap.Type != type)
            continue;
        if (heap.Usage == heap.Size) // full
            break;
        return &heap;
    }
    D3D12_QUERY_HEAP_DESC heapDesc =
    {
        static_cast<D3D12_QUERY_HEAP_TYPE>(type),
        initSize,
        0
    };
    auto& heap = Heaps.emplace_back(initSize, type);
    Device->Device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(reinterpret_cast<ID3D12QueryHeap**>(&heap.Heap)));
    return &heap;
}

DxTimestampToken DxQueryProvider::AllocateTimeQuery(DxCmdList_& cmdList)
{
    const auto heap = GetQueryBlock(static_cast<uint8_t>(D3D12_QUERY_HEAP_TYPE_TIMESTAMP));
    const auto offset = heap->Usage++;
    cmdList.CmdList->EndQuery(heap->Heap, D3D12_QUERY_TYPE_TIMESTAMP, offset);
    return { this, gsl::narrow_cast<uint32_t>(heap - &Heaps.front()), offset };
}

void DxQueryProvider::Finish()
{
    TotalSize = 0;
    for (auto& heap : Heaps)
    {
        heap.Offset = TotalSize;
        switch (heap.Type)
        {
        case D3D12_QUERY_HEAP_TYPE_TIMESTAMP:
            TotalSize += heap.Usage * 1;
            break;
        default:
            break;
        }
    }
}

DxQueryProvider::ResolveRecord DxQueryProvider::GenerateResolve(const DxCmdQue_& que) const
{
    auto cmdList = que.CreateList();
    auto buf = DxBuffer_::Create(Device, HeapType::Readback, HeapFlags::Empty, TotalSize * sizeof(uint64_t), ResourceFlags::Empty);
    for (const auto& heap : Heaps)
    {
        if (heap.Usage == 0)
            continue;
        switch (heap.Type)
        {
        case D3D12_QUERY_HEAP_TYPE_TIMESTAMP:
            cmdList->CmdList->ResolveQueryData(heap.Heap, D3D12_QUERY_TYPE_TIMESTAMP, 0, heap.Usage, buf->Resource, heap.Offset * sizeof(uint64_t));
            break;
        default:
            break;
        }
    }
    cmdList->Close();
    uint64_t timestampFreq = 0;
    THROW_HR(que.CmdQue->GetTimestampFrequency(&timestampFreq), u"Failed to get timestamp frequency");
    return { shared_from_this(), std::move(buf), std::move(cmdList), timestampFreq };
}


DxQueryResolver::DxQueryResolver(DxQueryProvider::ResolveRecord& record) :
    Provider(std::move(record.Provider)), Result(std::make_unique<uint64_t[]>(Provider->TotalSize)), 
    TimestampRatio(10e9 / static_cast<double>(record.TimestampFreq))
{ 
    const auto bytes = Provider->TotalSize * sizeof(uint64_t);
    const auto result = record.Result->Map(0, bytes);
    memcpy_s(Result.get(), bytes, result.Get().data(), bytes);
}

uint64_t DxQueryResolver::ResolveQuery(const DxTimestampToken& token) const
{
    Expects(token.Provider == Provider.get());
    const auto idx = Provider->Heaps[token.BlockIdx].Offset + token.BlockOffset * 1;
    const auto tick = Result[idx];
    return static_cast<uint64_t>(tick * TimestampRatio);
}



}
