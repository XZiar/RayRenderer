#include "oclRely.h"
#include "oclImage.h"
#include "oclUtil.h"
#include "oclPromise.hpp"


namespace oclu::detail
{

void CL_CALLBACK OnMemDestroyed(cl_mem memobj, void *user_data)
{
    const auto& buf = *reinterpret_cast<_oclImage*>(user_data);
    const auto[w, h, d] = buf.GetSize();
    oclLog().debug(u"oclImage {:p} with size [{}x{}x{}], being destroyed.\n", (void*)memobj, w, h, d);
    //async callback, should not access cl-func since buffer may be released at any time.
    //size_t size = 0;
    //clGetMemObjectInfo(memobj, CL_MEM_SIZE, sizeof(size), &size, nullptr);
}

using oglu::TextureDataFormat;


static cl_image_format ParseImageFormat(const TextureDataFormat dformat)
{
    cl_image_format format;
    if (HAS_FIELD(dformat, TextureDataFormat::INTEGER_MASK))
    {
        switch (dformat & TextureDataFormat::TYPE_RAW_MASK)
        {
        case TextureDataFormat::TYPE_U8:     format.image_channel_data_type = CL_UNSIGNED_INT8; break;
        case TextureDataFormat::TYPE_I8:     format.image_channel_data_type = CL_SIGNED_INT8; break;
        case TextureDataFormat::TYPE_U16:    format.image_channel_data_type = CL_UNSIGNED_INT16; break;
        case TextureDataFormat::TYPE_I16:    format.image_channel_data_type = CL_SIGNED_INT16; break;
        case TextureDataFormat::TYPE_U32:    format.image_channel_data_type = CL_UNSIGNED_INT32; break;
        case TextureDataFormat::TYPE_I32:    format.image_channel_data_type = CL_SIGNED_INT32; break;
        default: COMMON_THROW(OCLWrongFormatException, u"unsupported integer format", dformat); // should not enter
        }
    }
    else
    {
        switch (dformat & TextureDataFormat::TYPE_RAW_MASK)
        {
        case TextureDataFormat::TYPE_U8:     format.image_channel_data_type = CL_UNORM_INT8; break;
        case TextureDataFormat::TYPE_I8:     format.image_channel_data_type = CL_SNORM_INT8; break;
        case TextureDataFormat::TYPE_U16:    format.image_channel_data_type = CL_UNORM_INT16; break;
        case TextureDataFormat::TYPE_I16:    format.image_channel_data_type = CL_SNORM_INT16; break;
        case TextureDataFormat::TYPE_565:    format.image_channel_data_type = CL_UNORM_SHORT_565; break;
        case TextureDataFormat::TYPE_5551:   format.image_channel_data_type = CL_UNORM_SHORT_555; break;
        case TextureDataFormat::TYPE_10_2:   format.image_channel_data_type = CL_UNORM_INT_101010; break;
        case TextureDataFormat::TYPE_HALF:   format.image_channel_data_type = CL_HALF_FLOAT; break;
        case TextureDataFormat::TYPE_FLOAT:  format.image_channel_data_type = CL_FLOAT; break;
        default: COMMON_THROW(OCLWrongFormatException, u"unsupported normalized/float format", dformat);
        }
    }
    switch (dformat & TextureDataFormat::FORMAT_MASK)
    {
    case TextureDataFormat::FORMAT_R:        format.image_channel_order = CL_R; break;
    case TextureDataFormat::FORMAT_RG:       format.image_channel_order = CL_RG; break;
    case TextureDataFormat::FORMAT_RGB:      format.image_channel_order = CL_RGB; break;
    case TextureDataFormat::FORMAT_RGBA:     format.image_channel_order = CL_RGBA; break;
    case TextureDataFormat::FORMAT_BGRA:     format.image_channel_order = CL_BGRA; break;
    default: COMMON_THROW(OCLWrongFormatException, u"unsupported channel", dformat);
    }
    if (format.image_channel_order == CL_RGB)
    {
        if (format.image_channel_data_type != CL_UNORM_SHORT_565 && format.image_channel_data_type != CL_UNORM_SHORT_555 && format.image_channel_data_type != CL_UNORM_INT_101010)
            COMMON_THROW(OCLWrongFormatException, u"unsupported format for RGB channel image", dformat);
    }
    if (format.image_channel_order == CL_BGRA)
    {
        const auto rdtype = dformat & TextureDataFormat::TYPE_RAW_MASK;
        if (rdtype != TextureDataFormat::TYPE_U8 && rdtype != TextureDataFormat::TYPE_I8)
            COMMON_THROW(OCLWrongFormatException, u"unsupported format for BGRA channel image", dformat);
    }
    return format;
}

static cl_image_desc CreateImageDesc(cl_mem_object_type type, const uint32_t width, const uint32_t height, const uint32_t depth = 1)
{
    cl_image_desc desc;
    desc.image_type = type;
    desc.image_width = width;
    desc.image_height = height;
    desc.image_depth = depth;
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

_oclImage::_oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const oglu::TextureDataFormat dformat, const cl_mem id)
    :Context(ctx), Width(width), Height(height), Depth(depth), Flags(flag), Format(dformat), memID(id)
{
    clSetMemObjectDestructorCallback(memID, &OnMemDestroyed, this);
}
_oclImage::_oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const oglu::TextureDataFormat dformat, cl_mem_object_type type)
    :_oclImage(ctx, flag, width, height, depth, dformat, CreateMem(ctx->context, (cl_mem_flags)flag, CreateImageDesc(type, width, height, depth), dformat))
{ }
PromiseResult<void> _oclImage::ReturnPromise(cl_event e, const oclCmdQue que)
{
    return std::make_shared<detail::oclPromiseVoid>(e, que->cmdque);
}


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


