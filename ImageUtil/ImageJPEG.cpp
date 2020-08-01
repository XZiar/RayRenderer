#include "ImageUtilPch.h"
#include "ImageJPEG.h"

#include "libjpeg-turbo/jpeglib.h"


#pragma message("Compiling ImageJPEG with libjpeg-turbo[" STRINGIZE(LIBJPEG_TURBO_VERSION) "]")
namespace xziar::img::jpeg
{
using std::byte;
using std::string;
using std::wstring;
using std::u16string;
using common::AlignedBuffer;
using common::SimpleTimer;
using common::io::RandomInputStream;
using common::io::RandomOutputStream;


class StreamReader : public NonCopyable, public NonMovable
{
public:
    using MemSpan = common::span<const std::byte>;
    virtual ~StreamReader() {}
    [[nodiscard]] virtual MemSpan Rewind() = 0;
    [[nodiscard]] virtual MemSpan ReadFromStream() = 0;
    [[nodiscard]] virtual MemSpan SkipStream(size_t len) = 0;
};

class BufferedStreamReader : public StreamReader
{
private:
    common::io::BufferedRandomInputStream& Stream;
public:
    BufferedStreamReader(common::io::BufferedRandomInputStream& stream) : Stream(stream) {}
    virtual ~BufferedStreamReader() override {}
    [[nodiscard]] virtual MemSpan Rewind() override
    {
        Stream.SetPos(0);
        return Stream.ExposeAvaliable();
    }
    [[nodiscard]] virtual MemSpan ReadFromStream() override
    {
        Stream.LoadNext();
        return Stream.ExposeAvaliable();
    }
    [[nodiscard]] virtual MemSpan SkipStream(size_t len) override
    {
        const auto totalLen = Stream.ExposeAvaliable().size() + len;
        Stream.Skip(totalLen);
        return Stream.ExposeAvaliable();
    }
};

class BasicStreamReader : public StreamReader
{
private:
    common::io::RandomInputStream& Stream;
    common::AlignedBuffer Buffer;
public:
    BasicStreamReader(common::io::RandomInputStream& stream, const size_t bufSize) : Stream(stream), Buffer(bufSize) {}
    virtual ~BasicStreamReader() override {}
    [[nodiscard]] virtual MemSpan Rewind() override
    {
        Stream.SetPos(0);
        return ReadFromStream();
    }
    [[nodiscard]] virtual MemSpan ReadFromStream() override
    {
        const auto avaliable = Stream.ReadMany(Buffer.GetSize(), 1, Buffer.GetRawPtr());
        return Buffer.AsSpan().first(avaliable);
    }
    [[nodiscard]] virtual MemSpan SkipStream(size_t len) override
    {
        Stream.Skip(len);
        return ReadFromStream();
    }
};

[[nodiscard]] static std::unique_ptr<StreamReader> GetReader(RandomInputStream& stream)
{
    if (auto bufStream = dynamic_cast<common::io::BufferedRandomInputStream*>(&stream))
        return std::make_unique<BufferedStreamReader>(*bufStream);
    else
        return std::make_unique<BasicStreamReader>(stream, 65536);
}


struct JpegHelper
{
    static void EmptyDecompFunc([[maybe_unused]] j_decompress_ptr cinfo)
    { }
    static void EmptyCompFunc([[maybe_unused]] j_compress_ptr cinfo)
    { }

    forceinline static void SetMemSpan(j_decompress_ptr cinfo, const StreamReader::MemSpan span)
    {
        if (span.size() > 0)
        {
            cinfo->src->bytes_in_buffer = span.size();
            cinfo->src->next_input_byte = reinterpret_cast<const uint8_t*>(span.data());
        }
        else
        {
            constexpr uint8_t EndMark[4] = { 0xff, JPEG_EOI, 0, 0 };
            cinfo->src->bytes_in_buffer = 2;
            cinfo->src->next_input_byte = EndMark;
        }
    }

    static void InitStream(j_decompress_ptr cinfo)
    {
        auto& reader = *reinterpret_cast<StreamReader*>(cinfo->client_data);
        SetMemSpan(cinfo, reader.Rewind());
    }

    static boolean ReadFromStream(j_decompress_ptr cinfo)
    {
        auto& reader = *reinterpret_cast<StreamReader*>(cinfo->client_data);
        SetMemSpan(cinfo, reader.ReadFromStream());
        return 1;
    }

