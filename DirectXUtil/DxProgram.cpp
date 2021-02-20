#include "DxPch.h"
#include "DxProgram.h"
#include "StringUtil/Detect.h"


namespace dxu
{
using namespace std::string_view_literals;
using Microsoft::WRL::ComPtr;
using common::str::Charset;
using common::str::HashedStrView;



DxProgram_::DxProgram_(DxDevice dev) : Device(dev)
{
}
DxProgram_::~DxProgram_()
{ }

enum class FormatTypes : uint16_t { Other = 0, Sampler = 1, Buf = 2, Tex = 3 };
static std::pair<BoundedResourceType, FormatTypes> ParseDescType(const D3D12_SHADER_INPUT_BIND_DESC& desc) noexcept
{
    BoundedResourceType type = BoundedResourceType::OtherType;
    switch (desc.Type)
    {
    case D3D_SIT_CBUFFER:
        return { BoundedResourceType::CBuffer, FormatTypes::Buf };
    case D3D_SIT_TBUFFER:
        type = BoundedResourceType::TBuffer; break;
    case D3D_SIT_TEXTURE:
        type = BoundedResourceType::Texture; break;
    case D3D_SIT_SAMPLER:
        return { BoundedResourceType::Sampler, FormatTypes::Sampler };
    case D3D_SIT_UAV_RWTYPED:
        type = BoundedResourceType::RWTypedUAV; break;
    case D3D_SIT_STRUCTURED:
        type = BoundedResourceType::UntypedUAV; break;
    case D3D_SIT_UAV_RWSTRUCTURED:
        type = BoundedResourceType::RWUntypedUAV; break;
    case D3D_SIT_BYTEADDRESS:
        return { BoundedResourceType::RawUAV, FormatTypes::Buf };
    case D3D_SIT_UAV_RWBYTEADDRESS:
        return { BoundedResourceType::RWRawUAV, FormatTypes::Buf };
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
    default:
        return { BoundedResourceType::Other, FormatTypes::Other };
    }
    switch (desc.Dimension)
    {
    case D3D_SRV_DIMENSION_BUFFER:
    case D3D_SRV_DIMENSION_BUFFEREX:
        return { type, FormatTypes::Buf };
    case D3D_SRV_DIMENSION_UNKNOWN:
        return { type, FormatTypes::Other };
    default:
        return { type, FormatTypes::Tex };
    }
}

void DxProgram_::Initialize(common::span<const DxShader> shaders)
{
    for (const auto& shader : shaders)
    {
        if (!shader) continue;
        for (uint32_t i = 0; i < shader->BindCount; ++i)
        {
            const auto& item = shader->BindResources[i];
            const auto [type, format] = ParseDescType(item);
            std::vector<BoundedResource>* container = nullptr;
            switch (format)
            {
            case FormatTypes::Buf:      container = &BufferSlots; break;
            case FormatTypes::Tex:      container = &TextureSlots; break;
            case FormatTypes::Sampler:  container = &SamplerSlots; break;
            default:                    continue;
            }
            const auto offset = gsl::narrow_cast<uint16_t>(container->size());
            auto& res = container->emplace_back();
            HashedStrView<char> resName(item.Name);
            res.Hash = resName.Hash;
            res.Name = StrPool.AllocateString(resName);
            res.Detail = &item;
            res.DescOffset = offset;
            res.Type = type;
        }
    }
}

//const PtrProxy<DxDevice_::DeviceProxy>& DxProgram_::GetDevice() const noexcept
//{
//    return Device->Device;
//}

DxProgram_::BoundedResWrapper DxProgram_::GetBufSlot(const size_t idx) const noexcept
{
    const auto& slot = BufferSlots[idx];
    return 
    { 
        StrPool.GetStringView(slot.Name), static_cast<uint16_t>(slot.Detail->Space), 
        static_cast<uint16_t>(slot.Detail->BindPoint), static_cast<uint16_t>(slot.Detail->BindCount), 
        slot.DescOffset, slot.Type 
    };
}

DxProgram_::BoundedResWrapper DxProgram_::GetTexSlot(const size_t idx) const noexcept
{
    const auto& slot = TextureSlots[idx];
    return
    {
        StrPool.GetStringView(slot.Name), static_cast<uint16_t>(slot.Detail->Space),
        static_cast<uint16_t>(slot.Detail->BindPoint), static_cast<uint16_t>(slot.Detail->BindCount),
        slot.DescOffset, slot.Type
    };
}

const DxProgram_::BoundedResource* DxProgram_::GetSlot(const std::vector<BoundedResource>& container, HashedStrView<char> name) const
{
    for (const auto& item : container)
    {
        if (item.Hash != name.Hash) continue;
        if (name == StrPool.GetStringView(item.Name))
            return &item;
    }
    return nullptr;
}


DxProgram_::DxProgramPrepareBase::DxProgramPrepareBase(DxProgram program) : Program(std::move(program)),
    BindMan(std::make_shared<DxUniqueBindingManager>(Program->BufferSlots.size(), Program->TextureSlots.size(), Program->SamplerSlots.size()))
{ }
DxProgram_::DxProgramPrepareBase::~DxProgramPrepareBase()
{ }

bool DxProgram_::DxProgramPrepareBase::SetBuf(HashedStrView<char> name, const DxBuffer_::BufferView& bufview)
{
    if (!bufview.Buffer->CanBindToShader())
        return false;
    const auto slot = Program->GetSlot(Program->BufferSlots, name);
    if (!slot)
        return false;
    const auto res = bufview.Buffer.get();
    for (auto& item : Bindings)
    {
        if (item.Slot == slot)
        {
            item.Resource = res;
            item.Offset = BindMan->SetBuf(bufview, item.Offset);
            return true;
        }
    }
    const auto descOffset = BindMan->SetBuf(bufview, slot->DescOffset);
    Bindings.push_back({ slot, res, descOffset });
    return true;
}


DxProgram_::DxProgramCall::DxProgramCall(DxProgramPrepareBase& prepare) :
    Device(prepare.Program->Device), BindMan(std::move(prepare.BindMan)),
    CSUDescHeap(BindMan->GetCSUHeap(prepare.Program->Device))
{ 
    std::vector<D3D12_DESCRIPTOR_RANGE> descRanges;
    descRanges.reserve(prepare.Bindings.size());
    ResStates.reserve(prepare.Bindings.size());
    for (const auto [res, dxRes, pos] : prepare.Bindings)
    {
        const auto& resDesc = *res->Detail;
        // Expects(slot.FormatType != TempBindResource::FormatTypes::Other);
        D3D12_DESCRIPTOR_RANGE_TYPE descType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ResourceState state = ResourceState::Common;
        switch (res->Type & BoundedResourceType::TypeMask)
        {
        case BoundedResourceType::CBVType:
            descType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            state    = ResourceState::ConstBuffer;
            break;
        case BoundedResourceType::SRVType:
            descType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            state    = ResourceState::NonPSRes;
            break;
        case BoundedResourceType::UAVType:
            descType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; 
            state    = ResourceState::UnorderAccess;
            break;
        case BoundedResourceType::SamplerType:
            descType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            break;
        default:
            continue;
        }
        descRanges.push_back({ descType, resDesc.BindCount, resDesc.BindPoint, resDesc.Space, pos });
        if (state != ResourceState::Common)
        {
            ResStateRecord* ptr = nullptr;
            for (auto& record : ResStates)
            {
                if (record.Resource == dxRes)
                {
                    ptr = &record;
                    break;
                }
            }
            if (ptr)
                ptr->State |= state;
            else
                ResStates.push_back({ dxRes, state });
        }
    }
    //std::vector<D3D12_ROOT_PARAMETER> rootParams;
    D3D12_ROOT_PARAMETER rootParam{};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParam.DescriptorTable.NumDescriptorRanges = gsl::narrow_cast<uint32_t>(descRanges.size());
    rootParam.DescriptorTable.pDescriptorRanges = descRanges.data();
    // create RootSignature
    {
        D3D12_ROOT_SIGNATURE_DESC rootSigDesc =
        {
            1,
            &rootParam,
            0, // static sampler count
            nullptr, // static sampler
            D3D12_ROOT_SIGNATURE_FLAG_NONE
        };
        ComPtr<ID3DBlob> rootSigSer = nullptr, errorBlob = nullptr;
        THROW_HR(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, rootSigSer.GetAddressOf(), errorBlob.GetAddressOf()),
            detail::TryErrorString(u"Failed to SerializeRootSignature", errorBlob));
        THROW_HR(GetDevice()->CreateRootSignature(0, rootSigSer->GetBufferPointer(), rootSigSer->GetBufferSize(),
            IID_PPV_ARGS(&RootSig)), u"Failed to create Root Signature");
    }
}
DxProgram_::DxProgramCall::~DxProgramCall()
{ }

