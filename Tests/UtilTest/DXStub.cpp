#include "TestRely.h"
#include "DirectXUtil/DxDevice.h"
#include "DirectXUtil/DxCmdQue.h"
#include "DirectXUtil/DxResource.h"
#include "DirectXUtil/DxBuffer.h"
#include "DirectXUtil/DxShader.h"
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


static void DXStub()
{
    const auto& devs = DxDevice_::GetDevices();
    if (devs.size() == 0)
    {
        log().error(u"No DirectX12 devices found!\n");
        return;
    }
    while (true)
    {
        const auto devidx = SelectIdx(devs, u"device", [](DxDevice dev) 
            {
                return FMTSTR(u"{} [SM{}.{}]\t {:3} {:3}", dev->AdapterName, dev->SMVer / 10, dev->SMVer % 10,
                    dev->IsTBR() ? u"TBR"sv : u""sv, dev->IsUMA() ? u"UMA"sv : u""sv);
            });
        const auto& dev = devs[devidx];
        const auto cmdque = DxComputeCmdQue_::Create(dev);
        const auto cmdlist = DxComputeCmdList_::Create(dev);
        //const auto buf = DxBuffer_::Create(dev, HeapType::Upload, HeapFlags::Empty, 1024576, ResourceFlags::Empty);
        const auto buf = DxBuffer_::Create(dev, {CPUPageProps::WriteBack, MemPrefer::PreferCPU}, HeapFlags::Empty, 1024576, ResourceFlags::Empty);
        const auto region = buf->Map(0, 4096);
        for (auto& item : region.AsType<float>())
        {
            item = 1.0f;
        }
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
            else if (fpath.empty())
                continue;
            common::fs::path filepath = fpath;
            log().debug(u"loading hlsl file [{}]\n", filepath.u16string());
            try
            {
                const auto kertxt = common::file::ReadAllText(filepath);
                auto shaderStub = DxShader_::Create(dev, ShaderType::Compute, kertxt);
                shaderStub.Build({});
                auto shader = shaderStub.Finish();

                for (const auto& item : shader->BufSlots())
                {
                    log().verbose(u"---[Buffer][{}] Bind[{},{}] Type[{}]\n", item.Name, item.Index, item.Count, 
                        dxu::DxShader_::GetBoundedResTypeName(item.Type));
                }
            }
            catch (const BaseException& be)
            {
                PrintException(be, u"Error here");
            }
        }
    }
}

const static uint32_t ID = RegistTest("DXStub", &DXStub);
