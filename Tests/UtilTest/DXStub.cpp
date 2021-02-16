#include "TestRely.h"
#include "DirectXUtil/DxDevice.h"
#include "DirectXUtil/DxCmdQue.h"
#include "DirectXUtil/DxResource.h"
#include "DirectXUtil/DxBuffer.h"
#include "DirectXUtil/DxShader.h"
#include "DirectXUtil/DxProgram.h"
#include "SystemCommon/ConsoleEx.h"
#include "StringUtil/Convert.h"
#include "common/Linq2.hpp"


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
        const auto cmdlist = DxComputeCmdList_::Create(dev);
        try
        {
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
                common::fs::path filepath = fpath;
                log().debug(u"loading hlsl file [{}]\n", filepath.u16string());
                try
                {
                    const auto kertxt = common::file::ReadAllText(filepath);
                    auto shaderStub = DxShader_::Create(dev, ShaderType::Compute, kertxt);
                    shaderStub.Build({});
                    const auto prog = std::make_shared<DxComputeProgram_>(shaderStub.Finish());
                    for (const auto& item : prog->BufSlots())
                    {
                        log().verbose(u"---[Buffer][{}] Bind[{},{}] Type[{}]\n", item.Name, item.BindReg, item.Count,
                            dxu::detail::GetBoundedResTypeName(item.Type));
                    }
                }
                catch (const BaseException& be)
                {
                    PrintException(be, u"Error here");
                }
            }
        }
        catch (const common::BaseException& be)
        {
            PrintException(be, u"Error");
        }
    }
}

const static uint32_t ID = RegistTest("DXStub", &DXStub);
