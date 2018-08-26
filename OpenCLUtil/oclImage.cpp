#include "oclRely.h"
#include "oclImage.h"
#include "oclException.h"
#include "oclUtil.h"


namespace oclu::detail
{

void CL_CALLBACK OnMemDestroyed(cl_mem memobj, void *user_data)
{
    const auto& buf = *reinterpret_cast<_oclImage*>(user_data);
    oclLog().debug(u"oclImage {:p} with size [{}x{}], being destroyed.\n", (void*)memobj, buf.Width, buf.Height);
    //async callback, should not access cl-func since buffer may be released at any time.
    //size_t size = 0;
    //clGetMemObjectInfo(memobj, CL_MEM_SIZE, sizeof(size), &size, nullptr);
}

using oglu::TextureDataFormat;


static cl_image_format ParseImageFormat(const TextureDataFormat dformat)
{
    cl_image_format format;
    if (HAS_FIELD(dformat, TextureDataFormat::NORMAL_MASK))
    {
        switch (dformat & TextureDataFormat::TYPE_MASK)
        {
        case TextureDataFormat::U8_TYPE:     format.image_channel_data_type = CL_UNORM_INT8; break;
        case TextureDataFormat::I8_TYPE:     format.image_channel_data_type = CL_SNORM_INT8; break;
        case TextureDataFormat::U16_TYPE:    format.image_channel_data_type = CL_UNORM_INT16; break;
        case TextureDataFormat::I16_TYPE:    format.image_channel_data_type = CL_SNORM_INT16; break;
        default: COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"unsupported format");
        }
    }
    else
    {
        switch (dformat & TextureDataFormat::TYPE_MASK)
        {
        case TextureDataFormat::U8_TYPE:     format.image_channel_data_type = CL_UNSIGNED_INT8; break;
        case TextureDataFormat::I8_TYPE:     format.image_channel_data_type = CL_SIGNED_INT8; break;
        case TextureDataFormat::U16_TYPE:    format.image_channel_data_type = CL_UNSIGNED_INT16; break;
        case TextureDataFormat::I16_TYPE:    format.image_channel_data_type = CL_SIGNED_INT16; break;
        case TextureDataFormat::U32_TYPE:    format.image_channel_data_type = CL_UNSIGNED_INT32; break;
        case TextureDataFormat::I32_TYPE:    format.image_channel_data_type = CL_SIGNED_INT32; break;
        case TextureDataFormat::HALF_TYPE:   format.image_channel_data_type = CL_HALF_FLOAT; break;
        case TextureDataFormat::FLOAT_TYPE:  format.image_channel_data_type = CL_FLOAT; break;
        default: COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"unsupported format"); // should not enter
        }
    }
    switch (dformat & TextureDataFormat::RAW_FORMAT_MASK)
    {
    case TextureDataFormat::R_FORMAT:        format.image_channel_order = CL_R; break;
    case TextureDataFormat::RG_FORMAT:       format.image_channel_order = CL_RG; break;
    case TextureDataFormat::RGB_FORMAT:      format.image_channel_order = CL_RGB; break;
    case TextureDataFormat::RGBA_FORMAT:     format.image_channel_order = CL_RGBA; break;
    case TextureDataFormat::BGRA_FORMAT:     format.image_channel_order = CL_BGRA; break;
    default: COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"unsupported format");
    }
    return format;
}

static cl_image_desc ParseImageDesc(const uint32_t width, const uint32_t height)
{
    cl_image_desc desc;
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    desc.image_width = width;
    desc.image_height = height;
    desc.image_depth = 1;
    desc.image_array_size = 0;
    desc.image_row_pitch = 0;
    desc.image_slice_pitch = 0;
    desc.num_mip_levels = 0;
    desc.num_samples = 0;
    desc.buffer = 0;
    return desc;
}
static cl_mem CreateMem(const cl_context ctx, const cl_mem_flags flag, const cl_image_desc& desc, const oglu::TextureDataFormat dformat)
{
    cl_int errcode;
    const auto format = ParseImageFormat(dformat);
    const auto id = clCreateImage(ctx, flag, &format, &desc, nullptr, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot create image", errcode));
    return id;
}

_oclImage::_oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const oglu::TextureDataFormat dformat, const cl_mem id)
    :Context(ctx), Width(width), Height(height), Flags(flag), Format(dformat), memID(id)
{
    clSetMemObjectDestructorCallback(memID, &OnMemDestroyed, this);
}
_oclImage::_oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const oglu::TextureDataFormat dformat)
    : _oclImage(ctx, flag, width, height, dformat, CreateMem(ctx->context, (cl_mem_flags)flag, ParseImageDesc(width, height), dformat))
{ }

_oclImage::~_oclImage()
{
#ifdef _DEBUG
    uint32_t refCount = 0;
    clGetMemObjectInfo(memID, CL_MEM_REFERENCE_COUNT, sizeof(uint32_t), &refCount, nullptr);
    if (refCount == 1)
    {
        oclLog().debug(u"oclImage {:p} with size [{}x{}], has {} reference being release.\n", (void*)memID, Width, Height, refCount);
        clReleaseMemObject(memID);
    }
    else
        oclLog().warning(u"oclImage {:p} with size [{}x{}], has {} reference and not able to release.\n", (void*)memID, Width, Height, refCount);
#else
    clReleaseMemObject(memID);
#endif
}


oclPromise _oclImage::Read(const oclCmdQue que, Image& image, const bool shouldBlock) const
{
    image = Image(oglu::TexFormatUtil::ConvertFormat(Format));
    image.SetSize(Width, Height);
    constexpr size_t origin[3] = { 0,0,0 };
    const size_t region[3] = { Width,Height,1 };
    cl_event e;
    const auto ret = clEnqueueReadImage(que->cmdque, memID, shouldBlock ? CL_TRUE : CL_FALSE, origin, region, 0, 0, image.GetRawPtr(), 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot read clImage", ret));
    if (shouldBlock)
        return {};
    else
        return std::make_shared<detail::oclPromise_>(detail::oclPromise_(e, que->cmdque));
}

oclPromise _oclImage::Write(const oclCmdQue que, const Image& image, const bool shouldBlock) const
{
    if (image.GetWidth() != Width || image.GetHeight() != Height)
        COMMON_THROW(BaseException, u"write size unmatch");
    return Write(que, image.GetData(), shouldBlock);
}

oclPromise _oclImage::Write(const oclCmdQue que, const common::AlignedBuffer<32>& data, const bool shouldBlock) const
{
    constexpr size_t origin[3] = { 0,0,0 };
    const size_t region[3] = { Width,Height,1 };
    cl_event e;
    const auto ret = clEnqueueWriteImage(que->cmdque, memID, shouldBlock ? CL_TRUE : CL_FALSE, origin, region, 0, 0, data.GetRawPtr(), 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot write clImage", ret));
    if (shouldBlock)
        return {};
    else
        return std::make_shared<detail::oclPromise_>(detail::oclPromise_(e, que->cmdque));
}


_oclGLImage::_oclGLImage(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D tex)
    : _oclImage(ctx, flag, UINT32_MAX, UINT32_MAX, oglu::TexFormatUtil::DecideFormat(tex->GetInnerFormat()),
        CreateMemFromGLTex(ctx->context, (cl_mem_flags)flag, *tex)),
    GlTex(tex)
{ }

_oclGLImage::~_oclGLImage()
{
}


}
