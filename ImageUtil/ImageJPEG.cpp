#include "ImageUtilRely.h"
#include "ImageJPEG.h"
#include "libjpeg-turbo/jpeglib.h"


#pragma message("Compiler ImageJPEG with libjpeg-turbo[" STRINGIZE(LIBJPEG_TURBO_VERSION) "]")
namespace xziar::img::jpeg
{


struct JpegHelper
{
	static void EmptyDecompFunc(j_decompress_ptr cinfo)
	{ }
	static void EmptyCompFunc(j_compress_ptr cinfo)
	{ }

	static uint8_t ReadFromFile(j_decompress_ptr cinfo)
	{
		auto reader = reinterpret_cast<JpegReader*>(cinfo->client_data);
		auto& imgFile = reader->ImgFile;
		auto& buffer = reader->Buffer;
		const auto readSize = std::min(imgFile.LeftSpace(), buffer.GetSize());
		if (imgFile.Read(readSize, buffer.GetRawPtr()))
		{
			cinfo->src->bytes_in_buffer = readSize;
			cinfo->src->next_input_byte = buffer.GetRawPtr<uint8_t>();
		}
		return 1;
	}

	static void SkipFile(j_decompress_ptr cinfo, long bytes)
	{
		auto reader = reinterpret_cast<JpegReader*>(cinfo->client_data);
		auto& imgFile = reader->ImgFile;
		imgFile.Skip(bytes);
	}

	static uint8_t WriteToFile(j_compress_ptr cinfo)
	{
		auto writer = reinterpret_cast<JpegWriter*>(cinfo->client_data);
		auto& imgFile = writer->ImgFile;
		auto& buffer = writer->Buffer;
		if (imgFile.Write(buffer.GetSize(), buffer.GetRawPtr()))
		{
			cinfo->dest->free_in_buffer = buffer.GetSize();
			cinfo->dest->next_output_byte = buffer.GetRawPtr<uint8_t>();
		}
		return 1;
	}

	static void FlushToFile(j_compress_ptr cinfo)
	{
		auto writer = reinterpret_cast<JpegWriter*>(cinfo->client_data);
		auto& imgFile = writer->ImgFile;
		auto& buffer = writer->Buffer;
		const auto writeSize = buffer.GetSize() - cinfo->dest->free_in_buffer;
		if (imgFile.Write(writeSize, buffer.GetRawPtr()))
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
		auto reader = reinterpret_cast<JpegReader*>(cinfo->client_data);
		char jpegLastErrorMsg[JMSG_LENGTH_MAX];
		(*cinfo->err->format_message)(cinfo, jpegLastErrorMsg);
		ImgLog().error(u"LIBJPEG report an error: {}\n", jpegLastErrorMsg);
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
	jpegSource->init_source = JpegHelper::EmptyDecompFunc;
	jpegSource->fill_input_buffer = JpegHelper::ReadFromFile;
	jpegSource->skip_input_data = JpegHelper::SkipFile;
	jpegSource->resync_to_restart = jpeg_resync_to_restart;
	jpegSource->term_source = JpegHelper::EmptyDecompFunc;
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
	switch (REMOVE_MASK(dataType, ImageDataType::FLOAT_MASK,ImageDataType::ALPHA_MASK))
	{
	case ImageDataType::BGR:
		decompStruct->out_color_space = JCS_EXT_BGR; break;
	case ImageDataType::RGB:
		decompStruct->out_color_space = JCS_EXT_RGB; break;
	case ImageDataType::GRAY:
		decompStruct->out_color_space = JCS_GRAYSCALE; break;
	}

	jpeg_start_decompress(decompStruct);

	image.SetSize(decompStruct->image_width, decompStruct->image_height);
	auto ptrs = image.GetRowPtrs(needAlpha ? image.Width : 0);
	while (decompStruct->output_scanline < decompStruct->output_height)
	{
		jpeg_read_scanlines(decompStruct, reinterpret_cast<uint8_t**>(&ptrs[decompStruct->output_scanline]), decompStruct->output_height - decompStruct->output_scanline);
	}
	if (needAlpha)
	{
		for (uint32_t row = 0; row < image.Height; ++row)
			convert::RGBsToRGBAs(image.GetRawPtr(row), ptrs[row], image.Width);
	}

	jpeg_finish_decompress(decompStruct);
	return image;
}


JpegWriter::JpegWriter(FileObject& file) : ImgFile(file), Buffer(65536)
{
	auto compStruct = new jpeg_compress_struct();
	JpegCompStruct = compStruct;
	compStruct->client_data = this;
	jpeg_create_compress(compStruct);

	auto jpegDest = new jpeg_destination_mgr();
	JpegDest = jpegDest;
	compStruct->dest = jpegDest;
	jpegDest->init_destination = JpegHelper::EmptyCompFunc;
	jpegDest->empty_output_buffer = JpegHelper::WriteToFile;
	jpegDest->term_destination = JpegHelper::FlushToFile;
	jpegDest->free_in_buffer = Buffer.GetSize();
	jpegDest->next_output_byte = Buffer.GetRawPtr<uint8_t>();

	auto jpegErrorHandler = new jpeg_error_mgr();
	JpegErrorHandler = jpegErrorHandler;
	compStruct->err = jpegErrorHandler;
	jpeg_std_error(jpegErrorHandler);
	jpegErrorHandler->error_exit = JpegHelper::OnError;
	jpegErrorHandler->emit_message = JpegHelper::OnReport;
}

inline JpegWriter::~JpegWriter() 
{
	if (JpegCompStruct)
	{
		jpeg_destroy_compress((j_compress_ptr)JpegCompStruct);
		delete JpegCompStruct;
	}
	if (JpegDest)
		delete JpegDest;
	if (JpegErrorHandler)
		delete JpegErrorHandler;
}

void JpegWriter::Write(const Image& image)
{
	if (image.Width > JPEG_MAX_DIMENSION || image.Height > JPEG_MAX_DIMENSION)
		return;
	if (HAS_FIELD(image.DataType, ImageDataType::FLOAT_MASK))
		return;
	auto compStruct = (j_compress_ptr)JpegCompStruct;
	const auto dataType = REMOVE_MASK(image.DataType, { ImageDataType::FLOAT_MASK });
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
	compStruct->image_width = image.Width;
	compStruct->image_height = image.Height;
	compStruct->input_components = image.ElementSize;
	jpeg_set_defaults(compStruct);
	jpeg_set_quality(compStruct, 90, TRUE);

	jpeg_start_compress(compStruct, TRUE);
	auto ptrs = image.GetRowPtrs();
	jpeg_write_scanlines(compStruct, (uint8_t**)ptrs.data(), image.Height);
	jpeg_finish_compress(compStruct);
}

}