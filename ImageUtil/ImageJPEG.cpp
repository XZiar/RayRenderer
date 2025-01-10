#include "ImageUtilPch.h"
#include "ImageJPEG.h"

#include "libjpeg-turbo/src/jpeglib.h"


#pragma message("Compiling ImageJPEG with libjpeg-turbo[" STRINGIZE(LIBJPEG_TURBO_VERSION) "]")
namespace xziar::img::libjpeg
{
using std::byte;
using std::string;
using std::wstring;
using std::u16string;
using common::AlignedBuffer;
using common::io::RandomInputStream;
using common::io::RandomOutputStream;


struct JpegErrorMan
{
    jpeg_error_mgr ErrorManager;
    JpegErrorMan()
    {
        memset(&ErrorManager, 0, sizeof(ErrorManager));
        jpeg_std_error(&ErrorManager);
        ErrorManager.error_exit = OnError;
        ErrorManager.emit_message = OnReport;
    }

    static void OnReport(j_common_ptr cinfo, const int msg_level)
    {
        const auto isWarning = msg_level == -1;
#if !CM_DEBUG
        if (!isWarning) return;
#endif
        char message[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message)(cinfo, message);
        ImgLog().Log(isWarning ? common::mlog::LogLevel::Warning : common::mlog::LogLevel::Debug, FmtString(u"LIBJPEG {} {}\n"), isWarning ? u"warns"sv : u"trace"sv, message);
    }
    static void OnError(j_common_ptr cinfo)
    {
        char jpegLastErrorMsg[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message)(cinfo, jpegLastErrorMsg);
        std::string_view msg(jpegLastErrorMsg);
        ImgLog().Error(u"LIBJPEG report an error: {}\n", msg);
        COMMON_THROWEX(BaseException, u"Libjpeg report an error").Attach("detail", std::string(msg));
    }
};


class JpegReader final : public ImgReader, public JpegErrorMan
{
private:
    jpeg_decompress_struct Decompress;
    jpeg_source_mgr SrcManager;
    RandomInputStream& Stream;
    common::AlignedBuffer Buffer;
    const size_t BufferSize;
    size_t LastRegionSize = 0;

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
    void SetMemSpan()
    {
        Expects(LastRegionSize == 0u);
        auto space = Stream.TryGetAvaliableInMemory();
        if (space)
        {
            LastRegionSize = space->size();
        }
        else // use buffer
        {
            if (Buffer.GetSize() < BufferSize)
                Buffer = common::AlignedBuffer(BufferSize);
            const auto avaliable = Stream.ReadMany(Buffer.GetSize(), 1, Buffer.GetRawPtr());
            space = Buffer.AsSpan().first(avaliable);
        }
        if (space->size() > 0)
        {
            Decompress.src->bytes_in_buffer = space->size();
            Decompress.src->next_input_byte = reinterpret_cast<const uint8_t*>(space->data());
        }
        else
        {
            static constexpr uint8_t EndMark[4] = { 0xff, JPEG_EOI, 0, 0 };
            Decompress.src->bytes_in_buffer = 2;
            Decompress.src->next_input_byte = EndMark;
        }
    }

    static void EmptyDecompFunc([[maybe_unused]] j_decompress_ptr cinfo) {}
    static void InitStream(j_decompress_ptr cinfo)
    {
        auto& self = *reinterpret_cast<JpegReader*>(cinfo->client_data);
        self.Stream.SetPos(0);
        self.LastRegionSize = 0u;
        self.SetMemSpan();
    }
    static boolean ReadFromStream(j_decompress_ptr cinfo)
    {
        auto& self = *reinterpret_cast<JpegReader*>(cinfo->client_data);
        self.Skip(0);
        self.SetMemSpan();
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
            auto& self = *reinterpret_cast<JpegReader*>(cinfo->client_data);
            self.Skip(needSkip);
            self.SetMemSpan();
        }
    }
public:
    JpegReader(common::io::RandomInputStream& stream) : Stream(stream), BufferSize(65536)
    {
        memset(&Decompress, 0, sizeof(Decompress));
        memset(&SrcManager, 0, sizeof(SrcManager));
        Decompress.err = &ErrorManager;
        jpeg_create_decompress(&Decompress);

        stream.SetPos(0);
        if (const auto space = stream.TryGetAvaliableInMemory(); space && space->size() == stream.GetSize() && space->size() <= std::numeric_limits<unsigned long>::max())
        { // all in memory
            ImgLog().Debug(u"LIBJPEG bypass Stream with mem region.\n");
            jpeg_mem_src(&Decompress, reinterpret_cast<const unsigned char*>(space->data()), static_cast<unsigned long>(space->size()));
        }
        else
        {
            SrcManager.init_source = InitStream;
            SrcManager.fill_input_buffer = ReadFromStream;
            SrcManager.skip_input_data = SkipStream;
            SrcManager.resync_to_restart = jpeg_resync_to_restart;
            SrcManager.term_source = EmptyDecompFunc;
            SrcManager.bytes_in_buffer = 0;
            Decompress.client_data = this;
            Decompress.src = &SrcManager;
        }
    }
    ~JpegReader() final
    {
        jpeg_destroy_decompress(&Decompress);
    }
    [[nodiscard]] bool Validate() final
    {
        try
        {
            jpeg_read_header(&Decompress, true);
        }
        catch (const BaseException& be)
        {
            ImgLog().Warning(u"libjpeg-turbo validate failed {}\n", be.Message());
            return false;
        }
        return true;
    }
    [[nodiscard]] Image Read(ImgDType dataType) final
    {
        const j_decompress_ptr decompStruct = &Decompress;
        if (dataType)
        {
            if (!dataType.Is(ImgDType::DataTypes::Uint8))
                COMMON_THROWEX(BaseException, u"Cannot read non-int8 datatype");
        }
        else
        {
            switch (decompStruct->jpeg_color_space)
            {
            case JCS_GRAYSCALE:
                dataType = ImageDataType::GRAY; break;
            case JCS_EXT_RGBA:
            case JCS_EXT_ABGR:
                dataType = ImageDataType::RGBA; break;
            case JCS_EXT_BGRA:
            case JCS_EXT_ARGB:
                dataType = ImageDataType::BGRA; break;
            case JCS_EXT_BGR:
            case JCS_EXT_BGRX:
                dataType = ImageDataType::BGR; break;
            default:
                dataType = ImageDataType::RGB; break;
            }
        }
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
            COMMON_THROWEX(BaseException, u"unknown channel type");
        }
        Ensures(!needPadAlpha || dataType.HasAlpha());

