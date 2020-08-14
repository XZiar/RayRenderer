#include "dxPch.h"
#include "dxCmdQue.h"
#include "ProxyStruct.h"

namespace dxu
{
MAKE_ENABLER_IMPL(DXCmdQue_);
MAKE_ENABLER_IMPL(DXComputeCmdQue_);
MAKE_ENABLER_IMPL(DXDirectCmdQue_);

DXCmdQue_::DXCmdQue_(DXDevice device, QueType type, bool diableTimeout) : CmdQue(nullptr)
{ 
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Flags = diableTimeout ? D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT : D3D12_COMMAND_QUEUE_FLAG_NONE;
    switch (type)
    {
    case QueType::Copy:     desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;    break;
    case QueType::Compute:  desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE; break;
    case QueType::Direct:   desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;  break;
    default:                COMMON_THROW(DXException, u"Unrecognized command que type");
    }
    auto cmdque = reinterpret_cast<ID3D12CommandQueue**>(&CmdQue);
    THROW_HR(device->Device->CreateCommandQueue(&desc, IID_PPV_ARGS(cmdque)), u"Failed to create command que");
}
DXCmdQue_::~DXCmdQue_()
{
    if (CmdQue)
        CmdQue->Release();
}

DXCmdQue DXCmdQue_::Create(DXDevice device, bool diableTimeout)
{
    return MAKE_ENABLER_SHARED(DXCmdQue_, (device, QueType::Copy, diableTimeout));
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