    static void SkipStream(j_decompress_ptr cinfo, long bytes)
    {
        if (bytes < 0)
        {
            ImgLog().warning(u"LIBJPEG request an negative skip, ignored.\n");
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
            auto& reader = *reinterpret_cast<StreamReader*>(cinfo->client_data);
            SetMemSpan(cinfo, reader.SkipStream(needSkip));
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
            ImgLog().warning(u"LIBJPEG warns {}\n", message);
        }
#ifdef _DEBUG
        else
            ImgLog().verbose(u"LIBJPEG trace {}\n", message);
#endif
    }

    static void OnError(j_common_ptr cinfo)
    {
        //auto reader = reinterpret_cast<JpegReader*>(cinfo->client_data);
        char jpegLastErrorMsg[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message)(cinfo, jpegLastErrorMsg);
        ImgLog().error(u"LIBJPEG report an error: {}\n", jpegLastErrorMsg);
        COMMON_THROW(BaseException, u"Libjpeg report an error");
    }
};

JpegReader::JpegReader(RandomInputStream& stream) : Stream(stream)
{
    auto decompStruct = new jpeg_decompress_struct();
    JpegDecompStruct = decompStruct;
    jpeg_create_decompress(decompStruct);

    if (auto memStream = dynamic_cast<common::io::MemoryInputStream*>(&Stream))
    {
        ImgLog().verbose(u"LIBJPEG faces MemoryStream, bypass it.\n");
        const auto [ptr, size] = memStream->ExposeAvaliable();
        jpeg_mem_src(decompStruct, reinterpret_cast<const unsigned char*>(ptr), static_cast<unsigned long>(size));
    }
    else
    {
        Reader = GetReader(stream);
        decompStruct->client_data = Reader.get();
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
    Stream.SetPos(0);
    auto decompStruct = (j_decompress_ptr)JpegDecompStruct;
    try
    {
        jpeg_read_header(decompStruct, true);
    }
    catch (const BaseException& be)
    {
        ImgLog().warning(u"libjpeg-turbo validate failed {}\n", be.Message());
        return false;
    }
    return true;
}

Image JpegReader::Read(const ImageDataType dataType)
{
    auto decompStruct = (j_decompress_ptr)JpegDecompStruct;
    Image image(dataType);
    if (HAS_FIELD(dataType, ImageDataType::FLOAT_MASK))
        return image;
    const bool needAlpha = HAS_FIELD(dataType, ImageDataType::ALPHA_MASK);
    switch (REMOVE_MASK(dataType, ImageDataType::FLOAT_MASK, ImageDataType::ALPHA_MASK))
    {
    case ImageDataType::BGR:
        decompStruct->out_color_space = JCS_EXT_BGR; break;
    case ImageDataType::RGB:
        decompStruct->out_color_space = JCS_EXT_RGB; break;
    case ImageDataType::GRAY:
        decompStruct->out_color_space = JCS_GRAYSCALE; break;
    default:
        return image;
    }

    jpeg_start_decompress(decompStruct);

    image.SetSize(decompStruct->image_width, decompStruct->image_height);
    auto ptrs = image.GetRowPtrs(needAlpha ? image.GetWidth() : 0);
    while (decompStruct->output_scanline < decompStruct->output_height)
    {
        jpeg_read_scanlines(decompStruct, reinterpret_cast<uint8_t**>(&ptrs[decompStruct->output_scanline]), decompStruct->output_height - decompStruct->output_scanline);
    }
    if (needAlpha)
    {
        for (uint32_t row = 0; row < image.GetHeight(); ++row)
            convert::RGBsToRGBAs(image.GetRawPtr(row), ptrs[row], image.GetWidth());
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
    if (HAS_FIELD(image.GetDataType(), ImageDataType::FLOAT_MASK))
        return;
    auto compStruct = (j_compress_ptr)JpegCompStruct;
    const auto dataType = REMOVE_MASK(image.GetDataType(), ImageDataType::FLOAT_MASK);
    switch (dataType)
    {
    case ImageDataType::BGR:
        compStruct->in_color_space = JCS_EXT_BGR; break;
    case ImageDataType::BGRA:
        compStruct->in_color_space = JCS_EXT_BGRA; break;
    case ImageDataType::RGB:
        compStruct->in_color_space = JCS_RGB; break;
    case ImageDataType::RGBA:
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