#include "DxPch.h"
#include <DXProgrammableCapture.h>

#if defined(_WIN32)
#   pragma comment (lib, "D3d12.lib")
#   pragma comment (lib, "DXGI.lib")
#endif
#pragma message("Compiling DirectXUtil with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace dxu
{
using namespace common::mlog;
using Microsoft::WRL::ComPtr;

MiniLogger<false>& dxLog()
{
    static MiniLogger<false> dxlog(u"DirectXUtil", { GetConsoleBackend() });
    return dxlog;
}


class DxPixCapture : public DxCapture
{
    ComPtr<IDXGraphicsAnalysis> Handle;
public:
    DxPixCapture()
    {
        if (const common::HResultHolder hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&Handle)); !hr)
        {
            dxLog().warning(u"Failed to get DXGraphicsAnalysis, [{}].\n", hr.ToStr());
        }
    }
    ~DxPixCapture() override {}
    void Begin() const noexcept override
    {
        if (Handle)
            Handle->BeginCapture();
    }
    void End() const noexcept override
    {
        if (Handle)
            Handle->EndCapture();
    }
};
DxCapture::~DxCapture() {}
const DxCapture& DxCapture::Get() noexcept
{
    static DxPixCapture capture;
    return capture;
}


DxNamable::~DxNamable()
{ }

void DxNamable::SetName(std::u16string name)
{
    Name = std::move(name);
    reinterpret_cast<ID3D12Object*>(GetD3D12Object())->SetName(reinterpret_cast<const wchar_t*>(name.c_str()));
}


DxException::DxException(std::u16string msg) : common::BaseException(T_<ExceptionInfo>{}, msg)
{ }

DxException::DxException(common::HResultHolder hresult, std::u16string msg) : DxException(msg)
{
    Attach("HResult", hresult);
    Attach("detail", hresult.ToStr());
}


