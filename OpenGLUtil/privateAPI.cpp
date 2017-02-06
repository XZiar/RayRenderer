#include "privateAPI.h"

namespace oglu
{


TextureManager::TextureManager()
{
	GLint maxs;
	curpos = 1;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxs);
	maxsize = static_cast<uint8_t>(maxs > 255 ? 255 : maxs);
	texmap.resize(maxsize);
}

uint8_t TextureManager::allocPos(const _oglProgram& prog, const oglTexture& tex)
{
	const auto tid = tex->tID;
	const auto ret = mapcache.find(tid);
	if (ret != mapcache.end())
		return ret->second;

	const auto bakpos = curpos;
	do //choose unused slot first
	{
		if (curpos >= maxsize)
			curpos = 1;
		if (texmap[curpos].tid == UINT32_MAX)
		{
			texmap[curpos] = TexItem(tid, 0);
			mapcache.insert_or_assign(tid, curpos);
			tex->bind(curpos);
			return curpos;
		}
		++curpos;
	} while (curpos != bakpos);

	const uint8_t ptexcnt = static_cast<uint8_t>(prog.texs.size());
	while (true)//remove other program's texture
	{
		if (++curpos >= maxsize)
			curpos = 1;
		const GLuint oldtid = texmap[curpos].tid;
		bool hasTex = false;
		for (uint8_t a = 0; a < ptexcnt; ++a)
		{
			if (prog.texs[a].tid == oldtid)
			{
				hasTex = true;
				break;
			}
		}
		if (!hasTex)//this slot is not used by this prog
		{
			mapcache.erase(oldtid);
			texmap[curpos] = TexItem(tid, 0);
			mapcache.insert_or_assign(tid, curpos);
			tex->bind(curpos);
			return curpos;
		}
	}
}

uint8_t TextureManager::bindTexture(const _oglProgram& prog, const oglTexture& tex)
{
	uint32_t retpos = allocPos(prog, tex);
	texmap[retpos].count++;
	return retpos;
}

bool TextureManager::unbindTexture(const _oglProgram& prog, const oglTexture& tex)
{
	const auto tid = tex->tID;
	const auto ret = mapcache.find(tid);
	if (ret == mapcache.end())
		return false;
	auto& obj = texmap[ret->second];
	if (--obj.count == 0)
	{
		curpos = ret->second;//nexttime just use this empty slot
		obj.tid = UINT32_MAX;
		mapcache.erase(tid);
	}
	return true;
}

}