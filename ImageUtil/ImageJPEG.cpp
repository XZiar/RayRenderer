#include "ImageUtilPch.h"
#include "ImageJPEG.h"

#include "libjpeg-turbo/jpeglib.h"


#pragma message("Compiling ImageJPEG with libjpeg-turbo[" STRINGIZE(LIBJPEG_TURBO_VERSION) "]")
namespace xziar::img::libjpeg
{
using std::byte;
using std::string;
using std::wstring;
using std::u16string;
using common::AlignedBuffer;
using common::SimpleTimer;
using common::io::RandomInputStream;
using common::io::RandomOutputStream;


struct JpegReader::StreamBlock
{
    RandomInputStream& Stream;
    common::AlignedBuffer Buffer;
    const size_t BufferSize;
    size_t LastRegionSize = 0;
    StreamBlock(RandomInputStream& stream, const size_t bufSize) noexcept : Stream(stream), BufferSize(bufSize) {}

    [[nodiscard]] common::span<const std::byte> GetMemRegion()
    {
        Expects(LastRegionSize == 0u);
        const auto space = Stream.TryGetAvaliableInMemory();
        if (space)
        {
            LastRegionSize = space->size();
            return *space;
        }
        // use buffer
        if (Buffer.GetSize() < BufferSize)
            Buffer = common::AlignedBuffer(BufferSize);
        const auto avaliable = Stream.ReadMany(Buffer.GetSize(), 1, Buffer.GetRawPtr());
        return Buffer.AsSpan().first(avaliable);
    }
    void Skip(size_t count)
    {
        if (LastRegionSize)
        {
            count += LastRegionSize;
            LastRegionSize = 0;
        }
        const auto ret = Stream.Skip(LastRegionSize);
        Ensures(ret);
    }
};


struct JpegHelper
{
    static void EmptyDecompFunc([[maybe_unused]] j_decompress_ptr cinfo)
    { }
    static void EmptyCompFunc([[maybe_unused]] j_compress_ptr cinfo)
    { }

    forceinline static void SetMemSpan(j_decompress_ptr cinfo, const common::span<const std::byte> span)
    {
        if (span.size() > 0)
        {
            cinfo->src->bytes_in_buffer = span.size();
            cinfo->src->next_input_byte = reinterpret_cast<const uint8_t*>(span.data());
        }
        else
        {
            static constexpr uint8_t EndMark[4] = { 0xff, JPEG_EOI, 0, 0 };
            cinfo->src->bytes_in_buffer = 2;
            cinfo->src->next_input_byte = EndMark;
        }
    }

    static void InitStream(j_decompress_ptr cinfo)
    {
        auto& block = *reinterpret_cast<JpegReader::StreamBlock*>(cinfo->client_data);
        block.Stream.SetPos(0);
        block.LastRegionSize = 0u;
        SetMemSpan(cinfo, block.GetMemRegion());
    }

    static boolean ReadFromStream(j_decompress_ptr cinfo)
    {
        auto& block = *reinterpret_cast<JpegReader::StreamBlock*>(cinfo->client_data);
        block.Skip(0);
        SetMemSpan(cinfo, block.GetMemRegion());
        return 1;
    }

    static void SkipStream(j_decompress_ptr cinfo, long bytes)
    {
        if (bytes < 0)
        {
            ImgLog().Warning(u"LIBJPEG request an negative skip, ignored.\n");
            return;
        }
        if (cinfo->src->bytes_in_buffer >= static_cast<size_t>(bytes))
        {
            cinfo->src->bytes_in_buffer -= bytes;
            cinfo->src->next_input_byte += bytes;
        }
        else
        {
            const auto needSkip = bytes - cinfo->src->bytes_in_buffer;
            auto& block = *reinterpret_cast<JpegReader::StreamBlock*>(cinfo->client_data);
            block.Skip(needSkip);
            SetMemSpan(cinfo, block.GetMemRegion());
        }
    }

    static boolean WriteToStream(j_compress_ptr cinfo)
    {
        auto writer = reinterpret_cast<JpegWriter*>(cinfo->client_data);
        auto& stream = writer->Stream;
        auto& buffer = writer->Buffer;
        if (stream.Write(buffer.GetSize(), buffer.GetRawPtr()))
        {
            cinfo->dest->free_in_buffer = buffer.GetSize();
            cinfo->dest->next_output_byte = buffer.GetRawPtr<uint8_t>();
        }
        return 1;
    }

