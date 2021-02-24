#include "TestRely.h"
#include "DirectXUtil/DxDevice.h"
#include "DirectXUtil/DxCmdQue.h"
#include "DirectXUtil/DxBuffer.h"
#include "DirectXUtil/DxShader.h"
#include "DirectXUtil/DxProgram.h"
#include "DirectXUtil/NLDX.h"
#include "SystemCommon/ConsoleEx.h"
#include "StringUtil/Convert.h"
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


static void BufTest(DxDevice dev, DxComputeCmdQue cmdque)
{
    //const auto buf = DxBuffer_::Create(dev, { CPUPageProps::WriteBack, MemPrefer::PreferCPU }, HeapFlags::Empty, 1024576, ResourceFlags::Empty);
    const auto buf = DxBuffer_::Create(dev, HeapType::Default, HeapFlags::Empty, 1024576, ResourceFlags::Empty);

    {
        const auto map0 = buf->Map(cmdque, MapFlags::ReadOnly, 0, 1024);
        const auto sp0 = map0.AsType<uint8_t>();
        log().verbose("sp0: {}\n", sp0.subspan(0, 8));
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
        log().verbose("sp1: {}\n", sp1.subspan(0, 8));
    }
    {
        std::array<uint8_t, 256> tmp = { 0 };
        buf->ReadSpan(cmdque, tmp, 0)->WaitFinish();
        common::span<const uint8_t> sp2(tmp);
        log().verbose("sp2: {}\n", sp2.subspan(0, 8));
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
    auto prepare = prog->Prepare();
    std::vector<DxBuffer> bufs;
    {
        const auto rgBuf = cmdlist->DeclareRange(u"Prepare buffers");
        for (const auto& item : prog->BufSlots())
        {
            const auto flag = (item.Type & BoundedResourceType::CategoryMask) == BoundedResourceType::UAVs ?
                ResourceFlags::AllowUnorderAccess : ResourceFlags::Empty;
            const auto buf = DxBuffer_::Create(dev, HeapType::Default, HeapFlags::Empty, 4096, flag);
            buf->SetName(common::str::to_u16string(item.Name, common::str::Charset::UTF8));
            switch (item.Type & BoundedResourceType::InnerTypeMask)
            {
            case BoundedResourceType::InnerTyped:
                prepare.SetBuf(item.Name, buf->CreateTypedView(xziar::img::TextureFormat::RGBA8, 1024));
                log().verbose(u"Attach Typed view to [{}]({}) Type[{}]\n", item.Name, item.BindReg,
                    dxu::detail::GetBoundedResTypeName(item.Type));
                break;
            case BoundedResourceType::InnerStruct:
                prepare.SetBuf(item.Name, buf->CreateStructuredView<uint32_t>(1024));
                log().verbose(u"Attach Structured view to [{}]({}) Type[{}]\n", item.Name, item.BindReg,
                    dxu::detail::GetBoundedResTypeName(item.Type));
                break;
            case BoundedResourceType::InnerRaw:
                prepare.SetBuf(item.Name, buf->CreateRawView(1024));
                log().verbose(u"Attach Raw view to [{}]({}) Type[{}]\n", item.Name, item.BindReg,
                    dxu::detail::GetBoundedResTypeName(item.Type));
                break;
            case BoundedResourceType::InnerCBuf:
                prepare.SetBuf(item.Name, buf->CreateConstBufView(4096));
                log().verbose(u"Attach Const Buf view to [{}]({}) Type[{}]\n", item.Name, item.BindReg,
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
    {
        const auto rgExe = cmdlist->DeclareRange(u"Execute shader");
        call.Execute(cmdlist, { 1024, 1, 1 });
    }
    cmdlist->EnsureClosed();
    cmdque->Execute(cmdlist)->WaitFinish();
    {
        const auto rgExe = cmdque->DeclareRange(u"Readback buffer");
        for (const auto& buf : bufs)
        {
            const auto mapped = buf->Map(cmdque, MapFlags::ReadOnly, 0, 64);
            const auto sp = mapped.AsType<uint32_t>();
            const auto vals = fmt::format("{}", sp);
            log().debug(u"{}: {}\n", buf->GetName(), vals);
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
    log().debug(u"loading hlsl file [{}]\n", filepath.u16string());
    try
    {
        const auto kertxt = common::file::ReadAllText(filepath);
        DxShaderConfig config;
        std::unique_ptr<NLDXResult> nldxRes;
        DxShader shader;
        if (isNLDX)
        {
            static const NLDXProcessor NLDXProc;
            const auto prog = NLDXProc.Parse(common::as_bytes(common::to_span(kertxt)), filepath.u16string());
            //prog->AttachExtension([&](auto&, auto& ctx) { return std::make_unique<CLStubExtension>(ctx, runInfo); });
            nldxRes = NLDXProc.CompileShader(prog, dev, {}, config);
            common::file::WriteAll(fpath + ".hlsl", nldxRes->GetNewSource());
            shader = nldxRes->GetShader();
        }
        else
        {
            shader = DxShader_::CreateAndBuild(dev, ShaderType::Compute, kertxt, config);
        }
        const auto prog = std::make_shared<DxComputeProgram_>(shader);
        for (const auto& item : prog->BufSlots())
        {
            log().verbose(u"---[Buffer][{}] Space[{}] Bind[{},{}] Type[{}]\n", item.Name, item.Space, item.BindReg, item.Count,
                dxu::detail::GetBoundedResTypeName(item.Type));
        }
        if (runnable)
        {
            const auto rg = cmdque->DeclareRange(FMTSTR(u"Test run of [{}]", filepath.u16string()));
            RunKernel(dev, cmdque, prog);
        }
    }
    catch (const BaseException& be)
    {
        PrintException(be, u"Error here");
    }
}

static void DXStub()
{
    const auto& devs = DxDevice_::GetDevices();
    const bool isAuto = common::container::FindInVec(GetCmdArgs(), [](const auto str) { return str == "auto"; }) != nullptr;
    if (devs.size() == 0)
    {
        log().error(u"No DirectX12 devices found!\n");
        return;
    }
    while (true)
    {
        const auto devidx = isAuto ? 0u : SelectIdx(devs, u"device", [](DxDevice dev)
            {
                return FMTSTR(u"{} [SM{}.{}]\t {:3} {:3}", dev->AdapterName, dev->SMVer / 10, dev->SMVer % 10,
                    dev->IsTBR() ? u"TBR"sv : u""sv, dev->IsUMA() ? u"UMA"sv : u""sv);
            });
        const auto& dev = devs[devidx];
        const auto cmdque = DxComputeCmdQue_::Create(dev);
        try
        {
            const auto& capture = DxCapture::Get();
            const auto cap = capture.CaptureRange();
            while (true)
            {
                common::mlog::SyncConsoleBackend();
                string fpath = common::console::ConsoleEx::ReadLine("input hlsl file:");
                if (fpath == "BREAK")
                    break;
                else if (fpath == "clear")
                {
                    common::console::ConsoleEx::ClearConsole();
                    continue;
                }
                else if (fpath == "BUFTEST")
                {
                    BufTest(dev, cmdque);
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
