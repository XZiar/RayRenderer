#include "TestRely.h"
#include "XCompCommon.h"
#include "DirectXUtil/DxDevice.h"
#include "DirectXUtil/DxCmdQue.h"
#include "DirectXUtil/DxBuffer.h"
#include "DirectXUtil/DxShader.h"
#include "DirectXUtil/DxProgram.h"
#include "DirectXUtil/NLDX.h"
#include "DirectXUtil/NLDXRely.h"
#include "SystemCommon/ConsoleEx.h"
#include "SystemCommon/StringConvert.h"
#include "common/Linq2.hpp"
#include "common/ContainerEx.hpp"


using namespace common;
using namespace common::mlog;
using namespace std::string_view_literals;
using namespace dxu;
using std::string;
using std::u16string;
using std::u16string_view;
using common::str::to_u16string;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"DXStub", { GetConsoleBackend() });
    return logger;
}

namespace
{
using namespace xcomp;

struct DXStubHelper : public XCStubHelper
{
    struct NLDXContext_ : public NLDXContext
    {
        friend DXStubHelper;
    };
    struct KerArg
    {
        const NLDXContext_& Ctx;
        std::variant<const ResourceInfo*, const ConstantInfo*> Obj;
        KerArg(const RunArgInfo& info) noexcept : Ctx(*reinterpret_cast<NLDXContext_*>(static_cast<uintptr_t>(info.Meta0)))
        {
            if (info.Meta1 & 0x80000000)
            {
                Obj = &(Ctx.ShaderConstants[info.Meta1 & 0x7fffffff]);
            }
            else
            {
                Obj = &(Ctx.BindResoures[info.Meta1]);
            }
        }
        constexpr bool IsSimple() const noexcept 
        { 
            return Obj.index() == 1;
        }
        constexpr bool IsTex() const noexcept 
        { 
            return Obj.index() == 0 && std::get<0>(Obj)->TexType != InstanceArgInfo::TexTypes::Empty;
        }
        std::string_view GetName() const noexcept
        {
            if (Obj.index() == 0)
                return Ctx.StrPool.GetStringView(std::get<0>(Obj)->Name);
            else
                return Ctx.StrPool.GetStringView(std::get<1>(Obj)->Name);
        }
        std::string_view GetDataType() const noexcept
        {
            if (Obj.index() == 0)
                return Ctx.StrPool.GetStringView(std::get<0>(Obj)->DataType);
            else
                return Ctx.StrPool.GetStringView(std::get<1>(Obj)->DataType);
        }
    };
    ~DXStubHelper() override { }
    std::any TryFindKernel(xcomp::XCNLContext& context, std::string_view name) const final
    {
        auto& ctx = static_cast<NLDXContext_&>(context);
        const auto name32 = common::str::to_u32string(name);
        size_t idx = 0;
        for (const auto& kname : ctx.KernelNames)
        {
            if (name32 == kname)
            {
                return std::make_pair(&ctx, idx);
            }
            idx++;
        }
        return {};
    }
    void FillArgs(std::vector<RunArgInfo>& dst, const std::any& cookie) const final
    {
        const auto [ctx, kidx] = std::any_cast<std::pair<NLDXContext_*, size_t>>(cookie);
        const auto kch = static_cast<char>(kidx);
        const uint64_t meta0 = reinterpret_cast<uintptr_t>(ctx);
        uint32_t idx = 0;
        for (const auto& res : ctx->BindResoures)
        {
            if (res.KernelIds.find(kch) != std::string::npos)
            {
                dst.emplace_back(meta0, idx, [](const auto& res)
                    {
                        if (res.TexType != InstanceArgInfo::TexTypes::Empty)
                            return RunArgInfo::ArgType::Image;
                        return RunArgInfo::ArgType::Buffer;
                    }(res));
            }
            idx++;
        }
        idx = 0x80000000;
        for (const auto& res : ctx->ShaderConstants)
        {
            if (res.KernelIds.find(kch) != std::string::npos)
            {
                dst.emplace_back(meta0, idx, RunArgInfo::ArgType::Val32);
            }
            idx++;
        }
    }
    std::optional<size_t> FindArgIdx(common::span<const RunArgInfo> args, std::string_view name) const final
    {
        size_t idx = 0;
        for (const auto& arg : args)
        {
            if (const KerArg tmp(arg); tmp.GetName() == name)
                return idx;
            idx++;
        }
        return {};
    }
    bool CheckType(const RunArgInfo& dst, RunArgInfo::ArgType type) const noexcept final
    {
        const KerArg tmp(dst);
        if (tmp.IsSimple())
            return type != RunArgInfo::ArgType::Buffer && type != RunArgInfo::ArgType::Image;
        if (tmp.IsTex())
            return type == RunArgInfo::ArgType::Image;
        else
            return type == RunArgInfo::ArgType::Buffer;
    }
    std::string_view GetRealTypeName(const RunArgInfo& info) const noexcept final
    {
        const KerArg tmp(info);
        if (tmp.IsSimple())
            return tmp.GetDataType();
        if (tmp.IsTex())
            return InstanceArgInfo::GetTexTypeName(std::get<0>(tmp.Obj)->TexType);
        else
            return dxu::detail::GetBoundedResTypeName(std::get<0>(tmp.Obj)->Type);
    }
};
static DXStubHelper StubHelper;

}


