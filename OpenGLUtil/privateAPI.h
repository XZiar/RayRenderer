#pragma once

#include "oglUtil.h"

namespace oglu
{

class TextureManager : public NonCopyable
{
private:
	friend _oglProgram;
	struct TexItem
	{
		GLuint tid;
		uint32_t count;
		TexItem(const GLuint tid_ = UINT32_MAX, uint32_t count_ = 0) :tid(tid_), count(count_) { };
	};
	vector<TexItem> texmap;
	map<GLuint, uint8_t> mapcache;
	uint8_t curpos, maxsize;
	uint8_t allocPos(const _oglProgram& prog, const oglTexture& tex);
public:
	TextureManager();
	uint8_t bindTexture(const _oglProgram& prog, const oglTexture& tex);
	bool unbindTexture(const _oglProgram& prog, const oglTexture& tex);
};

}