void DxProgram_::DxProgramCall::PutResourceBarrier(const DxCmdList& cmdlist) const
{
    for (const auto& record : ResStates)
    {
        record.Resource->TransitState(cmdlist, record.State);
    }
}


DxComputeProgram_::DxComputeProgram_(DxShader shader) : DxProgram_(shader->Device), Shader(std::move(shader))
{
}
DxComputeProgram_::~DxComputeProgram_()
{ }

DxProgram_::DxProgramPrepare<DxComputeProgram_> DxComputeProgram_::Prepare() const
{
    return std::static_pointer_cast<const DxComputeProgram_>(shared_from_this());
}

DxComputeProgram DxComputeProgram_::Create(DxDevice dev, std::string str, const DxShaderConfig& config)
{
    return std::make_shared<DxComputeProgram_>(DxShader_::CreateAndBuild(dev, ShaderType::Compute, std::move(str), config));
}


DxComputeProgram_::DxComputeCall::DxComputeCall(DxProgramPrepare<DxComputeProgram_>& prepare) :
    DxProgramCall(prepare), Program(prepare.GetProgram())
{
    const auto bytecode = Program->Shader->GetBinary();
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc =
    {
        RootSig,
        { bytecode.data(), bytecode.size() }, // CS bytecode
        0,
        { nullptr, 0 }, // no cache
        D3D12_PIPELINE_STATE_FLAG_NONE
    };
    THROW_HR(GetDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&PSO)), u"Failed to create Compute PSO");
} 

void DxComputeProgram_::DxComputeCall::ExecuteIn(const DxCmdList& cmdlist, const std::array<uint32_t, 3>& threadgroups) const
{
    auto& list = *GetCmdList(cmdlist);
    list.SetPipelineState(PSO);
    list.SetComputeRootSignature(RootSig);
    list.SetComputeRootDescriptorTable(0, CSUDescHeap->GetGPUDescriptorHandleForHeapStart());
    PutResourceBarrier(cmdlist);
    list.Dispatch(threadgroups[0], threadgroups[1], threadgroups[2]);
}

common::PromiseResult<void> DxComputeProgram_::DxComputeCall::ExecuteIn(const DxCmdQue& que, const std::array<uint32_t, 3>& threadgroups) const
{
    const auto cmdlist = que->CreateList();
    ExecuteIn(cmdlist, threadgroups);
    return que->ExecuteAnyList(cmdlist);
}


}

