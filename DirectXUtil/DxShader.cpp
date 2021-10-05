#include "DxPch.h"
#include "DxShader.h"
#include "SystemCommon/StringDetect.h"
#include <dxcapi.h>


namespace dxu
{
ClzProxy(DxShader_, ShaderBlob, IDxcBlob);
using namespace std::string_view_literals;
using Microsoft::WRL::ComPtr;
using common::str::Encoding;
using common::str::HashedStrView;


forceinline const wchar_t* WStr(const char16_t* ptr) noexcept 
{
    return reinterpret_cast<const wchar_t*>(ptr);
}


enum class DXCType { None = 0, Debug = 1, Internal = 2 };
MAKE_ENUM_BITFIELD(DXCType)

class DXCHolder
{
private:
    HMODULE Dll = nullptr;
    DXCHolder()
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
public:
    DxcCreateInstanceProc  DxcCreateInstance  = nullptr;
    DxcCreateInstance2Proc DxcCreateInstance2 = nullptr;
    ~DXCHolder()
    {
        if (Dll)
            FreeLibrary(Dll);
    }
    static const DXCHolder& Get()
    {
        static DXCHolder holder;
        return holder;
    }
};

class DXCCompiler : public common::NonCopyable
{
private:
    uint32_t CheckMaxShaderModel()
    {
        constexpr auto vsTxt = "void main(){ return; }"sv;
        ComPtr<IDxcBlobEncoding> srcBlob;
        THROW_HR(Library->CreateBlobWithEncodingFromPinned(
            vsTxt.data(), gsl::narrow_cast<uint32_t>(vsTxt.size()), CP_UTF8, &srcBlob),
            u"Failed to create src blob");

        uint32_t maxVer = 60;

        while (true)
        {
            const auto smVer = maxVer + 1;
            const auto targetProfile = FMTSTR(L"vs_{}_{}", smVer / 10, smVer % 10);
            ComPtr<IDxcOperationResult> result;
            common::HResultHolder hr = Compiler->Compile(srcBlob.Get(), L"hlsl",
                L"main",
                targetProfile.c_str(),
                nullptr, 0,
                nullptr, 0,
                nullptr, result.GetAddressOf());
            if (!hr) break;
            result->GetStatus(&hr.Value);
            if (!hr) break;
            maxVer = smVer;
        }
        dxLog().debug(u"DxcCompler support ShaderModel [{}.{}].\r\n", maxVer / 10, maxVer % 10);
        return maxVer;
    }
    ComPtr<IDxcLibrary> Library;
    ComPtr<IDxcCompiler> Compiler;
    ComPtr<IDxcValidator> Validator;
    uint32_t MaxSmVer = 0;
public:
    DXCCompiler()
    {
        const auto& holder = DXCHolder::Get();
        THROW_HR(holder.DxcCreateInstance(CLSID_DxcLibrary,   IID_PPV_ARGS(Library.GetAddressOf())),   u"Failed to create DxcLibrary");
        THROW_HR(holder.DxcCreateInstance(CLSID_DxcCompiler,  IID_PPV_ARGS(Compiler.GetAddressOf())),  u"Failed to create DxcCompiler");
        THROW_HR(holder.DxcCreateInstance(CLSID_DxcValidator, IID_PPV_ARGS(Validator.GetAddressOf())), u"Failed to create DxcValidator");

        static const auto MaxShaderModel = CheckMaxShaderModel();
        MaxSmVer = MaxShaderModel;
        /*ComPtr<IDxcVersionInfo> verInfo;
        Validator->QueryInterface(IID_PPV_ARGS(verInfo.GetAddressOf()));
        UINT32 major = 0, minor = 0, flag = 0;
        verInfo->GetVersion(&major, &minor);
        verInfo->GetFlags(&flag);
        const auto type = static_cast<DXCType>(flag);
        dxLog().debug(u"Validator version: [{}.{}], [{}|{}].\n", major, minor, 
            HAS_FIELD(type, DXCType::Debug) ? u"debug" : u"", HAS_FIELD(type, DXCType::Internal) ? u"internal" : u"");*/
    }
    constexpr uint32_t GetMaxSmVer() const noexcept { return MaxSmVer; }

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
            defstrs.emplace_back(to_u16string(k, Encoding::UTF8), u"");
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
            flagstrs.emplace_back(to_u16string(flag, Encoding::UTF8));
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


class DXCReflector : public common::NonCopyable
{
private:
    static constexpr uint32_t FourCC(const char (&str)[5]) noexcept
    {
        const auto [a, b, c, d, e] = str;
        return static_cast<uint32_t>((static_cast<uint8_t>(a) << 0) | (static_cast<uint8_t>(b) << 8) | (static_cast<uint8_t>(c) << 16) | (static_cast<uint8_t>(d) << 24));
    }
    ComPtr<IDxcContainerReflection> Reflector;
    IDxcBlob& Shader;
    uint32_t GetIdx(const uint32_t fourcc, std::u16string_view msg) const
    {
        uint32_t idx = 0;
        THROW_HR(Reflector->FindFirstPartKind(fourcc, &idx), std::u16string(msg));
        return idx;
    }
    std::optional<uint32_t> TryGetIdx(const uint32_t fourcc) const
    {
        uint32_t idx = 0;
        const common::HResultHolder hr = Reflector->FindFirstPartKind(fourcc, &idx);
        if (hr) return idx;
        return {};
    }
public:
    DXCReflector(IDxcBlob& shader) : Shader(shader)
    {
        const auto& holder = DXCHolder::Get();
        THROW_HR(holder.DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(Reflector.GetAddressOf())), u"Failed to create DxcContainerReflection");
        THROW_HR(Reflector->Load(&Shader), u"Failed to load shader");
    }
#define GetPartIdx(name)                                                    \
    constexpr auto name##4CC = FourCC(#name);                               \
    const auto idx = GetIdx(name##4CC, u"Failed to find part of " #name)    \

    ComPtr<ID3D12ShaderReflection> GetShaderReflection() const
    {
        GetPartIdx(DXIL);
        ComPtr<ID3D12ShaderReflection> shaderReflector;
        THROW_HR(Reflector->GetPartReflection(idx, IID_PPV_ARGS(shaderReflector.GetAddressOf())), u"Failed to get part reflection of DXIL");
        return shaderReflector;
    }
    ComPtr<IDxcBlob> GetShaderHash() const
    {
        GetPartIdx(HASH);
        ComPtr<IDxcBlob> content;
        THROW_HR(Reflector->GetPartContent(idx, content.GetAddressOf()), u"Failed to get part content of HASH");
        return content;
    }
    std::string GetShaderHashStr() const
    {
        const auto hash = GetShaderHash();
        std::string str;
        str.reserve(hash->GetBufferSize() * 2);
        constexpr auto ch = "0123456789abcdef";
        const auto pHash = reinterpret_cast<const uint8_t*>(hash->GetBufferPointer());
        for (size_t i = 0; i < hash->GetBufferSize(); ++i)
        {
            const uint8_t dat = pHash[i];
            str.push_back(ch[dat / 16]);
            str.push_back(ch[dat % 16]);
        }
        return str;
    }
#undef GetPartIdx
};



DxShaderStub_::DxShaderStub_(DxDevice dev, ShaderType type, std::string&& str)
    : Device(dev), Source(std::move(str)), Type(type)
{ }
DxShaderStub_::~DxShaderStub_()
{ }
void DxShaderStub_::Build(const DxShaderConfig& config)
{
    const auto reqSmVer = config.SMVersion ? config.SMVersion : Device->SMVer;
    thread_local DXCCompiler Compiler;
    const auto smVer = std::min(reqSmVer, Compiler.GetMaxSmVer());
    if (smVer < config.SMVersion)
        dxLog().warning(u"request [SM{}] but only support [SM{}]\n", config.SMVersion / 10.f, smVer / 10.f);
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
                std::optional<Encoding> chset;
                if (isKnown || codePage != 0)
                {
                    switch (codePage)
                    {
                    case 936:   chset = Encoding::GB18030; break;
                    case 1200:  chset = Encoding::UTF16LE; break;
                    case 1201:  chset = Encoding::UTF16BE; break;
                    case 1252:  chset = Encoding::UTF7;    break;
                    case 12000: chset = Encoding::UTF32LE; break;
                    case 12001: chset = Encoding::UTF32BE; break;
                    case 65000: chset = Encoding::UTF7;    break;
                    case 65001: chset = Encoding::UTF8;    break;
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


static constexpr ShaderType ParseShaderType(const uint32_t versionType) noexcept
{
    switch (versionType)
    {
    case D3D12_SHVER_PIXEL_SHADER:      return ShaderType::Pixel;
    case D3D12_SHVER_VERTEX_SHADER:     return ShaderType::Vertex;
    case D3D12_SHVER_GEOMETRY_SHADER:   return ShaderType::Geometry;
    case D3D12_SHVER_HULL_SHADER:       return ShaderType::Hull;
    case D3D12_SHVER_DOMAIN_SHADER:     return ShaderType::Domain;
    case D3D12_SHVER_COMPUTE_SHADER:    return ShaderType::Compute;
    default: /*Expects(false);*/        return ShaderType::Compute;
    }
}

DxShader_::DxShader_(DxShader_::T_, DxShaderStub_* stub)
    : Device(stub->Device), Source(std::move(stub->Source)), Blob(std::move(stub->Blob)), ThreadGroupSize({ 0 })
{
    DXCReflector reflector(*Blob);
    ShaderHash = reflector.GetShaderHashStr();
    const auto shaderReflector = reflector.GetShaderReflection();
    D3D12_SHADER_DESC desc = {};
    THROW_HR(shaderReflector->GetDesc(&desc), u"Failed to get desc");
    Type = ParseShaderType((desc.Version & 0xFFFF0000) >> 16);
    Version = ((desc.Version & 0x000000F0) >> 4) * 10 + (desc.Version & 0x0000000F);

    if (Type == ShaderType::Compute)
    {
        shaderReflector->GetThreadGroupSize(&ThreadGroupSize[0], &ThreadGroupSize[1], &ThreadGroupSize[2]);
    }

    BindCount = desc.BoundResources;
    BindResources = std::make_unique<detail::BindResourceDetail[]>(BindCount);
    for (uint32_t i = 0; i < BindCount; ++i)
    {
        auto& res = BindResources[i];
        THROW_HR(shaderReflector->GetResourceBindingDesc(i, &res), u"Failed to get binding desc");
        res.NameSv = StrPool.AllocateString(res.Name); // Name will be invalidated after reflector destruct
        dxLog().verbose(u"[{}] [{}] Bind[{},{}] Type[{}] SrvDim[{}], Ret[{}]\n",
            i, res.Name, res.BindPoint, res.BindCount,
            uint32_t(res.Type), uint32_t(res.Dimension), uint32_t(res.ReturnType));
    }
}
DxShader_::~DxShader_()
{ }

const PtrProxy<detail::Device>& DxShader_::GetDevice() const noexcept
{
    return Device->Device;
}

common::span<const std::byte> DxShader_::GetBinary() const
{
    const auto ptr  = Blob->GetBufferPointer();
    const auto size = Blob->GetBufferSize();
    return { reinterpret_cast<const std::byte*>(ptr), size };
}

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


}

