#include "oclPch.h"
#include "oclImage.h"
#include "oclUtil.h"
#include "oclPromise.h"


namespace oclu
{
using common::BaseException;
using common::PromiseResult;
using xziar::img::Image;
using xziar::img::ImageView;
using xziar::img::TexFormatUtil;
using xziar::img::TextureFormat;

COMMON_EXCEPTION_IMPL(OCLWrongFormatException)
MAKE_ENABLER_IMPL(oclImage2D_)
MAKE_ENABLER_IMPL(oclImage3D_)


TextureFormat ParseCLImageFormat(const cl_image_format& clFormat)
{
    TextureFormat format = TextureFormat::EMPTY;
    switch(clFormat.image_channel_data_type)
    {
    case CL_SIGNED_INT8:        format |= TextureFormat::DTYPE_I8  | TextureFormat::DTYPE_PLAIN_RAW; break;
    case CL_UNSIGNED_INT8:      format |= TextureFormat::DTYPE_U8  | TextureFormat::DTYPE_PLAIN_RAW; break;
    case CL_SIGNED_INT16:       format |= TextureFormat::DTYPE_I16 | TextureFormat::DTYPE_PLAIN_RAW; break;
    case CL_UNSIGNED_INT16:     format |= TextureFormat::DTYPE_U16 | TextureFormat::DTYPE_PLAIN_RAW; break;
    case CL_SIGNED_INT32:       format |= TextureFormat::DTYPE_I32 | TextureFormat::DTYPE_PLAIN_RAW; break;
    case CL_UNSIGNED_INT32:     format |= TextureFormat::DTYPE_U32 | TextureFormat::DTYPE_PLAIN_RAW; break;
    case CL_SNORM_INT8:         format |= TextureFormat::DTYPE_I8; break;
    case CL_UNORM_INT8:         format |= TextureFormat::DTYPE_U8; break;
    case CL_SNORM_INT16:        format |= TextureFormat::DTYPE_I16; break;
    case CL_UNORM_INT16:        format |= TextureFormat::DTYPE_U16; break;
    case CL_HALF_FLOAT:         format |= TextureFormat::DTYPE_HALF; break;
    case CL_FLOAT:              format |= TextureFormat::DTYPE_FLOAT; break;
    case CL_UNORM_SHORT_565:    format |= TextureFormat::COMP_565; break;
    case CL_UNORM_SHORT_555:    format |= TextureFormat::COMP_5551; break;
    case CL_UNORM_INT_101010:   format |= TextureFormat::COMP_10_2; break;
    case CL_UNORM_INT_101010_2: format |= TextureFormat::COMP_10_2; break;
    default:                    return TextureFormat::ERROR;
    }
    switch(clFormat.image_channel_order)
    {
    case CL_A:
    case CL_LUMINANCE:
    case CL_INTENSITY:
    case CL_R:          format |= TextureFormat::CHANNEL_R; break;
    case CL_RA:
    case CL_RG:         format |= TextureFormat::CHANNEL_RG; break;
    case CL_sRGB:
    case CL_RGB:        format |= TextureFormat::CHANNEL_RGB; break;
    case CL_RGBx:
    case CL_sRGBx:
    case CL_sRGBA:
    case CL_RGBA:       format |= TextureFormat::CHANNEL_RGBA; break;
    case CL_sBGRA:
    case CL_BGRA:       format |= TextureFormat::CHANNEL_BGRA; break;
    case CL_ABGR:       format |= TextureFormat::CHANNEL_ABGR; break;
    default:            return TextureFormat::ERROR;
    }
    return format;
}
static cl_image_format ParseTextureFormat(const TextureFormat format)
{
    cl_image_format clFormat;
    const auto category = format & TextureFormat::MASK_DTYPE_CAT;
    const auto dtype = format & TextureFormat::MASK_DTYPE_RAW;
    switch (category)
    {
    case TextureFormat::DTYPE_CAT_COMPRESS:
        COMMON_THROW(OCLWrongFormatException, u"unsupported compressed format", format); // should not enter
    case TextureFormat::DTYPE_CAT_COMP:
        switch (dtype)
        {
        case TextureFormat::COMP_565:    
            clFormat.image_channel_data_type = CL_UNORM_SHORT_565;    clFormat.image_channel_order = CL_RGBx; break;
        case TextureFormat::COMP_5551:   
            clFormat.image_channel_data_type = CL_UNORM_SHORT_555;    clFormat.image_channel_order = CL_RGBx; break;
        case TextureFormat::COMP_10_2:   
            clFormat.image_channel_data_type = CL_UNORM_INT_101010_2; clFormat.image_channel_order = CL_RGBx; break;
        default: 
            COMMON_THROW(OCLWrongFormatException, u"unsupported composed format", format); // should not enter
        }
        break;
    case TextureFormat::DTYPE_CAT_PLAIN:
        if (!TexFormatUtil::IsNormalized(format))
        {
            switch (format & TextureFormat::MASK_PLAIN_RAW)
            {
            case TextureFormat::DTYPE_U8:     clFormat.image_channel_data_type = CL_UNSIGNED_INT8;  break;
            case TextureFormat::DTYPE_I8:     clFormat.image_channel_data_type = CL_SIGNED_INT8;    break;
            case TextureFormat::DTYPE_U16:    clFormat.image_channel_data_type = CL_UNSIGNED_INT16; break;
            case TextureFormat::DTYPE_I16:    clFormat.image_channel_data_type = CL_SIGNED_INT16;   break;
            case TextureFormat::DTYPE_U32:    clFormat.image_channel_data_type = CL_UNSIGNED_INT32; break;
            case TextureFormat::DTYPE_I32:    clFormat.image_channel_data_type = CL_SIGNED_INT32;   break;
            case TextureFormat::DTYPE_HALF:   clFormat.image_channel_data_type = CL_HALF_FLOAT;     break;
            case TextureFormat::DTYPE_FLOAT:  clFormat.image_channel_data_type = CL_FLOAT;          break;
            default: COMMON_THROW(OCLWrongFormatException, u"unsupported integer/float format", format); // should not enter
            }
        }
        else
        {
            switch (format & TextureFormat::MASK_PLAIN_RAW)
            {
            case TextureFormat::DTYPE_U8:     clFormat.image_channel_data_type = CL_UNORM_INT8;  break;
            case TextureFormat::DTYPE_I8:     clFormat.image_channel_data_type = CL_SNORM_INT8;  break;
            case TextureFormat::DTYPE_U16:    clFormat.image_channel_data_type = CL_UNORM_INT16; break;
            case TextureFormat::DTYPE_I16:    clFormat.image_channel_data_type = CL_SNORM_INT16; break;
            default: COMMON_THROW(OCLWrongFormatException, u"unsupported normalized format", format);
            }
        }
        switch (format & TextureFormat::MASK_CHANNEL)
        {
        case TextureFormat::CHANNEL_R:        clFormat.image_channel_order = CL_R;    break;
        case TextureFormat::CHANNEL_RG:       clFormat.image_channel_order = CL_RG;   break;
        case TextureFormat::CHANNEL_RGB:      clFormat.image_channel_order = CL_RGB;  break;
        case TextureFormat::CHANNEL_RGBA:     clFormat.image_channel_order = CL_RGBA; break;
        case TextureFormat::CHANNEL_BGRA:     clFormat.image_channel_order = CL_BGRA; break;
        case TextureFormat::CHANNEL_ABGR:     clFormat.image_channel_order = CL_ABGR; break;
        default: COMMON_THROW(OCLWrongFormatException, u"unsupported channel", format);
        }
        if (clFormat.image_channel_order == CL_RGB)
        {
            if (!common::MatchAny(clFormat.image_channel_data_type, 
                    (cl_channel_type)CL_UNORM_SHORT_565, (cl_channel_type)CL_UNORM_SHORT_555, (cl_channel_type)CL_UNORM_INT_101010))
                COMMON_THROW(OCLWrongFormatException, u"unsupported format for RGB channel image", format);
        }
        if (clFormat.image_channel_order == CL_BGRA)
        {
            if (category != TextureFormat::DTYPE_CAT_PLAIN ||
                !common::MatchAny(format & TextureFormat::MASK_PLAIN_RAW,
                    TextureFormat::DTYPE_U8, TextureFormat::DTYPE_I8))
                COMMON_THROW(OCLWrongFormatException, u"unsupported format for BGRA channel image", format);
        }
        break;
    default:
        COMMON_THROW(OCLWrongFormatException, u"unknown format data category", format);
    }
    return clFormat;
}

static cl_image_desc CreateImageDesc(cl_mem_object_type type, const uint32_t width, const uint32_t height, const uint32_t depth = 1)
{
    cl_image_desc desc = {};
    desc.image_type = type;
    desc.image_width = width;
    desc.image_height = height;
    desc.image_depth = depth;
    desc.image_array_size = 0;
    desc.image_row_pitch = 0;
    desc.image_slice_pitch = 0;
    desc.num_mip_levels = 0;
    desc.num_samples = 0;
    desc.buffer = nullptr;
    return desc;
}

static cl_mem CreateMem(const detail::PlatFuncs* funcs, const uint32_t version, const cl_context ctx, 
    const MemFlag flag, const cl_image_desc& desc, const TextureFormat format, const void* ptr)
{
    cl_int errcode;
    const auto clFormat = ParseTextureFormat(format);
    cl_mem id = nullptr;
    if (version >= 12)
    {
        id = funcs->clCreateImage(ctx, common::enum_cast(flag), &clFormat, &desc, const_cast<void*>(ptr), &errcode);
    }
    else
    {
        switch (desc.image_type)
        {
        case CL_MEM_OBJECT_IMAGE2D:
            id = funcs->clCreateImage2D(ctx, common::enum_cast(flag), &clFormat, desc.image_width, desc.image_height,
                desc.image_row_pitch, const_cast<void*>(ptr), &errcode);
            break;
        case CL_MEM_OBJECT_IMAGE3D:
            id = funcs->clCreateImage3D(ctx, common::enum_cast(flag), &clFormat, desc.image_width, desc.image_height, desc.image_depth,
                desc.image_row_pitch, desc.image_slice_pitch, const_cast<void*>(ptr), &errcode);
            break;
        default:
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"unsupported image type");
        }
    }
    if (errcode != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, errcode, u"cannot create image");
    return id;
}