static void BufTest(DxDevice dev, DxComputeCmdQue cmdque)
{
    //const auto buf = DxBuffer_::Create(dev, { CPUPageProps::WriteBack, MemPrefer::PreferCPU }, HeapFlags::Empty, 1024576, ResourceFlags::Empty);
    const auto buf = DxBuffer_::Create(dev, HeapType::Default, HeapFlags::Empty, 1024576, ResourceFlags::Empty);

    {
        const auto map0 = buf->Map(cmdque, MapFlags::ReadOnly, 0, 1024);
        const auto sp0 = map0.AsType<uint8_t>();
        //log().Verbose("sp0: {}\n", sp0.subspan(0, 8));
    }
    {
        std::array<uint8_t, 256> tmp;
        for (uint32_t i = 0; i < 256; ++i)
            tmp[i] = static_cast<uint8_t>(i);
        buf->WriteSpan(cmdque, tmp, 4)->WaitFinish();
    }
    {
        const auto map1 = buf->Map(cmdque, MapFlags::ReadOnly, 0, 1024);
        const auto sp1 = map1.AsType<uint8_t>();
        //log().Verbose("sp1: {}\n", sp1.subspan(0, 8));
    }
    {
        std::array<uint8_t, 256> tmp = { 0 };
        buf->ReadSpan(cmdque, tmp, 0)->WaitFinish();
        common::span<const uint8_t> sp2(tmp);
        //log().Verbose("sp2: {}\n", sp2.subspan(0, 8));
    }
    const auto readback = buf->Read(cmdque, 1024)->Get();
    const auto sp = readback.AsSpan<uint8_t>();
}

