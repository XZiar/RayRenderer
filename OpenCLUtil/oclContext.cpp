#include "oclRely.h"
#include "oclContext.h"
#include "oclException.h"
#include "oclDevice.h"
#include "oclUtil.h"
#include "oclImage.h"


using common::container::FindInVec;
namespace oclu
{


static void CL_CALLBACK onNotify(const char * errinfo, [[maybe_unused]]const void * private_info, size_t, void *user_data)
{
    const oclContext_& ctx = *(oclContext_*)user_data;
    const auto u16Info = common::strchset::to_u16string(errinfo, common::strchset::Charset::UTF8);
    oclLog().verbose(u"{}\n", u16Info);
    if (ctx.onMessage)
        ctx.onMessage(u16Info);
    return;
}

cl_context oclContext_::CreateContext(vector<cl_context_properties>& props, const vector<oclDevice>& devices, void* self)
{
    cl_int ret;
    vector<cl_device_id> deviceIDs;
    bool supportIntelDiag = true;
    for (const auto& dev : devices)
    {
        deviceIDs.push_back(dev->deviceID);
        supportIntelDiag &= dev->Extensions.Has("cl_intel_driver_diagnostics");
    }
    constexpr cl_context_properties intelDiagnostics = CL_CONTEXT_DIAGNOSTICS_LEVEL_BAD_INTEL | CL_CONTEXT_DIAGNOSTICS_LEVEL_GOOD_INTEL | CL_CONTEXT_DIAGNOSTICS_LEVEL_NEUTRAL_INTEL;
    if (supportIntelDiag)
        props.insert(props.cend(), { CL_CONTEXT_SHOW_DIAGNOSTICS_INTEL, intelDiagnostics }); 
    const auto ctx = clCreateContext(props.data(), (cl_uint)deviceIDs.size(), deviceIDs.data(), &onNotify, self, &ret);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot create opencl-context");
    return ctx;
}

extern xziar::img::TextureFormat ParseCLImageFormat(const cl_image_format& format);

static common::container::FrozenDenseSet<xziar::img::TextureFormat> GetSupportedImageFormat(const cl_context& ctx, const cl_mem_object_type type)
{
    cl_uint count;
    clGetSupportedImageFormats(ctx, CL_MEM_READ_ONLY, type, 0, nullptr, &count);
    vector<cl_image_format> formats(count);
    clGetSupportedImageFormats(ctx, CL_MEM_READ_ONLY, type, count, formats.data(), &count);
    set<xziar::img::TextureFormat, std::less<>> dformats;
    for (const auto format : formats)
        dformats.insert(ParseCLImageFormat(format));
    return dformats;
}

oclContext_::oclContext_(const std::shared_ptr<const oclPlatform_>& plat, vector<cl_context_properties> props, const vector<oclDevice>& devices, const u16string name, const Vendor thevendor)
    : Plat(plat), Context(CreateContext(props, devices, this)), Devices(devices), PlatformName(name),
    Img2DFormatSupport(GetSupportedImageFormat(Context, CL_MEM_OBJECT_IMAGE2D)), Img3DFormatSupport(GetSupportedImageFormat(Context, CL_MEM_OBJECT_IMAGE3D)),
    vendor(thevendor) { }

oclDevice oclContext_::GetDevice(const cl_device_id devid) const
{
    const auto it = FindInVec(Devices, [=](const oclDevice& dev) {return dev->deviceID == devid; });
    return it ? *it : oclDevice{};
}

oclContext_::~oclContext_()
{
#ifdef _DEBUG
    uint32_t refCount = 0;
    clGetContextInfo(Context, CL_CONTEXT_REFERENCE_COUNT, sizeof(uint32_t), &refCount, nullptr);
    if (refCount == 1)
    {
        oclLog().debug(u"oclContext {:p} named {}, has {} reference being release.\n", (void*)Context, PlatformName, refCount);
        clReleaseContext(Context);
    }
    else
        oclLog().warning(u"oclContext {:p} named {}, has {} reference and not able to release.\n", (void*)Context, PlatformName, refCount);
#else
    clReleaseContext(context);
#endif
}

oclDevice oclContext_::GetGPUDevice() const
{
    const auto gpuDev = std::find_if(Devices.cbegin(), Devices.cend(), [&](const oclDevice& dev)
    {
        return dev->Type == DeviceType::GPU;
    });
    return gpuDev == Devices.cend() ? oclDevice{} : *gpuDev;
}

}