    static void FlushToStream(j_compress_ptr cinfo)
    {
        auto writer = reinterpret_cast<JpegWriter*>(cinfo->client_data);
        auto& stream = writer->Stream;
        auto& buffer = writer->Buffer;
        const auto writeSize = buffer.GetSize() - cinfo->dest->free_in_buffer;
        if (stream.Write(writeSize, buffer.GetRawPtr()))
        {
            cinfo->dest->free_in_buffer = buffer.GetSize();
            cinfo->dest->next_output_byte = buffer.GetRawPtr<uint8_t>();
        }
    }

    static void OnReport(j_common_ptr cinfo, const int msg_level)
    {
        char message[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message)(cinfo, message);
        if (msg_level == -1)
        {
            ImgLog().Warning(u"LIBJPEG warns {}\n", message);
        }
#ifdef _DEBUG
        else
            ImgLog().Verbose(u"LIBJPEG trace {}\n", message);
#endif
    }

    static void OnError(j_common_ptr cinfo)
    {
        //auto reader = reinterpret_cast<JpegReader*>(cinfo->client_data);
        char jpegLastErrorMsg[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message)(cinfo, jpegLastErrorMsg);
        ImgLog().Error(u"LIBJPEG report an error: {}\n", jpegLastErrorMsg);
        COMMON_THROW(BaseException, u"Libjpeg report an error");
    }
};

JpegReader::JpegReader(RandomInputStream& stream)
{
    auto decompStruct = new jpeg_decompress_struct();
    JpegDecompStruct = decompStruct;
    jpeg_create_decompress(decompStruct);

    stream.SetPos(0);
    if (const auto space = stream.TryGetAvaliableInMemory(); space && space->size() == stream.GetSize() && space->size() <= std::numeric_limits<unsigned long>::max())
    { // all in memory
        ImgLog().Verbose(u"LIBJPEG bypass Stream with mem region.\n");
        jpeg_mem_src(decompStruct, reinterpret_cast<const unsigned char*>(space->data()), static_cast<unsigned long>(space->size()));
    }
    else
    {
        Block = std::make_unique<StreamBlock>(stream, 65536);
        decompStruct->client_data = Block.get();
        auto jpegSource = new jpeg_source_mgr();
        JpegSource = jpegSource;
        decompStruct->src = jpegSource;
        jpegSource->init_source       = JpegHelper::InitStream;
        jpegSource->fill_input_buffer = JpegHelper::ReadFromStream;
        jpegSource->skip_input_data   = JpegHelper::SkipStream;
        jpegSource->resync_to_restart = jpeg_resync_to_restart;
        jpegSource->term_source       = JpegHelper::EmptyDecompFunc;
        jpegSource->bytes_in_buffer   = 0;
    }

    auto jpegErrorHandler = new jpeg_error_mgr();
    JpegErrorHandler  = jpegErrorHandler;
    decompStruct->err = jpegErrorHandler;
    jpeg_std_error(jpegErrorHandler);
    jpegErrorHandler->error_exit   = JpegHelper::OnError;
    jpegErrorHandler->emit_message = JpegHelper::OnReport;
}

JpegReader::~JpegReader() 
{
    if (JpegDecompStruct)
    {
        jpeg_destroy_decompress((j_decompress_ptr)JpegDecompStruct);
        delete (jpeg_decompress_struct*)JpegDecompStruct;
    }
    if (JpegSource)
        delete (jpeg_source_mgr*)JpegSource;
    if (JpegErrorHandler)
        delete (jpeg_error_mgr*)JpegErrorHandler;
}

bool JpegReader::Validate()
{
    auto decompStruct = (j_decompress_ptr)JpegDecompStruct;
    try
    {
        jpeg_read_header(decompStruct, true);
    }
    catch (const BaseException& be)
    {
        ImgLog().Warning(u"libjpeg-turbo validate failed {}\n", be.Message());
        return false;
    }
    return true;
}

Image JpegReader::Read(ImgDType dataType)
{
    auto decompStruct = (j_decompress_ptr)JpegDecompStruct;
    Image image(dataType);
    if (!dataType.Is(ImgDType::DataTypes::Uint8))
        return image;
    bool needPadAlpha = false;
    const auto ch = dataType.Channel();
    switch (ch)
    {
    case ImgDType::Channels::BGRA:
        decompStruct->out_color_space = JCS_EXT_BGRA; break;
    case ImgDType::Channels::RGBA:
        decompStruct->out_color_space = JCS_EXT_RGBA; break;
    case ImgDType::Channels::BGR:
        decompStruct->out_color_space = JCS_EXT_BGR; break;
    case ImgDType::Channels::RGB:
        decompStruct->out_color_space = JCS_EXT_RGB; break;
    case ImgDType::Channels::RA:
        decompStruct->out_color_space = JCS_GRAYSCALE; needPadAlpha = true; break;
    case ImgDType::Channels::R:
        decompStruct->out_color_space = JCS_GRAYSCALE; break;
    default:
        return image;
    }
    Ensures(!needPadAlpha || dataType.HasAlpha());

    jpeg_start_decompress(decompStruct);

    image.SetSize(decompStruct->image_width, decompStruct->image_height);
    auto ptrs = image.GetRowPtrs<uint8_t>(needPadAlpha ? image.GetWidth() : 0); // offset by width when need padding
    while (decompStruct->output_scanline < decompStruct->output_height)
    {
        jpeg_read_scanlines(decompStruct, &ptrs[decompStruct->output_scanline], decompStruct->output_height - decompStruct->output_scanline);
    }
    if (needPadAlpha)
    {
        Ensures(ch == ImgDType::Channels::RA);
        const auto& cvter = ColorConvertor::Get();
        for (uint32_t row = 0; row < image.GetHeight(); ++row)
            cvter.GrayToGrayA(image.GetRawPtr<uint16_t>(row), ptrs[row], image.GetWidth());
    }

    jpeg_finish_decompress(decompStruct);
    return image;
}


JpegWriter::JpegWriter(RandomOutputStream& stream) : Stream(stream), Buffer(65536)
{
    auto compStruct = new jpeg_compress_struct();
    JpegCompStruct = compStruct;
    compStruct->client_data = this;
    jpeg_create_compress(compStruct);

    auto jpegDest = new jpeg_destination_mgr();
    JpegDest = jpegDest;
    compStruct->dest = jpegDest;
    jpegDest->init_destination    = JpegHelper::EmptyCompFunc;
    jpegDest->empty_output_buffer = JpegHelper::WriteToStream;
    jpegDest->term_destination    = JpegHelper::FlushToStream;
    jpegDest->free_in_buffer      = Buffer.GetSize();
    jpegDest->next_output_byte    = Buffer.GetRawPtr<uint8_t>();

    auto jpegErrorHandler = new jpeg_error_mgr();
    JpegErrorHandler = jpegErrorHandler;
    compStruct->err  = jpegErrorHandler;
    jpeg_std_error(jpegErrorHandler);
    jpegErrorHandler->error_exit   = JpegHelper::OnError;
    jpegErrorHandler->emit_message = JpegHelper::OnReport;
}

inline JpegWriter::~JpegWriter() 
{
    if (JpegCompStruct)
    {
        jpeg_destroy_compress((j_compress_ptr)JpegCompStruct);
        delete (jpeg_compress_struct*)JpegCompStruct;
    }
    if (JpegDest)
        delete (jpeg_destination_mgr*)JpegDest;
    if (JpegErrorHandler)
        delete (jpeg_error_mgr*)JpegErrorHandler;
}

void JpegWriter::Write(const Image& image, const uint8_t quality)
{
    if (image.GetWidth() > JPEG_MAX_DIMENSION || image.GetHeight() > JPEG_MAX_DIMENSION)
        return;
    const auto dstDType = image.GetDataType();
    if (!dstDType.Is(ImgDType::DataTypes::Uint8))
        return;
    if (dstDType.ChannelCount() < 3)
        return;

    auto compStruct = (j_compress_ptr)JpegCompStruct;
    switch (dstDType.Channel())
    {
    case ImgDType::Channels::BGR:
        compStruct->in_color_space = JCS_EXT_BGR; break;
    case ImgDType::Channels::BGRA:
        compStruct->in_color_space = JCS_EXT_BGRA; break;
    case ImgDType::Channels::RGB:
        compStruct->in_color_space = JCS_RGB; break;
    case ImgDType::Channels::RGBA:
        compStruct->in_color_space = JCS_EXT_RGBA; break;
    default:
        return;
    }
    compStruct->image_width = image.GetWidth();
    compStruct->image_height = image.GetHeight();
    compStruct->input_components = image.GetElementSize();
    jpeg_set_defaults(compStruct);
    jpeg_set_quality(compStruct, quality, TRUE);

    jpeg_start_compress(compStruct, TRUE);
    auto ptrs = image.GetRowPtrs();
    jpeg_write_scanlines(compStruct, (uint8_t**)ptrs.data(), image.GetHeight());
    jpeg_finish_compress(compStruct);
}
static auto DUMMY = RegistImageSupport<JpegSupport>();

}