static void RunKernel(DxDevice dev, DxComputeCmdQue cmdque, DxComputeProgram prog)
{
    const auto origin = DxBuffer_::Create(dev, HeapType::Upload, HeapFlags::Empty, 4096, ResourceFlags::Empty);
    {
        const auto mapped = origin->Map(cmdque, MapFlags::WriteOnly, 0, 4096);
        const auto sp = mapped.AsType<uint32_t>();
        uint32_t val = 0;
        for (auto& ele : sp)
        {
            ele = val++;
        }
    }
    const auto cmdlist = DxComputeCmdList_::Create(dev);
    cmdlist->SetRecordFlush(true);
    auto prepare = prog->Prepare();
    std::vector<DxBuffer> bufs;
    {
        const auto rgBuf = cmdlist->DeclareRange(u"Prepare buffers");
        for (const auto& item : prog->BufSlots())
        {
            const auto flag = (item.Type & BoundedResourceType::CategoryMask) == BoundedResourceType::UAVs ?
                ResourceFlags::AllowUnorderAccess : ResourceFlags::Empty;
            const auto buf = DxBuffer_::Create(dev, HeapType::Default, HeapFlags::Empty, 4096, flag);
            buf->SetName(common::str::to_u16string(item.Name, common::str::Encoding::UTF8));
            switch (item.Type & BoundedResourceType::InnerTypeMask)
            {
            case BoundedResourceType::InnerTyped:
                prepare.SetBuf(item.Name, buf->CreateTypedView(xziar::img::TextureFormat::RGBA8, 1024));
                log().Verbose(u"Attach Typed view to [{}]({}) Type[{}]\n", item.Name, item.BindReg,
                    dxu::detail::GetBoundedResTypeName(item.Type));
                break;
            case BoundedResourceType::InnerStruct:
                prepare.SetBuf(item.Name, buf->CreateStructuredView<uint32_t>(1024));
                log().Verbose(u"Attach Structured view to [{}]({}) Type[{}]\n", item.Name, item.BindReg,
                    dxu::detail::GetBoundedResTypeName(item.Type));
                break;
            case BoundedResourceType::InnerRaw:
                prepare.SetBuf(item.Name, buf->CreateRawView(1024));
                log().Verbose(u"Attach Raw view to [{}]({}) Type[{}]\n", item.Name, item.BindReg,
                    dxu::detail::GetBoundedResTypeName(item.Type));
                break;
            case BoundedResourceType::InnerCBuf:
                prepare.SetBuf(item.Name, buf->CreateConstBufView(4096));
                log().Verbose(u"Attach Const Buf view to [{}]({}) Type[{}]\n", item.Name, item.BindReg,
                    dxu::detail::GetBoundedResTypeName(item.Type));
                break;
            default: continue;
            }
            buf->CopyFrom(cmdlist, 0, origin, 0, 4096);
            bufs.emplace_back(buf);
        }
        common::mlog::SyncConsoleBackend();
    }
    const auto call = prepare.Finish();
    const auto tsBegin = cmdlist->CaptureTimestamp();
    {
        const auto rgExe = cmdlist->DeclareRange(u"Execute shader");
        call.Execute(cmdlist, { 1024, 1, 1 });
    }
    const auto tsEnd = cmdlist->CaptureTimestamp();
    cmdlist->Close();
    const auto query = cmdque->ExecuteWithQuery(cmdlist)->Get();
    const auto timeBegin = query.ResolveQuery(tsBegin);
    const auto timeEnd   = query.ResolveQuery(tsEnd);
    log().Debug(u"kernel execution: [{} ~ {}] = [{}]ns.\n", timeBegin, timeEnd, timeEnd - timeBegin);
    {
        const auto rgExe = cmdque->DeclareRange(u"Readback buffer");
        for (const auto& buf : bufs)
        {
            const auto mapped = buf->Map(cmdque, MapFlags::ReadOnly, 0, 64);
            const auto sp = mapped.AsType<uint32_t>();
            log().Debug(u"{}: {}\n", buf->GetName(), sp);
        }
        common::mlog::SyncConsoleBackend();
    }
}

static void TestDX(DxDevice dev, DxComputeCmdQue cmdque, std::string fpath)
{
    bool runnable = false;
    while (true)
    {
        const auto ch = fpath.back();
        if (ch == '@')
            runnable = true;
        else
            break;
        fpath.pop_back();
    }
    common::fs::path filepath = fpath;
    const bool isNLDX = filepath.extension().string() == ".nldx";
    log().Debug(u"loading hlsl file [{}]\n", filepath.u16string());
    try
    {
        const auto kertxt = common::file::ReadAllText(filepath);
        DxShaderConfig config;
        std::unique_ptr<NLDXResult> nldxRes;
        std::vector<std::pair<std::string, DxShader>> shaders;
        RunInfo runInfo;
        if (isNLDX)
        {
            static const NLDXProcessor NLDXProc;
            const auto prog = NLDXProc.Parse(common::as_bytes(common::to_span(kertxt)), filepath.u16string());
            prog->AttachExtension([&](auto&, auto& ctx) { return StubHelper.GenerateExtension(ctx, runInfo); });
            nldxRes = NLDXProc.CompileShader(prog, dev, {}, config);
            common::file::WriteAll(fpath + ".hlsl", nldxRes->GetNewSource());
            shaders.assign(nldxRes->GetShaders().begin(), nldxRes->GetShaders().end());
        }
        else
        {
            shaders.emplace_back("main", DxShader_::CreateAndBuild(dev, ShaderType::Compute, kertxt, config));
        }
        std::vector<std::pair<std::string, DxComputeProgram>> progs;
        for (const auto& [name, shader] : shaders)
        {
            progs.emplace_back(name, std::make_shared<DxComputeProgram_>(shader));
        }
        for (const auto& [name, prog] : progs)
        {
            const auto& tgSize = prog->GetThreadGroupSize();
            log().Success(u"program [{}], TGSize[{}x{}x{}]:\n", name, tgSize[0], tgSize[1], tgSize[2]);
            for (const auto& item : prog->BufSlots())
            {
                log().Verbose(u"---[Buffer][{}] Space[{}] Bind[{},{}] Type[{}]\n", item.Name, item.Space, item.BindReg, item.Count,
                    dxu::detail::GetBoundedResTypeName(item.Type));
            }
        }
        if (runnable)
        {
            const auto rg = cmdque->DeclareRange(FMTSTR2(u"Test run of [{}]", filepath.u16string()));
            RunKernel(dev, cmdque, progs[0].second);
        }
    }
    catch (const BaseException& be)
    {
        PrintException(be, u"Error here");
    }
}

