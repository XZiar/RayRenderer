#include "DxPch.h"
#include "DxProgram.h"
#include "ProxyStruct.h"
#include "StringUtil/Detect.h"
#include <dxcapi.h>


namespace dxu
{
DxProxy(DxProgram_, RootSignature, ID3D12RootSignature);
using namespace std::string_view_literals;
using Microsoft::WRL::ComPtr;
using common::str::Charset;
using common::str::HashedStrView;



enum class BoundedResourceFormat : uint16_t
{
    Other = 0, Sampler = 1, Buf = 2, Tex = 3
};

struct TempBindResource
{
    enum class FormatTypes : uint16_t { Other = 0, Sampler = 1, Buf = 2, Tex = 3 };
    D3D12_SHADER_INPUT_BIND_DESC Descriptor;
    uint32_t CategoryIdx;
    BoundedResourceType ResType;
    FormatTypes FormatType;
    constexpr TempBindResource() noexcept : Descriptor{}, CategoryIdx(0),
        ResType(BoundedResourceType::OtherType), FormatType(FormatTypes::Other)
    { }
    void InitType() noexcept
    {
        switch (Descriptor.Type)
        {
        case D3D_SIT_CBUFFER:
            ResType = BoundedResourceType::CBuffer; FormatType = FormatTypes::Buf; return;
        case D3D_SIT_TBUFFER:
            ResType = BoundedResourceType::TBuffer; break;
        case D3D_SIT_TEXTURE:
            ResType = BoundedResourceType::Texture; break;
        case D3D_SIT_SAMPLER:
            ResType = BoundedResourceType::Sampler; FormatType = FormatTypes::Sampler; return;
        case D3D_SIT_UAV_RWTYPED:
            ResType = BoundedResourceType::RWTypedUAV; break;
        case D3D_SIT_STRUCTURED:
            ResType = BoundedResourceType::UntypedUAV; break;
        case D3D_SIT_UAV_RWSTRUCTURED:
            ResType = BoundedResourceType::RWUntypedUAV; break;
        case D3D_SIT_BYTEADDRESS:
            ResType = BoundedResourceType::RawUAV; FormatType = FormatTypes::Buf; return;
        case D3D_SIT_UAV_RWBYTEADDRESS:
            ResType = BoundedResourceType::RWRawUAV; FormatType = FormatTypes::Buf; return;
        case D3D_SIT_UAV_APPEND_STRUCTURED:
        case D3D_SIT_UAV_CONSUME_STRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        default:
            ResType = BoundedResourceType::Other; FormatType = FormatTypes::Other; return;
        }
        switch (Descriptor.Dimension)
        {
        case D3D_SRV_DIMENSION_BUFFER:
        case D3D_SRV_DIMENSION_BUFFEREX:
            FormatType = FormatTypes::Buf; return;
        case D3D_SRV_DIMENSION_UNKNOWN:
            FormatType = FormatTypes::Other; return;
        default:
            FormatType = FormatTypes::Tex; return;
        }
    }
};

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

DxComputeProgram_::DxComputeCall::DxComputeCall(DxProgramPrepare<DxComputeProgram_>& prepare, const DxCmdList& prevList) :
    Program(prepare.GetProgram()), CmdList(DxComputeCmdList_::Create(Program->Device, prevList)),
    RootSig(std::move(prepare.RootSig))
{
    prepare.FinishBinding(CmdList);
}



DxProgram_::DxProgramPrepareBase::DxProgramPrepareBase(DxProgram program) : Program(std::move(program)), 
    BindMan(Program->BufferSlots.size(), Program->TextureSlots.size(), Program->SamplerSlots.size())
{ }
DxProgram_::DxProgramPrepareBase::~DxProgramPrepareBase()
{
    if (RootSig)
        RootSig->Release();
}

bool DxProgram_::DxProgramPrepareBase::SetBuf(HashedStrView<char> name, const DxBuffer_::BufferView& bufview)
{
    if (!bufview.Buffer->CanBindToShader())
        return false;
    const auto res = Program->GetSlot(Program->BufferSlots, name);
    if (!res)
        return false;
    for (auto& item : Bindings)
    {
        if (item.first == res)
        {
            item.second = BindMan.SetBuf(bufview, item.second);
            return true;
        }
    }
    const auto descOffset = BindMan.SetBuf(bufview, res->DescOffset);
    Bindings.emplace_back(res, descOffset);
    return true;
}

void DxProgram_::DxProgramPrepareBase::FinishBinding(const DxCmdList& cmdlist)
{
    auto& device = *Program->Device->Device;
    std::vector<D3D12_DESCRIPTOR_RANGE> descRanges;
    descRanges.reserve(Bindings.size());
    for (uint32_t i = 0; i < descRanges.size(); ++i)
    {
        const auto [res, pos] = Bindings[i];
        const auto& resDesc = *res->Detail;
        // Expects(slot.FormatType != TempBindResource::FormatTypes::Other);
        D3D12_DESCRIPTOR_RANGE_TYPE descType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        switch (res->Type & BoundedResourceType::TypeMask)
        {
        case BoundedResourceType::CBVType:      descType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; break;
        case BoundedResourceType::SRVType:      descType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; break;
        case BoundedResourceType::UAVType:      descType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; break;
        case BoundedResourceType::SamplerType:  descType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER; break;
        default:                                continue;
        }
        descRanges.push_back({ descType, resDesc.BindCount, resDesc.BindPoint, resDesc.Space, pos });
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
            ([](const auto& err) -> std::u16string
                {
                    if (err != nullptr)
                    {
                        const auto msg = reinterpret_cast<const char*>(err->GetBufferPointer());
                        return FMTSTR(u"Failed to SerializeRootSignature:\n{}", msg);
                    }
                    else
                        return u"Failed to SerializeRootSignature";
                }(errorBlob)));
        THROW_HR(device.CreateRootSignature(0, rootSigSer->GetBufferPointer(), rootSigSer->GetBufferSize(),
            IID_PPV_ARGS(&RootSig)), u"Failed to create Root Signature");
    }
    
}




}

