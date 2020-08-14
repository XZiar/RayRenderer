#include "TestRely.h"
#include "DirectXUtil/dxDevice.h"
#include "DirectXUtil/dxCmdQue.h"
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
        const auto devidx = SelectIdx(devs, u"device", [](const auto& dev) 
            {
                return dev.GetAdapterName();
            });
        const auto& dev = devs[devidx];
        const auto cmdque = DXComputeCmdQue_::Create(&dev);
    }
}

const static uint32_t ID = RegistTest("DXStub", &DXStub);
