#include "oclPch.h"
#include "oclPlatform.h"
#include "oclUtil.h"


extern "C"
{
    extern size_t GetOpenCLIcdLoaderDllNames(bool, char*);
}


namespace oclu
{
using namespace std::string_view_literals;
using std::string;
using std::u16string;
using std::string_view;
using std::u16string_view;
using std::vector;
using common::str::Encoding;

MAKE_ENABLER_IMPL(oclPlatform_)



common::span<const oclPlatform> oclPlatform_::GetPlatforms()
{
    struct PlatformHolder
    {
        std::string IcdDllNames;
        std::string NonIcdDllNames;
        std::vector<std::unique_ptr<oclPlatform_>> Platforms;
        std::vector<std::unique_ptr<detail::PlatFuncs>> Funcs;
        std::vector<oclPlatform> Plats;
    };
    static const auto Holder = []()
    {
        PlatformHolder holder;
        std::u16string msg;
        {
            const auto ele0 = GetOpenCLIcdLoaderDllNames(true, nullptr);
            holder.IcdDllNames.resize(ele0);
            GetOpenCLIcdLoaderDllNames(true, holder.IcdDllNames.data());
            const auto ele1 = GetOpenCLIcdLoaderDllNames(false, nullptr);
            holder.NonIcdDllNames.resize(ele1);
            GetOpenCLIcdLoaderDllNames(false, holder.NonIcdDllNames.data());
        }
        const auto InitPlatform = [&](bool isIcd, const detail::PlatFuncs* platFuncs)
        {
            cl_uint numPlatforms = 0;
            platFuncs->clGetPlatformIDs(0, nullptr, &numPlatforms);
            //Get all Platforms
            vector<cl_platform_id> platformIDs(numPlatforms);
            platFuncs->clGetPlatformIDs(numPlatforms, platformIDs.data(), nullptr);
            for (const auto& pID : platformIDs)
            {
                auto plat = MAKE_ENABLER_UNIQUE(oclPlatform_, (platFuncs, pID));
                try
                {
                    plat->InitDevice();
                    holder.Platforms.emplace_back(std::move(plat));
                }
                catch (const std::exception& ex)
                {
                    oclLog().error(FMT_STRING(u"Failed to init platform [{}]({}):\n {}\n"), 
                        plat->Name, isIcd ? u"icd"sv: u"nonicd"sv, ex.what());
                }
            }
        };
        // init icds
        {
            msg.append(u"icd dlls:\n");
            for (const auto dll : common::str::SplitStream(holder.IcdDllNames, '\0', false))
                msg.append(common::str::to_u16string(dll)).append(u"\n");

            holder.Funcs.emplace_back(std::make_unique<detail::PlatFuncs>());
            InitPlatform(true, holder.Funcs.back().get());
        }
        // init non-icds
        {
            msg.append(u"nonicd dlls:\n");
            for (const auto dll : common::str::SplitStream(holder.NonIcdDllNames, '\0', false))
            {
                msg.append(common::str::to_u16string(dll)).append(u"\n");
                auto platFuncs = std::make_unique<detail::PlatFuncs>(dll);
                if (!platFuncs->clGetPlatformIDs) continue;

                InitPlatform(true, platFuncs.get());
                holder.Funcs.emplace_back(std::move(platFuncs));
            }
        }
        oclLog().debug(msg);
        holder.Plats.reserve(holder.Platforms.size());
        for (const auto& plat : holder.Platforms)
            holder.Plats.emplace_back(plat.get());
        return holder;
    }();
    return Holder.Plats;
}

vector<intptr_t> oclPlatform_::GetCLProps() const
{
    vector<intptr_t> props{ CL_CONTEXT_PLATFORM, (cl_context_properties)*PlatformID };
    props.push_back(0);
    return props;
}


static Vendors JudgeBand(const u16string& name)
{
    const auto capName = common::str::ToUpperEng(name);
    if (capName.find(u"NVIDIA") != u16string::npos)
        return Vendors::NVIDIA;
    else if (capName.find(u"AMD") != u16string::npos)
        return Vendors::AMD;
    else if (capName.find(u"INTEL") != u16string::npos)
        return Vendors::Intel;
    else if (capName.find(u"ARM") != u16string::npos)
        return Vendors::ARM;
    else if (capName.find(u"QUALCOMM") != u16string::npos)
        return Vendors::Qualcomm;
    else
        return Vendors::Other;
}

static string GetStr(const detail::PlatFuncs* funcs, const cl_platform_id platformID, const cl_platform_info type)
{
    thread_local string ret;
    size_t size = 0;
    funcs->clGetPlatformInfo(platformID, type, 0, nullptr, &size);
    ret.resize(size, '\0');
    funcs->clGetPlatformInfo(platformID, type, size, ret.data(), &size);
    if(!ret.empty())
        ret.pop_back();
    return ret;
}
static u16string GetUStr(const detail::PlatFuncs* funcs, const cl_platform_id platformID, const cl_platform_info type)
{
    const auto u8str = GetStr(funcs, platformID, type);
    return u16string(u8str.cbegin(), u8str.cend()); 
}

oclPlatform_::oclPlatform_(const detail::PlatFuncs* funcs, void* pID) : 
    detail::oclCommon(funcs), PlatformID(pID), DefDevice(nullptr)
{
    Extensions = common::str::Split(GetStr(Funcs, *PlatformID, CL_PLATFORM_EXTENSIONS), ' ', false);
    Name    = GetUStr(Funcs, *PlatformID, CL_PLATFORM_NAME);
    Ver     = GetUStr(Funcs, *PlatformID, CL_PLATFORM_VERSION);
    BeignetFix = Ver.find(u"beignet") != u16string::npos;
    {
        const auto version = ParseVersionString(Ver, 1);
        Version = version.first * 10 + version.second;
    }
    PlatVendor = JudgeBand(Name);
}

void oclPlatform_::InitDevice()
{
    cl_uint numDevices;
    if (!LogCLError(Funcs->clGetDeviceIDs(*PlatformID, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices),
        u"Failed to get device ids"))
        return;
    // Get all Device Info
    vector<cl_device_id> DeviceIDs(numDevices);
    Funcs->clGetDeviceIDs(*PlatformID, CL_DEVICE_TYPE_ALL, numDevices, DeviceIDs.data(), nullptr);
    cl_device_id defDevID;
    Funcs->clGetDeviceIDs(*PlatformID, CL_DEVICE_TYPE_DEFAULT, 1, &defDevID, nullptr);

    size_t defIdx = SIZE_MAX;
    Devices.reserve(numDevices);
    for (const auto id : DeviceIDs)
    {
        try
        {
            Devices.push_back(oclDevice_(this, id));
            if (id == defDevID) defIdx = Devices.size() - 1;
        }
        catch (const std::exception& ex)
        {
            oclLog().error(FMT_STRING(u"Failed to init platform[{}]'s device[{}]:\n {}\n"),
                Name, (void*)id, ex.what());
        }
    }
    
    if (defIdx != SIZE_MAX)
        DefDevice = &Devices[defIdx];
}

oclContext oclPlatform_::CreateContext(const vector<oclDevice>& devs, const vector<cl_context_properties>& props) const
{

    for (const auto& dev : devs)
    {
        if (dev < &Devices.front() || dev > &Devices.back())
        {
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, 
                u"cannot using device from other platform").Attach("dev", dev);
        }
    }
    return MAKE_ENABLER_SHARED(oclContext_, (this, props, devs));
}

vector<oclDevice> oclPlatform_::GetDevices() const
{
    return common::linq::FromIterable(Devices)
        .Select([](const auto& dev) { return &dev; })
        .ToVector();
}

oclContext oclPlatform_::CreateContext() const
{
    return CreateContext(GetDevices());
}


}