bool oclImage_::CheckFormatCompatible(TextureFormat format)
{
    try
    {
        ParseTextureFormat(format);
        return true;
    }
    catch (const BaseException&)
    {
        return false;
    }
}

oclImage_::oclImage_(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const TextureFormat format, void* id)
    :oclMem_(ctx, id, flag), Width(width), Height(height), Depth(depth), Format(format)
{ }
oclImage_::oclImage_(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const TextureFormat format, uint32_t type, const void* ptr)
    :oclImage_(ctx, flag, width, height, depth, format, 
        CreateMem(ctx->Funcs, ctx->Version, *ctx->Context, flag, 
            CreateImageDesc(type, width, height, depth), format, ptr))
{ }


oclImage_::~oclImage_()
{ 
    if (Context->ShouldDebugResurce())
        oclLog().debug(u"oclImage {:p} with size [{}x{}x{}], being destroyed.\n", (void*)*MemID, Width, Height, Depth);
}

common::span<std::byte> oclImage_::MapObject(CLHandle<detail::CLCmdQue> que, const MapFlag mapFlag)
{
    constexpr size_t origin[3] = { 0,0,0 };
    const size_t region[3] = { Width,Height,Depth };
    cl_int ret;
    size_t image_row_pitch = 0, image_slice_pitch = 0;
    const auto size = Width * Height * Depth * TexFormatUtil::BitPerPixel(Format) / 8;
    const auto ptr = Funcs->clEnqueueMapImage(*que, *MemID, CL_TRUE, common::enum_cast(mapFlag), 
        origin, region, &image_row_pitch, &image_slice_pitch,
        0, nullptr, nullptr, &ret);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot map clImage");
    oclLog().info(u"Mapped clImage [{}x{}x{}] with row pitch [{}] and slice pitch [{}].\n", Width, Height, Depth, image_row_pitch, image_slice_pitch);
    return common::span<std::byte>(reinterpret_cast<std::byte*>(ptr), size);
}

