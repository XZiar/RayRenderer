#include "dxPch.h"
#include "dxShader.h"
#include "ProxyStruct.h"
#include "StringUtil/Detect.h"
#include <dxcapi.h>


namespace dxu
{
MAKE_ENABLER_IMPL(DXShader_);
DxProxy(DXShader_,      ShaderBlob,     IDxcBlob);
DxProxy(DXShaderStub,   BuildResult,    IDxcOperationResult);
using Microsoft::WRL::ComPtr;

class DXCCompiler : public common::NonCopyable
{
private:
    HMODULE Dll = nullptr;
    DxcCreateInstanceProc DxcCreateInstance = nullptr;
    DxcCreateInstance2Proc DxcCreateInstance2 = nullptr;
    ComPtr<IDxcLibrary> Library;
    ComPtr<IDxcCompiler> Compiler;
    DXCCompiler()
    {
        Dll = LoadLibraryW(L"dxcompiler.dll");
        if (!Dll)
            COMMON_THROW(DXException, HRESULT_FROM_WIN32(GetLastError()), u"Cannot load dxcompiler.dll");
#define GetFunc(name) \
        if (name = reinterpret_cast<name##Proc>(GetProcAddress(Dll, #name)); !name) \
            COMMON_THROW(DXException, HRESULT_FROM_WIN32(GetLastError()), u"Cannot load func " #name)
        GetFunc(DxcCreateInstance);
        GetFunc(DxcCreateInstance2);
#undef GetFunc
        THROW_HR(DxcCreateInstance(CLSID_DxcLibrary,  IID_PPV_ARGS(Library.GetAddressOf())),  u"Failed to create DxcLibrary");
        THROW_HR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(Compiler.GetAddressOf())), u"Failed to create DxcCompiler");
    }
public:
    ~DXCCompiler() 
    {
        Library.Reset();
        Compiler.Reset();
        if (Dll)
            FreeLibrary(Dll);
    }
    std::pair<ComPtr<IDxcOperationResult>, common::HResultHolder> Compile(ShaderType type, std::string_view src, const DXShaderConfig& config, uint32_t smVer) const
    {
        using common::str::to_u16string;
        ComPtr<IDxcOperationResult> result;
        ComPtr<IDxcBlobEncoding> srcBlob;
        THROW_HR(Library->CreateBlobWithEncodingFromPinned(
            src.data(), gsl::narrow_cast<uint32_t>(src.size()), CP_UTF8, &srcBlob),
            u"Failed to create src blob");

        std::vector<std::pair<std::u16string, std::u16string>> defstrs;
        for (const auto& [k, v] : config.Defines)
        {
            defstrs.emplace_back(to_u16string(k, common::str::Charset::UTF8), u"");
            std::visit([&](const auto val)
                {
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (!std::is_same_v<T, std::monostate>)
                        defstrs.back().second = FMTSTR(u"{}", val);
                }, v);
        }
        std::vector<DxcDefine> defs;
        for (const auto& [k, v] : defstrs)
            defs.push_back(DxcDefine{ reinterpret_cast<const wchar_t*>(k.c_str()), 
                reinterpret_cast<const wchar_t*>(v.c_str()) });

        const auto tCh = [](ShaderType type)
        {
            switch (type)
            {
            case ShaderType::Vertex:    return u'V';
            case ShaderType::Geometry:  return u'G';
            case ShaderType::Pixel:     return u'P';
            case ShaderType::Hull:      return u'H';
            case ShaderType::Domain:    return u'D';
            case ShaderType::Compute:   return u'C';
            default:                    COMMON_THROW(DXException, u"unrecoginized shadertype");
            }
        }(type);
        const auto targetProfile = FMTSTR(u"{}S_{}_{}", tCh, smVer / 10, smVer % 10);

        common::HResultHolder hr = Compiler->Compile(srcBlob.Get(), L"hlsl",
            config.EntryPoint.empty() ? L"main" : reinterpret_cast<const wchar_t*>(config.EntryPoint.data()),
            reinterpret_cast<const wchar_t*>(targetProfile.c_str()),
            nullptr, 0,
            defs.data(), gsl::narrow_cast<uint32_t>(defs.size()),
            nullptr, result.GetAddressOf());
        if (hr) result->GetStatus(&hr.Value);
        return { result, hr };
    }

    static const DXCCompiler& Get()
    {
        static DXCCompiler compiler;
        return compiler;
    }
};


DXShaderStub::DXShaderStub(DXDevice dev, ShaderType type, std::string&& str)
    : Device(dev), Source(std::move(str)), Type(type)
{
    [[maybe_unused]] auto& tmp = DXCCompiler::Get();
}
DXShaderStub::~DXShaderStub()
{
    if (Blob)
        Blob->Release();
}
void DXShaderStub::Build(const DXShaderConfig& config)
{
    using common::str::Charset;
    const auto smVer = config.SMVersion ? config.SMVersion : Device->SMVer;
    const auto [result, hr] = DXCCompiler::Get().Compile(Type, Source, config, smVer);
    std::u16string errorBuf;
    if (result)
    {
        ComPtr<IDxcBlobEncoding> err;
        if (SUCCEEDED(result->GetErrorBuffer(err.GetAddressOf())))
        {
            common::span<const std::byte> rawBuf{ reinterpret_cast<const std::byte*>(err->GetBufferPointer()), err->GetBufferSize() };
            uint32_t codePage = 0;
            BOOL isKnown = false;
            if (SUCCEEDED(err->GetEncoding(&isKnown, &codePage)))
            {
                std::optional<Charset> chset;
                if (isKnown || codePage != 0)
                {
                    switch (codePage)
                    {
                    case 936:   chset = Charset::GB18030; break;
                    case 1200:  chset = Charset::UTF16LE; break;
                    case 1201:  chset = Charset::UTF16BE; break;
                    case 1252:  chset = Charset::UTF7;    break;
                    case 12000: chset = Charset::UTF32LE; break;
                    case 12001: chset = Charset::UTF32BE; break;
                    case 65000: chset = Charset::UTF7;    break;
                    case 65001: chset = Charset::UTF8;    break;
                    default:                              break;
                    }
                }
                if (!chset)
                {
                    chset = common::str::DetectEncoding(rawBuf);
                }
                errorBuf = common::str::to_u16string(rawBuf, *chset);
            }
        }
    }
    if (hr)
    {
        dxLog().success(u"build shader success:\n{}\n", errorBuf);
        THROW_HR(result->GetResult(&Blob), u"Failed to extract shader blob");
        return;
    }
    dxLog().error(u"build shader failed:\n{}\n", errorBuf);
    common::SharedString<char16_t> log(errorBuf);
    COMMON_THROWEX(DXException, hr, u"Failed to compile hlsl source")
        .Attach("dev", Device)
        .Attach("source", Source)
        .Attach("detail", log)
        .Attach("buildlog", log);
}

std::u16string DXShaderStub::GetBuildLog() const
{
    return std::u16string();
}

DXShader DXShaderStub::Finish()
{
    return DXShader();
}


DXShader_::~DXShader_()
{ }

DXShaderStub DXShader_::Create(DXDevice dev, ShaderType type, std::string str)
{
    return { dev, type, std::move(str) };
}

DXShader DXShader_::CreateAndBuild(DXDevice dev, ShaderType type, std::string str, const DXShaderConfig & config)
{
    auto stub = Create(dev, type, std::move(str));
    stub.Build(config);
    return stub.Finish();
}

}