namespace detail
{

uint32_t TexFormatToDXGIFormat(xziar::img::TextureFormat format) noexcept
{
    using xziar::img::TextureFormat;
    switch (format)
    {
    case TextureFormat::R8:         return DXGI_FORMAT_R8_UNORM;
    case TextureFormat::RG8:        return DXGI_FORMAT_R8G8_UNORM;
    //case TextureFormat::RGB8:       return DXGI_FORMAT_R8G8B8_UNORM;
    //case TextureFormat::SRGB8:      return DXGI_FORMAT_SRGB8_UNORM;
    case TextureFormat::RGBA8:      return DXGI_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat::BGRA8:      return DXGI_FORMAT_B8G8R8A8_UNORM;
    case TextureFormat::SRGBA8:     return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case TextureFormat::R16:        return DXGI_FORMAT_R16_UNORM;
    case TextureFormat::RG16:       return DXGI_FORMAT_R16G16_UNORM;
    //case TextureFormat::RGB16:      return DXGI_FORMAT_R16G16B16_UNORM;
    case TextureFormat::RGBA16:     return DXGI_FORMAT_R16G16B16A16_UNORM;
    case TextureFormat::R8S:        return DXGI_FORMAT_R8_SNORM;
    case TextureFormat::RG8S:       return DXGI_FORMAT_R8G8_SNORM;
    //case TextureFormat::RGB8S:      return DXGI_FORMAT_R8G8B8_SNORM;
    case TextureFormat::RGBA8S:     return DXGI_FORMAT_R8G8B8A8_SNORM;
    case TextureFormat::R16S:       return DXGI_FORMAT_R16_SNORM;
    case TextureFormat::RG16S:      return DXGI_FORMAT_R16G16_SNORM;
    //case TextureFormat::RGB16S:     return DXGI_FORMAT_R16G16B16_SNORM;
    case TextureFormat::RGBA16S:    return DXGI_FORMAT_R16G16B16A16_SNORM;
    case TextureFormat::R8U:        return DXGI_FORMAT_R8_UINT;
    case TextureFormat::RG8U:       return DXGI_FORMAT_R8G8_UINT;
    //case TextureFormat::RGB8U:      return DXGI_FORMAT_R8G8B8_UINT;
    case TextureFormat::RGBA8U:     return DXGI_FORMAT_R8G8B8A8_UINT;
    case TextureFormat::R16U:       return DXGI_FORMAT_R16_UINT;
    case TextureFormat::RG16U:      return DXGI_FORMAT_R16G16_UINT;
    //case TextureFormat::RGB16U:     return DXGI_FORMAT_R16G16B16_UINT;
    case TextureFormat::RGBA16U:    return DXGI_FORMAT_R16G16B16A16_UINT;
    case TextureFormat::R32U:       return DXGI_FORMAT_R32_UINT;
    case TextureFormat::RG32U:      return DXGI_FORMAT_R32G32_UINT;
    case TextureFormat::RGB32U:     return DXGI_FORMAT_R32G32B32_UINT;
    case TextureFormat::RGBA32U:    return DXGI_FORMAT_R32G32B32A32_UINT;
    case TextureFormat::R8I:        return DXGI_FORMAT_R8_SINT;
    case TextureFormat::RG8I:       return DXGI_FORMAT_R8G8_SINT;
    //case TextureFormat::RGB8I:      return DXGI_FORMAT_R8G8B8_SINT;
    case TextureFormat::RGBA8I:     return DXGI_FORMAT_R8G8B8A8_SINT;
    case TextureFormat::R16I:       return DXGI_FORMAT_R16_SINT;
    case TextureFormat::RG16I:      return DXGI_FORMAT_R16G16_SINT;
    //case TextureFormat::RGB16I:     return DXGI_FORMAT_R16G16B16_SINT;
    case TextureFormat::RGBA16I:    return DXGI_FORMAT_R16G16B16A16_SINT;
    case TextureFormat::R32I:       return DXGI_FORMAT_R32_SINT;
    case TextureFormat::RG32I:      return DXGI_FORMAT_R32G32_SINT;
    case TextureFormat::RGB32I:     return DXGI_FORMAT_R32G32B32_SINT;
    case TextureFormat::RGBA32I:    return DXGI_FORMAT_R32G32B32A32_SINT;
    case TextureFormat::Rh:         return DXGI_FORMAT_R16_FLOAT;
    case TextureFormat::RGh:        return DXGI_FORMAT_R16G16_FLOAT;
    //case TextureFormat::RGBh:       return DXGI_FORMAT_R16G16B16_FLOAT;
    case TextureFormat::RGBAh:      return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case TextureFormat::Rf:         return DXGI_FORMAT_R32_FLOAT;
    case TextureFormat::RGf:        return DXGI_FORMAT_R32G32_FLOAT;
    case TextureFormat::RGBf:       return DXGI_FORMAT_R32G32B32_FLOAT;
    case TextureFormat::RGBAf:      return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case TextureFormat::RG11B10:    return DXGI_FORMAT_R11G11B10_FLOAT;
    //case TextureFormat::RGB332:     return DXGI_FORMAT_R3_G3_B2;
    case TextureFormat::BGRA4444:   return DXGI_FORMAT_B4G4R4A4_UNORM;
    case TextureFormat::BGR5A1:     return DXGI_FORMAT_B5G5R5A1_UNORM;
    case TextureFormat::BGR565:     return DXGI_FORMAT_B5G6R5_UNORM;
    case TextureFormat::RGB10A2:    return DXGI_FORMAT_R10G10B10A2_UNORM;
    case TextureFormat::RGB10A2U:   return DXGI_FORMAT_R10G10B10A2_UINT;
    case TextureFormat::BC1:        return DXGI_FORMAT_BC1_UNORM;
    //case TextureFormat::BC1A:       return DXGI_FORMAT_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    case TextureFormat::BC2:        return DXGI_FORMAT_BC2_UNORM;
    case TextureFormat::BC3:        return DXGI_FORMAT_BC3_UNORM;
    case TextureFormat::BC4:        return DXGI_FORMAT_BC4_UNORM;
    case TextureFormat::BC5:        return DXGI_FORMAT_BC5_UNORM;
    case TextureFormat::BC6H:       return DXGI_FORMAT_BC6H_UF16;
    case TextureFormat::BC7:        return DXGI_FORMAT_BC7_UNORM;
    case TextureFormat::BC1SRGB:    return DXGI_FORMAT_BC1_UNORM_SRGB;
    //case TextureFormat::BC1ASRGB:   return DXGI_FORMAT_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
    case TextureFormat::BC2SRGB:    return DXGI_FORMAT_BC2_UNORM_SRGB;
    case TextureFormat::BC3SRGB:    return DXGI_FORMAT_BC3_UNORM_SRGB;
    case TextureFormat::BC7SRGB:    return DXGI_FORMAT_BC7_UNORM_SRGB;
    default:                        return DXGI_FORMAT_UNKNOWN;
    }
}

std::u16string TryErrorString(std::u16string str, const Microsoft::WRL::ComPtr<ID3DBlob>& errBlob)
{
    if (errBlob != nullptr)
    {
        const auto msg = reinterpret_cast<const char*>(errBlob->GetBufferPointer());
        str.append(u":\n");
        str.append(common::str::to_u16string(msg, common::str::Charset::UTF8));
    }
    return str;
}

std::string_view GetBoundedResTypeName(const BoundedResourceType type) noexcept
{
    switch (type)
    {
#define CASE_T(t) case BoundedResourceType::t:    return #t
    CASE_T(Other);
    CASE_T(Sampler);
    CASE_T(CBuffer);
    CASE_T(TBuffer);
    CASE_T(Typed);
    CASE_T(StructBuf);
    CASE_T(RawBuf);
    CASE_T(RWTyped);
    CASE_T(RWStructBuf);
    CASE_T(RWRawBuf);
#undef CASE_T
    default:    return "unknown";
    }
}

}


}