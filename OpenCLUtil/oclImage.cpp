#include "oclRely.h"
#include "oclImage.h"
#include "oclException.h"
#include "oclUtil.h"


namespace oclu::detail
{
using xziar::img::Image;

void CL_CALLBACK OnMemDestroyed(cl_mem memobj, void *user_data)
{
    const auto& buf = *reinterpret_cast<_oclBuffer*>(user_data);
    oclLog().debug(u"oclImage {:p} with size {}, being destroyed.\n", (void*)memobj, buf.Size);
    //async callback, should not access cl-func since buffer may be released at any time.
    //size_t size = 0;
    //clGetMemObjectInfo(memobj, CL_MEM_SIZE, sizeof(size), &size, nullptr);
}

static cl_image_format ParseImageFormat(const oglu::TextureDataFormat dformat)
{
    using oglu::TextureDataFormat;
    cl_image_format format;
    if (HAS_FIELD(dformat, TextureDataFormat::NORMAL_MASK))
    {
        switch (dformat & TextureDataFormat::TYPE_MASK)
        {
        case TextureDataFormat::U8_TYPE:     format.image_channel_data_type = CL_UNORM_INT8; break;
        case TextureDataFormat::I8_TYPE:     format.image_channel_data_type = CL_SNORM_INT8; break;
        case TextureDataFormat::U16_TYPE:    format.image_channel_data_type = CL_UNORM_INT16; break;
        case TextureDataFormat::I16_TYPE:    format.image_channel_data_type = CL_SNORM_INT16; break;
        default: COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, L"unsupported format");
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
        }
    }
    switch (dformat & TextureDataFormat::RAW_FORMAT_MASK)
    {
    case TextureDataFormat::R_FORMAT:        format.image_channel_order = CL_R; break;
    case TextureDataFormat::RG_FORMAT:       format.image_channel_order = CL_RG; break;
    case TextureDataFormat::RGB_FORMAT:      format.image_channel_order = CL_RGB; break;
    case TextureDataFormat::RGBA_FORMAT:     format.image_channel_order = CL_RGBA; break;
    case TextureDataFormat::BGRA_FORMAT:     format.image_channel_order = CL_BGRA; break;
    default: COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, L"unsupported format");
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
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot create image", errcode));
    return id;
}
_oclImage::_oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const oglu::TextureDataFormat dformat)
    : Context(ctx), Width(width), Height(height), Flags(flag), Format(dformat),
    memID(CreateMem(Context->context, (cl_mem_flags)Flags, ParseImageDesc(Width, Height), Format))
{}

_oclImage::_oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const oglu::TextureDataFormat dformat, const cl_mem id)
    :Context(ctx), Width(width), Height(height), Flags(flag), Format(dformat), memID(id)
{
    clSetMemObjectDestructorCallback(memID, &OnMemDestroyed, this);
}
_oclImage::_oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const xziar::img::ImageDataType dtype, const bool isNormalized)
    : Context(ctx), Width(width), Height(height), Flags(flag), Format(oglu::TexFormatUtil::ConvertFormat(dtype, isNormalized)),
    memID(CreateMem(Context->context, (cl_mem_flags)Flags, ParseImageDesc(width, height), Format))
{
    clSetMemObjectDestructorCallback(memID, &OnMemDestroyed, this);
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


oclPromise _oclImage::Read(const oclCmdQue que, Image& image, const bool shouldBlock) const
{
    image.SetSize(Width, Height);
    constexpr size_t origin[3] = { 0,0,0 }; 
    const size_t region[3] = { Width,Height,1 }; 
    cl_event e;
    const auto ret = clEnqueueReadImage(que->cmdque, memID, shouldBlock ? CL_TRUE : CL_FALSE, origin, region, 0, 0, image.GetRawPtr(), 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot read clImage", ret));
    if (shouldBlock)
        return {};
    else
        return std::make_shared<detail::oclPromise_>(detail::oclPromise_(e));
}

oclPromise _oclImage::Write(const oclCmdQue que, const Image& image, const bool shouldBlock) const
{
    if (image.Width != Width || image.Height != Height)
        COMMON_THROW(BaseException, L"write size unmatch");
        
    constexpr size_t origin[3] = { 0,0,0 }; 
    const size_t region[3] = { Width,Height,1 }; 
    cl_event e;
    const auto ret = clEnqueueWriteImage(que->cmdque, memID, shouldBlock ? CL_TRUE : CL_FALSE, origin, region, 0, 0, image.GetRawPtr(), 0, nullptr, &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot write clImage", ret));
    if (shouldBlock)
        return {};
    else
        return std::make_shared<detail::oclPromise_>(detail::oclPromise_(e));
}



static cl_mem CreateMemFromBuf(const cl_context ctx, const cl_mem_flags flag, const GLuint buf)
{
    cl_int errcode;
    auto id = clCreateFromGLBuffer(ctx, flag, buf, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot create buffer from glBuffer", errcode));
    return id;
}

static cl_mem CreateMemFromTex(const cl_context ctx, const cl_mem_flags flag, const cl_GLenum texType, GLuint tex)
{
    cl_int errcode;
    auto id = clCreateFromGLTexture(ctx, flag, texType, 0, tex, &errcode);
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errString(L"cannot create buffer from glTexture", errcode));
    return id;
}

_oclGLImage::_oclGLImage(const oclContext& ctx, const MemFlag flag, const oglu::TextureDataFormat dformat, const oglu::oglBuffer buf)
    : _oclImage(ctx, flag, UINT32_MAX, UINT32_MAX, dformat, CreateMemFromBuf(ctx->context, (cl_mem_flags)Flags, buf->bufferID))
{
}

_oclGLImage::_oclGLImage(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D tex)
    : _oclImage(ctx, flag, UINT32_MAX, UINT32_MAX, oglu::TexFormatUtil::DecideFormat(tex->GetInnerFormat()), CreateMemFromTex(ctx->context, (cl_mem_flags)Flags, (cl_GLenum)tex->Type, tex->textureID))
{
}

_oclGLImage::~_oclGLImage()
{
}


}
