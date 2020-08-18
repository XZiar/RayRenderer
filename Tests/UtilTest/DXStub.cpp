#include "TestRely.h"
#include "DirectXUtil/dxDevice.h"
#include "DirectXUtil/dxCmdQue.h"
#include "DirectXUtil/dxResource.h"
#include "DirectXUtil/dxBuffer.h"
#include "DirectXUtil/dxShader.h"
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
    const auto& devs = DXDevice_::GetDevices();
    if (devs.size() == 0)
    {
        log().error(u"No DirectX12 devices found!\n");
        return;
    }
    while (true)
    {
        const auto devidx = SelectIdx(devs, u"device", [](DXDevice dev) 
            {
                return FMTSTR(u"{} SM{:3.1f}\t {:3} {:3}", dev->AdapterName, dev->SMVer / 10.0f,
                    dev->IsTBR() ? u"TBR"sv : u""sv, dev->IsUMA() ? u"UMA"sv : u""sv);
            });
        const auto& dev = devs[devidx];
        const auto cmdque = DXComputeCmdQue_::Create(dev);
        const auto cmdlist = DXComputeCmdList_::Create(dev);
        const auto buf = DXBuffer_::Create(dev, HeapType::Upload, HeapFlags::Empty, 1024576, ResourceFlags::Empty);
        const auto region = buf->Map(0, 4096);
        for (auto& item : region.AsType<float>())
        {
            item = 1.0f;
        }
        auto shaderStub = DXShader_::Create(dev, ShaderType::Compute, "");
        shaderStub.Build({});
        auto shader = shaderStub.Finish();
    }
}

const static uint32_t ID = RegistTest("DXStub", &DXStub);