size_t oclImage_::CalculateSize() const
{
    return Width * Height * Depth * TexFormatUtil::BitPerPixel(Format) / 8;
}

PromiseResult<void> oclImage_::ReadSpan(const common::PromiseStub& pmss, const oclCmdQue& que, common::span<std::byte> buf) const
{
    Expects(CalculateSize() <= size_t(buf.size())); // write size not sufficient

    constexpr size_t origin[3] = { 0,0,0 };
    const size_t region[3] = { Width,Height,Depth };
    DependEvents evts(pmss);
    const auto [evtPtr, evtCnt] = evts.GetWaitList();
    cl_event e;
    const auto ret = Funcs->clEnqueueReadImage(*que->CmdQue, *MemID, CL_FALSE, origin, region, 0, 0, buf.data(), 
        evtCnt, reinterpret_cast<const cl_event*>(evtPtr), &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot read clImage");
    return oclPromise<void>::Create(std::move(evts), e, que);
}

PromiseResult<Image> oclImage_::Read(const common::PromiseStub& pmss, const oclCmdQue& que) const
{
    Image img(xziar::img::TexFormatUtil::ToImageDType(Format, true));
    img.SetSize(Width, Height*Depth);
    constexpr size_t origin[3] = { 0,0,0 };
    const size_t region[3] = { Width,Height,Depth };
    DependEvents evts(pmss);
    const auto [evtPtr, evtCnt] = evts.GetWaitList();
    cl_event e;
    const auto ret = Funcs->clEnqueueReadImage(*que->CmdQue, *MemID, CL_FALSE, origin, region, 0, 0, img.GetRawPtr(), 
        evtCnt, reinterpret_cast<const cl_event*>(evtPtr), &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot read clImage");
    return oclPromise<Image>::Create(std::move(evts), e, que, std::move(img));
}

PromiseResult<common::AlignedBuffer> oclImage_::ReadRaw(const common::PromiseStub& pmss, const oclCmdQue& que) const
{
    common::AlignedBuffer buffer(CalculateSize());
    constexpr size_t origin[3] = { 0,0,0 };
    const size_t region[3] = { Width,Height,Depth };
    DependEvents evts(pmss);
    const auto [evtPtr, evtCnt] = evts.GetWaitList();
    cl_event e;
    const auto ret = Funcs->clEnqueueReadImage(*que->CmdQue, *MemID, CL_FALSE, origin, region, 0, 0, buffer.GetRawPtr(), 
        evtCnt, reinterpret_cast<const cl_event*>(evtPtr), &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot read clImage");
    return oclPromise<common::AlignedBuffer>::Create(std::move(evts), e, que, std::move(buffer));
}

PromiseResult<void> oclImage_::WriteSpan(const common::PromiseStub& pmss, const oclCmdQue& que, common::span<const std::byte> buf) const
{
    Expects(CalculateSize() < size_t(buf.size())); // write size not sufficient

    constexpr size_t origin[3] = { 0,0,0 };
    const size_t region[3] = { Width,Height,Depth };
    DependEvents evts(pmss);
    const auto [evtPtr, evtCnt] = evts.GetWaitList();
    cl_event e;
    const auto ret = Funcs->clEnqueueWriteImage(*que->CmdQue, *MemID, CL_FALSE, origin, region, 0, 0, buf.data(), 
        evtCnt, reinterpret_cast<const cl_event*>(evtPtr), &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot write clImage");
    return oclPromise<void>::Create(std::move(evts), e, que);
}

PromiseResult<void> oclImage_::Write(const common::PromiseStub& pmss, const oclCmdQue& que, const ImageView image) const
{
    Expects(image.GetWidth()    == Width); // write image size mismatch
    Expects(image.GetHeight()   == Height * Depth); // write image size mismatch
    const auto wantFormat = xziar::img::TexFormatUtil::ToImageDType(Format, true);
    Expects(image.GetDataType() == wantFormat); // image datatype mismatch

    constexpr size_t origin[3] = { 0,0,0 };
    const size_t region[3] = { Width,Height,Depth };
    DependEvents evts(pmss);
    const auto [evtPtr, evtCnt] = evts.GetWaitList();
    cl_event e;
    const auto ret = Funcs->clEnqueueWriteImage(*que->CmdQue, *MemID, CL_FALSE, origin, region, 0, 0, image.GetRawPtr(), 
        evtCnt, reinterpret_cast<const cl_event*>(evtPtr), &e);
    if (ret != CL_SUCCESS)
        COMMON_THROW(OCLException, OCLException::CLComponent::Driver, ret, u"cannot write clImage");
    return oclPromise<void>::Create(std::move(evts), e, que);
}

oclImage2D_::oclImage2D_(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const TextureFormat format, const void* ptr)
    : oclImage_(ctx, oclMem_::ProcessMemFlag(*ctx, flag, ptr), width, height, 1, format, CL_MEM_OBJECT_IMAGE2D, ptr)
{ }

oclImg2D oclImage2D_::Create(const oclContext & ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const xziar::img::TextureFormat format, const void* ptr)
{
    return MAKE_ENABLER_SHARED(oclImage2D_, (ctx, flag, width, height, format, ptr));
}


oclImage3D_::oclImage3D_(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const TextureFormat format, const void* ptr)
    : oclImage_(ctx, oclMem_::ProcessMemFlag(*ctx, flag, ptr), width, height, depth, format, CL_MEM_OBJECT_IMAGE3D, ptr)
{ }
oclImg3D oclImage3D_::Create(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::TextureFormat format, const void* ptr)
{
    return MAKE_ENABLER_SHARED(oclImage3D_, (ctx, flag, width, height, depth, format, ptr));
}


}