PromiseResult<void> _oclImage::Write(const oclCmdQue que, const void *data, const size_t size, const bool shouldBlock) const
{
    constexpr size_t origin[3] = { 0,0,0 };
    if (Width*Height*Depth*oglu::TexFormatUtil::ParseFormatSize(Format) > size)
        COMMON_THROW(BaseException, u"write size not sufficient");
    const size_t region[3] = { Width,Height,Depth };
    cl_event e;
    const auto ret = clEnqueueWriteImage(que->cmdque, memID, shouldBlock ? CL_TRUE : CL_FALSE, origin, region, 0, 0, data, 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot write clImage", ret));
    if (shouldBlock)
        return {};
    else
        return ReturnPromise(e, que);
}
PromiseResult<void> _oclImage::Write(const oclCmdQue que, const Image& image, const bool shouldBlock) const
{
    if (image.GetWidth() != Width || image.GetHeight() != Height * Depth)
        COMMON_THROW(BaseException, u"write image size mismatch");
    const auto wantFormat = oglu::TexFormatUtil::ConvertToImgType(Format, true);
    if (wantFormat != image.GetDataType())
        COMMON_THROW(OCLWrongFormatException, u"image datatype mismatch", Format, std::any(image.GetDataType()));
    return Write(que, image.GetRawPtr(), image.GetSize(), shouldBlock);
}
PromiseResult<void> _oclImage::Read(const oclCmdQue que, void *data, const bool shouldBlock) const
{
    constexpr size_t origin[3] = { 0,0,0 };
    const size_t region[3] = { Width,Height,Depth };
    cl_event e;
    const auto ret = clEnqueueReadImage(que->cmdque, memID, shouldBlock ? CL_TRUE : CL_FALSE, origin, region, 0, 0, data, 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(u"cannot read clImage", ret));
    if (shouldBlock)
        return {};
    else
        return ReturnPromise(e, que);
}
PromiseResult<void> _oclImage::Read(const oclCmdQue que, Image& image, const bool shouldBlock) const
{
    image = Image(oglu::TexFormatUtil::ConvertToImgType(Format, true));
    image.SetSize(Width, Height*Depth);
    return _oclImage::Read(que, image.GetRawPtr(), shouldBlock);
}


_oclImage2D::_oclImage2D(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const oglu::TextureDataFormat dformat)
    : _oclImage(ctx, flag, width, height, 1, dformat, CL_MEM_OBJECT_IMAGE2D)
{ }


_oclImage3D::_oclImage3D(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const oglu::TextureDataFormat dformat)
    : _oclImage(ctx, flag, width, height, depth, dformat, CL_MEM_OBJECT_IMAGE3D)
{ }


_oclGLImage2D::_oclGLImage2D(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex)
    : _oclImage2D(ctx, flag, tex->GetSize().first, tex->GetSize().second, 1, 
        oglu::TexFormatUtil::ConvertDtypeFrom(tex->GetInnerFormat()), CreateMemFromGLTex(ctx, (cl_mem_flags)flag, tex))
{ }


_oclGLImage3D::_oclGLImage3D(const oclContext& ctx, const MemFlag flag, const oglu::oglTex3D& tex)
    : _oclImage3D(ctx, flag, std::get<0>(tex->GetSize()), std::get<1>(tex->GetSize()), std::get<2>(tex->GetSize()),
        oglu::TexFormatUtil::ConvertDtypeFrom(tex->GetInnerFormat()), CreateMemFromGLTex(ctx, (cl_mem_flags)flag, tex))
{ }


}
