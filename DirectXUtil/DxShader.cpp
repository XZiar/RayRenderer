#include "DxPch.h"
#include "DxShader.h"
#include "ProxyStruct.h"
#include "StringUtil/Detect.h"
#include <dxcapi.h>


namespace dxu
{
DxProxy(DxShader_,      ShaderBlob,     IDxcBlob);
using Microsoft::WRL::ComPtr;
using common::str::Charset;


forceinline const wchar_t* WStr(const char16_t* ptr) noexcept 
{
    return reinterpret_cast<const wchar_t*>(ptr);
}


class DXCCompiler : public common::NonCopyable
{
private:
    struct Holder
    {
        HMODULE Dll = nullptr;
        DxcCreateInstanceProc DxcCreateInstance = nullptr;
        DxcCreateInstance2Proc DxcCreateInstance2 = nullptr;
        Holder()
        {
            Dll = LoadLibraryW(L"dxcompiler.dll");
            if (!Dll)
                COMMON_THROW(DxException, HRESULT_FROM_WIN32(GetLastError()), u"Cannot load dxcompiler.dll");
#define GetFunc(name) \
            if (name = reinterpret_cast<name##Proc>(GetProcAddress(Dll, #name)); !name) \
                COMMON_THROW(DxException, HRESULT_FROM_WIN32(GetLastError()), u"Cannot load func " #name)
            GetFunc(DxcCreateInstance);
            GetFunc(DxcCreateInstance2);
#undef GetFunc
        }
        ~Holder()
        {
            if (Dll)
                FreeLibrary(Dll);
        }
    };
    static const Holder& Get()
    {
        static Holder holder;
        return holder;
    }
    ComPtr<IDxcLibrary> Library;
    ComPtr<IDxcCompiler> Compiler;
public:
    DXCCompiler()
    {
        const auto& holder = Get();
        THROW_HR(holder.DxcCreateInstance(CLSID_DxcLibrary,  IID_PPV_ARGS(Library.GetAddressOf())),  u"Failed to create DxcLibrary");
        THROW_HR(holder.DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(Compiler.GetAddressOf())), u"Failed to create DxcCompiler");
    }
    std::pair<ComPtr<IDxcOperationResult>, common::HResultHolder> Compile(ShaderType type, std::string_view src, const DxShaderConfig& config, uint32_t smVer) const
    {
        using common::str::to_u16string;
        ComPtr<IDxcOperationResult> result;
        ComPtr<IDxcBlobEncoding> srcBlob;
        THROW_HR(Library->CreateBlobWithEncodingFromPinned(
            src.data(), gsl::narrow_cast<uint32_t>(src.size()), CP_UTF8, &srcBlob),
            u"Failed to create src blob");

        // defines
        std::vector<std::pair<std::u16string, std::u16string>> defstrs;
        for (const auto& [k, v] : config.Defines)
        {
            defstrs.emplace_back(to_u16string(k, Charset::UTF8), u"");
            std::visit([&](const auto val)
                {
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (!std::is_same_v<T, std::monostate>)
                        defstrs.back().second = FMTSTR(u"{}", val);
                }, v);
        }
        std::vector<DxcDefine> defs;
        for (const auto& [k, v] : defstrs)
            defs.push_back(DxcDefine{ WStr(k.c_str()), WStr(v.c_str()) });

        // flags
        std::vector<std::u16string> flagstrs = { u"-Zi", u"-O3", u"-Qembed_debug" };
        for (const auto& flag : config.Flags)
        {
            flagstrs.emplace_back(to_u16string(flag, Charset::UTF8));
        }
        std::vector<const wchar_t*> flags;
        for (const auto& flag : flagstrs)
            flags.push_back(WStr(flag.c_str()));

        const auto tCh = [](ShaderType type)
        {
            switch (type)
            {
            case ShaderType::Vertex:    return u'v';
            case ShaderType::Geometry:  return u'g';
            case ShaderType::Pixel:     return u'p';
            case ShaderType::Hull:      return u'h';
            case ShaderType::Domain:    return u'd';
            case ShaderType::Compute:   return u'c';
            default:                    COMMON_THROW(DxException, u"unrecoginized shadertype");
            }
        }(type);
        const auto targetProfile = FMTSTR(u"{}s_{}_{}", tCh, smVer / 10, smVer % 10);

        common::HResultHolder hr = Compiler->Compile(srcBlob.Get(), L"hlsl",
            config.EntryPoint.empty() ? L"main" : WStr(config.EntryPoint.data()),
            WStr(targetProfile.c_str()),
            flags.data(), gsl::narrow_cast<uint32_t>(flags.size()),
            defs.data(),  gsl::narrow_cast<uint32_t>(defs.size()),
            nullptr, result.GetAddressOf());
        if (hr) result->GetStatus(&hr.Value);
        return { result, hr };
    }
};


DxShaderStub_::DxShaderStub_(DxDevice dev, ShaderType type, std::string&& str)
    : Device(dev), Source(std::move(str)), Type(type)
{ }
DxShaderStub_::~DxShaderStub_()
{
    if (Blob)
        Blob->Release();
}
void DxShaderStub_::Build(const DxShaderConfig& config)
{
    const auto smVer = config.SMVersion ? config.SMVersion : Device->SMVer;
    thread_local DXCCompiler Compiler;
    const auto [result, hr] = Compiler.Compile(Type, Source, config, smVer);
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
    COMMON_THROWEX(DxException, hr, u"Failed to compile hlsl source")
        .Attach("dev", Device)
        .Attach("source", Source)
        .Attach("detail", log)
        .Attach("buildlog", log);
}

std::u16string DxShaderStub_::GetBuildLog() const
{
    return std::u16string();
}


DxShader_::DxShader_(DxShader_::T_, DxShaderStub_* stub)
    : Source(std::move(stub->Source)), Blob(stub->Blob)
{
    stub->Blob.SetNull();
}
DxShader_::~DxShader_()
{ }

DxShaderStub<DxShader_> DxShader_::Create(DxDevice dev, ShaderType type, std::string str)
{
    return { dev, type, std::move(str) };
}
DxShader DxShader_::CreateAndBuild(DxDevice dev, ShaderType type, std::string str, const DxShaderConfig & config)
{
    auto stub = Create(dev, type, std::move(str));
    stub.Build(config);
    return stub.Finish();
}


DxComputeShader_::DxComputeShader_(DxComputeShader_::T_, DxShaderStub_* stub)
    : DxShader_(DxShader_::T_{}, stub)
{
}
DxComputeShader_::~DxComputeShader_()
{ }

DxComputeShader DxComputeShader_::Create(DxDevice dev, std::string str, const DxShaderConfig& config)
{
    DxShaderStub<DxComputeShader_> stub{ dev, ShaderType::Compute, std::move(str) };
    stub.Build(config);
    return stub.Finish();
}


}

