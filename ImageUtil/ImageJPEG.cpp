#include "ImageUtilRely.h"
#include "ImageJPEG.h"
#include "libjpeg-turbo/jpeglib.h"


#pragma message("Compiler ImageJPEG with libjpeg-turbo[" STRINGIZE(LIBJPEG_TURBO_VERSION) "]")
namespace xziar::img::jpeg
{


struct JpegHelper
{
	static void EmptyFunc(j_decompress_ptr cinfo)
	{ }

	static uint8_t ReadFromFile(j_decompress_ptr cinfo)
	{
		auto reader = reinterpret_cast<JpegReader*>(cinfo->client_data);
		auto& imgFile = reader->ImgFile;
		auto& buffer = reader->Buffer;
		const auto readSize = std::min(imgFile.LeftSpace(), buffer.GetSize());
		cinfo->src->bytes_in_buffer = imgFile.Read(readSize, buffer.GetRawPtr()) ? readSize : 0;
		cinfo->src->next_input_byte = buffer.GetRawPtr();
		return 1;
	}

	static void SkipFile(j_decompress_ptr cinfo, long bytes)
	{
		auto reader = reinterpret_cast<JpegReader*>(cinfo->client_data);
		auto& imgFile = reader->ImgFile;
		imgFile.Skip(bytes);
	}

	static void OnReport(j_common_ptr cinfo, const int msg_level)
	{
		char message[JMSG_LENGTH_MAX];
		(*cinfo->err->format_message)(cinfo, message);
		const auto msg = to_wstring(message);
		if (msg_level == -1)
			ImgLog().warning(L"LIBJPEG warns {}\n", msg);
		else
			ImgLog().verbose(L"LIBJPEG trace {}\n", msg);
	}

	static void OnError(j_common_ptr cinfo)
	{
		auto reader = reinterpret_cast<JpegReader*>(cinfo->client_data);
		char jpegLastErrorMsg[JMSG_LENGTH_MAX];
		(*cinfo->err->format_message)(cinfo, jpegLastErrorMsg);
		ImgLog().error(L"LIBJPEG report an error: {}\n", to_wstring(jpegLastErrorMsg));
		COMMON_THROW(BaseException, L"Libjpeg report an error");
	}
};

JpegReader::JpegReader(FileObject& file) : ImgFile(file), Buffer(65536)
{
	auto decompStruct = new jpeg_decompress_struct();
	JpegDecompStruct = decompStruct;
	decompStruct->client_data = this;
	jpeg_create_decompress(decompStruct);

	auto jpegSource = new jpeg_source_mgr();
	JpegSource = jpegSource;
	decompStruct->src = jpegSource;
	jpegSource->init_source = JpegHelper::EmptyFunc;
	jpegSource->fill_input_buffer = JpegHelper::ReadFromFile;
	jpegSource->skip_input_data = JpegHelper::SkipFile;
	jpegSource->resync_to_restart = jpeg_resync_to_restart;
	jpegSource->term_source = JpegHelper::EmptyFunc;
	jpegSource->bytes_in_buffer = 0;

	auto jpegErrorHandler = new jpeg_error_mgr();
	JpegErrorHandler = jpegErrorHandler;
	decompStruct->err = jpegErrorHandler;
	jpeg_std_error(jpegErrorHandler);
	jpegErrorHandler->error_exit = JpegHelper::OnError;
	jpegErrorHandler->emit_message = JpegHelper::OnReport;
}

JpegReader::~JpegReader() 
{
	if (JpegDecompStruct)
	{
		jpeg_destroy_decompress((j_decompress_ptr)JpegDecompStruct);
		delete JpegDecompStruct;
	}
	if (JpegSource)
		delete JpegSource;
	if (JpegErrorHandler)
		delete JpegErrorHandler;
}

bool JpegReader::Validate()
{
	ImgFile.Rewind();
	auto decompStruct = (j_decompress_ptr)JpegDecompStruct;
	try
	{
		jpeg_read_header(decompStruct, true);
	}
	catch (BaseException& be)
	{
		ImgLog().warning(L"libjpeg-turbo validate failed {}\n", be.message);
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
	switch (REMOVE_MASK(dataType, { ImageDataType::FLOAT_MASK,ImageDataType::ALPHA_MASK }))
	{
	case ImageDataType::BGR:
		decompStruct->out_color_space = JCS_EXT_BGR; break;
	case ImageDataType::RGB:
		decompStruct->out_color_space = JCS_EXT_RGB; break;
	case ImageDataType::GREY:
		decompStruct->out_color_space = JCS_GRAYSCALE; break;
	}

	jpeg_start_decompress(decompStruct);

	image.SetSize(decompStruct->image_width, decompStruct->image_height);
	auto ptrs = image.GetRowPtrs(needAlpha ? image.Width : 0);
	while (decompStruct->output_scanline < decompStruct->output_height)
	{
		jpeg_read_scanlines(decompStruct, &ptrs[decompStruct->output_scanline], decompStruct->output_height - decompStruct->output_scanline);
	}
	if (needAlpha)
	{
		for (uint32_t row = 0; row < image.Height; ++row)
			convert::RGBsToRGBAs(image.GetRawPtr(row), ptrs[row], image.Width);
	}

	jpeg_finish_decompress(decompStruct);
	return image;
}


JpegWriter::JpegWriter(FileObject& file) : ImgFile(file)
{
}

void JpegWriter::Write(const Image& image)
{
}

}