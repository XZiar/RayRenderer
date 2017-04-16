#include "uchardetlib.h"
#include "uchardet.h"

namespace uchdet
{
using namespace common;


Charset detectEncoding(const std::vector<uint8_t>& str)
{
	return toCharset(getEncoding(str.data(), str.size()));
}

Charset detectEncoding(const std::string& str)
{
	return toCharset(getEncoding(str.c_str(), str.length()));
}

Charset detectEncoding(const std::wstring& str)
{
	return toCharset(getEncoding(str.c_str(), str.length()));
}

std::string getEncoding(const void *data, const size_t len)
{
	auto tool = uchardet_new();
	if (uchardet_handle_data(tool, (const char*)data, len))
		return "error";
	uchardet_data_end(tool);
	std::string chset(uchardet_get_charset(tool));

	uchardet_delete(tool);
	return chset;
}


}