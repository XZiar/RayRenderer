#pragma once

#include "oglRely.h"

namespace oglu
{

class _oglProgram;

//D:derived class type, T:resource type
//GLuint getID(const T& obj) const
//**** get obj's inner id
//void innerBind(const T& obj, const uint8_t pos) const
//**** when an slot is allocatted for obj
//bool hasRes(const _oglProgram& prog, const GLuint id) const
//**** judge whether prog holds resource(if not, this resource will be removed)
template<class D, class T>
class ResDister : public NonCopyable
{
private:
	static uint8_t getSize(const GLenum prop)
	{
		GLint maxs;
		glGetIntegerv(prop, &maxs);
		return static_cast<uint8_t>(maxs > 255 ? 255 : maxs);
	}
	GLuint getID(const T& obj) const
	{
		return ((const D*)this)->getID(obj);
	}
	void innerBind(const T& obj, const uint8_t pos) const
	{
		((const D*)this)->innerBind(obj, pos);
	}
	bool hasRes(const _oglProgram& prog, const GLuint id) const
	{
		return ((const D*)this)->hasRes(prog, id);
	}
protected:
	struct ResItem
	{
		GLuint id;
		uint32_t count;
		ResItem(const GLuint id_ = UINT32_MAX, uint32_t count_ = 0) :id(id_), count(count_) { };
	};
	vector<ResItem> resmap;
	map<GLuint, uint8_t> mapcache;
	uint8_t curpos, maxsize;
	
	ResDister(uint8_t size) : maxsize(size)
	{
		curpos = 1;
		resmap.resize(maxsize);
	}
	ResDister(GLenum prop) :ResDister(getSize(prop)) { }

	uint8_t allocPos(const _oglProgram& prog, const T& obj)
	{
		const auto id = getID(obj);
		const auto ret = mapcache.find(id);
		if (ret != mapcache.end())
			return ret->second;

		const auto bakpos = curpos;
		do //choose unused slot first
		{
			if (curpos >= maxsize)
				curpos = 1;
			if (resmap[curpos].id == UINT32_MAX)
			{
				resmap[curpos] = ResItem(id, 0);
				mapcache.insert_or_assign(id, curpos);
				innerBind(obj, curpos);
				return curpos;
			}
			++curpos;
		} while (curpos != bakpos);

		while (true)//remove other program's texture
		{
			if (++curpos >= maxsize)
				curpos = 1;
			const GLuint oldid = resmap[curpos].id;
			if (!hasRes(prog,oldid))//this slot is not used by this prog
			{
				mapcache.erase(oldid);
				resmap[curpos] = ResItem(id, 0);
				mapcache.insert_or_assign(id, curpos);
				innerBind(obj, curpos);
				return curpos;
			}
		}
	}

	uint8_t bind(const _oglProgram& prog, const T& obj)
	{
		uint32_t retpos = allocPos(prog, obj);
		resmap[retpos].count++;
		return retpos;
	}
	bool unbind(const _oglProgram& prog, const T& obj)
	{
		const auto id = getID(obj);
		const auto ret = mapcache.find(id);
		if (ret == mapcache.end())
			return false;
		auto& item = resmap[ret->second];
		if (--item.count == 0)
		{
			curpos = ret->second;//nexttime just use this empty slot
			item.id = UINT32_MAX;
			mapcache.erase(id);
		}
		return true;
	}
};

}