        jpeg_start_decompress(decompStruct);

        Image image(dataType);
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
};



class JpegWriter final : public ImgWriter, public JpegErrorMan
{
private:
    jpeg_compress_struct Compress;
    jpeg_destination_mgr DstManager;
    common::io::RandomOutputStream& Stream;
    common::AlignedBuffer Buffer;

    static void EmptyCompFunc([[maybe_unused]] j_compress_ptr cinfo) { }
    static boolean WriteToStream(j_compress_ptr cinfo)
    {
        auto& self = *reinterpret_cast<JpegWriter*>(cinfo->client_data);
        if (self.Stream.Write(self.Buffer.GetSize(), self.Buffer.GetRawPtr()))
        {
            cinfo->dest->free_in_buffer = self.Buffer.GetSize();
            cinfo->dest->next_output_byte = self.Buffer.GetRawPtr<uint8_t>();
        }
        return 1;
    }
    static void FlushToStream(j_compress_ptr cinfo)
    {
        auto& self = *reinterpret_cast<JpegWriter*>(cinfo->client_data);
        const auto writeSize = self.Buffer.GetSize() - cinfo->dest->free_in_buffer;
        if (self.Stream.Write(writeSize, self.Buffer.GetRawPtr()))
        {
            cinfo->dest->free_in_buffer = self.Buffer.GetSize();
            cinfo->dest->next_output_byte = self.Buffer.GetRawPtr<uint8_t>();
        }
    }
public:
    JpegWriter(common::io::RandomOutputStream& stream) : Stream(stream), Buffer(65536)
    {
        memset(&Compress, 0, sizeof(Compress));
        memset(&DstManager, 0, sizeof(DstManager));
        Compress.err = &ErrorManager;
        jpeg_create_compress(&Compress);
        
        {
            DstManager.init_destination = EmptyCompFunc;
            DstManager.empty_output_buffer = WriteToStream;
            DstManager.term_destination = FlushToStream;
            DstManager.free_in_buffer = Buffer.GetSize();
            DstManager.next_output_byte = Buffer.GetRawPtr<uint8_t>();
            Compress.client_data = this;
            Compress.dest = &DstManager;
        }
    }
    ~JpegWriter() final
    {
        jpeg_destroy_compress(&Compress);
    }
    void Write(ImageView image, const uint8_t quality) final
    {
        if (image.GetWidth() > JPEG_MAX_DIMENSION || image.GetHeight() > JPEG_MAX_DIMENSION)
            COMMON_THROWEX(BaseException, u"image shape exceeds JPEG_MAX_DIMENSION");
        const auto dstDType = image.GetDataType();
        if (!dstDType.Is(ImgDType::DataTypes::Uint8))
            COMMON_THROWEX(BaseException, u"only support uint8 image");
        if (dstDType.Is(ImgDType::DataTypes::Uint8))
            COMMON_THROWEX(BaseException, u"does not support gray-alpha image");

        switch (dstDType.Channel())
        {
        case ImgDType::Channels::BGR:
            Compress.in_color_space = JCS_EXT_BGR; break;
        case ImgDType::Channels::BGRA:
            Compress.in_color_space = JCS_EXT_BGRA; break;
        case ImgDType::Channels::RGB:
            Compress.in_color_space = JCS_RGB; break;
        case ImgDType::Channels::RGBA:
            Compress.in_color_space = JCS_EXT_RGBA; break;
        case ImgDType::Channels::R:
            Compress.in_color_space = JCS_GRAYSCALE; break;
        default:
            Ensures(false);
            return;
        }
        Compress.image_width = image.GetWidth();
        Compress.image_height = image.GetHeight();
        Compress.input_components = dstDType.ChannelCount();
        jpeg_set_defaults(&Compress);
        jpeg_set_quality(&Compress, quality, TRUE);

        jpeg_start_compress(&Compress, TRUE);
        auto ptrs = image.GetRowPtrs();
        jpeg_write_scanlines(&Compress, (uint8_t**)ptrs.data(), image.GetHeight());
        jpeg_finish_compress(&Compress);
    }
};


std::unique_ptr<ImgReader> JpegSupport::GetReader(common::io::RandomInputStream& stream, std::u16string_view) const
{
    return std::make_unique<JpegReader>(stream);
}
std::unique_ptr<ImgWriter> JpegSupport::GetWriter(common::io::RandomOutputStream& stream, std::u16string_view) const
{
    return std::make_unique<JpegWriter>(stream);
}
uint8_t JpegSupport::MatchExtension(std::u16string_view ext, ImgDType type, const bool isRead) const
{
    if (ext != u"JPEG" && ext != u"JPG")
        return 0;
    if (!type.Is(ImgDType::DataTypes::Uint8))
        return 0;
    if (!isRead && type.Is(ImgDType::Channels::RA))
        return 0;
    return 240;
}

static auto DUMMY = RegistImageSupport<JpegSupport>();

}