static void DXStub()
{
    const auto devs = DxDevice_::GetDevices();
    const auto& xcdevs = xcomp::ProbeDevice();
    const bool isAuto = IsAutoMode();
    if (devs.size() == 0)
    {
        log().Error(u"No DirectX12 devices found!\n");
        return;
    }
    while (true)
    {
        constexpr auto GetFeatLvStr = [](dxu::FeatureLevels lv)
        {
            switch (lv)
            {
            case dxu::FeatureLevels::Core1: return u"Core1"sv;
            case dxu::FeatureLevels::Dx9:   return u"Dx9"sv;
            case dxu::FeatureLevels::Dx10:  return u"Dx10"sv;
            case dxu::FeatureLevels::Dx11:  return u"Dx11"sv;
            case dxu::FeatureLevels::Dx12:  return u"Dx12"sv;
            default:                        return u"Error"sv;
            }
        };
        const auto devidx = SelectIdx(devs, u"device", [&](DxDevice dev)
        {
            return FMTSTR2(u"[{}][@{:1}]{} \t {:5}{{SM{}.{}}}[{:2}|{:3}|{:3}]", 
                dev->PCIEAddress, GetIdx36(xcdevs.GetDeviceIndex(dev->XCompDevice)), dev->AdapterName,
                GetFeatLvStr(dev->FeatureLevel), dev->SMVer / 10, dev->SMVer % 10, 
                dev->IsSoftware ? u"SW"sv : u"HW"sv, dev->IsTBR ? u"TBR"sv : u""sv, dev->IsUMA ? u"UMA"sv : u""sv);
        }, isAuto ? 0u : std::optional<uint32_t>{});
        const auto& dev = devs[devidx];
        const auto cmdque = DxComputeCmdQue_::Create(dev);
        try
        {
            const auto& capture = DxCapture::Get();
            while (true)
            {
                const auto cap = capture.CaptureRange();
                common::mlog::SyncConsoleBackend();
                string fpath = common::console::ConsoleEx::ReadLine("input hlsl file:");
                if (fpath == "BREAK")
                    break;
                else if (fpath == "clear")
                {
                    GetConsole().ClearConsole();
                    continue;
                }
                else if (fpath == "BUFTEST")
                {
                    BufTest(dev, cmdque);
                    continue;
                }
                else if (fpath == "INFO")
                {

                    std::u16string infotxt;
#define ADD_INFO(info) APPEND_FMT(infotxt, u"" #info ": [{}]\n"sv, dev->info)
                    ADD_INFO(AdapterName);
                    ADD_INFO(SMVer);
                    APPEND_FMT(infotxt, u"WaveSize: [{} - {}]\n"sv, dev->WaveSize.first, dev->WaveSize.second);
                    ADD_INFO(IsTBR);
                    ADD_INFO(IsUMA);
                    ADD_INFO(IsUMACacheCoherent);
                    ADD_INFO(IsIsolatedMMU);
                    ADD_INFO(SupportFP64);
                    ADD_INFO(SupportINT64);
                    ADD_INFO(SupportFP16);
                    ADD_INFO(SupportINT16);
                    ADD_INFO(SupportROV);
                    ADD_INFO(ExtComputeResState);
                    ADD_INFO(DepthBoundTest);
                    ADD_INFO(CopyQueueTimeQuery);
                    ADD_INFO(BackgroundProcessing);
                    APPEND_FMT(infotxt, u"LUID: [{}]\n"sv, common::MiscIntrin.HexToStr(dev->GetLUID()));
#undef ADD_INFO
                    log().Verbose(u"Device Info:\n{}\n", infotxt);
                    continue;
                }
                else if (fpath.empty())
                    continue;
                TestDX(dev, cmdque, fpath);
            }
        }
        catch (const common::BaseException& be)
        {
            PrintException(be, u"Error");
        }
    }
}

const static uint32_t ID = RegistTest("DXStub", &DXStub);
