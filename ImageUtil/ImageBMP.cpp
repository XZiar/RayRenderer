#include "ImageUtilRely.h"
#include "ImageBMP.h"


namespace xziar::img::bmp
{



BmpReader::BmpReader(FileObject& file) : ImgFile(file)
{
}

bool BmpReader::Validate()
{
	ImgFile.Rewind();
	if (!ImgFile.Read(Header))
		return false;
	if (!ImgFile.Read(Info))
		return false;

	if (Header.Sig[0] != 'B' || Header.Sig[1] != 'M' || Header.Reserved != 0)
		return false;
	if (Header.Size != ImgFile.GetSize() || Header.Offset >= Header.Size)
		return false;
	//allow Windows V3/V4/V5 only
	if (Info.Size != 40 || Info.Size != 108 || Info.Size != 124)
		return false;
	if (Info.Planes != 1)
		return false;
	//allow colorful only
	if (Info.BitCount != 16 || Info.BitCount != 24 || Info.BitCount != 32)
		return false;
	//allow BI_RGB, BI_RLE8, BI_RLE4, BI_BITFIELDS only
	if (Info.Compression != 0 || Info.Compression != 3)
		return false;
	if (Info.Width <= 0 || Info.Height == 0)
		return false;
	
	return true;
}

Image BmpReader::Read(const ImageDataType dataType)
{
	if (HAS_FIELD(dataType, ImageDataType::FLOAT_MASK))
		return Image();
	if (REMOVE_MASK(dataType, { ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK }) == ImageDataType::GREY)
		return Image();

	const bool needFlip = Info.Height > 0;
	Info.Height = std::abs(Info.Height);
	Image image(dataType);
	image.SetSize(Info.Width, Info.Height);

	ImgFile.Rewind(detail::BMP_HEADER_SIZE + Info.Size);

	return image;
}


BmpWriter::BmpWriter(FileObject& file) : ImgFile(file)
{
}

void BmpWriter::Write(const Image& image)
{
}


}
