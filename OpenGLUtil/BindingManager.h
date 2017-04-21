#pragma once

#include "oglRely.h"
#include "oglTexture.h"
#include "oglBuffer.h"
#include "oglProgram.h"

namespace oglu::detail
{


union NodeBlock
{
	struct
	{
		uint16_t prev, next;
	};
	struct
	{
		uint16_t head, tail;
	};
};

template<class Key>
class LRUPos
{
private:
	vector<std::pair<Key, NodeBlock>> data;
	std::map<Key, uint16_t> lookup;
	NodeBlock used, unused;
	uint16_t usedCnt, unuseCnt;
	void removeNode(uint16_t pos, NodeBlock& obj)
	{
		auto& node = data[pos].second;

		if (pos == obj.head)//is head
			obj.head = node.next;//next become head
		else//not head
			data[node.prev].second.next = node.next;

		if (pos == obj.tail)//is tail
			obj.tail = node.prev;//prev become tail
		else//not tail
			data[node.next].second.prev = node.prev;
	}
	void addNode(uint16_t pos, NodeBlock& obj)//become head
	{
		auto& node = data[pos].second;
		node.prev = UINT16_MAX;
		node.next = obj.head;
		obj.head = pos;
		if (node.next != UINT16_MAX)//has element behind
			data[node.next].second.prev = pos;
		else//only element, change tail
			obj.tail = pos;
	}
	void upNode(uint16_t pos, NodeBlock& obj)
	{
		if (pos == obj.head)//is head
			return;
		removeNode(pos, obj);
		addNode(pos, obj);
	}
	void moveNode(uint16_t pos, NodeBlock& from, NodeBlock& to)
	{
		removeNode(pos, from);
		addNode(pos, to);
	}
public:
	using CacheCallBack = std::function<void(const Key& obj, const uint16_t pos)>;
	CacheCallBack onRemove;
	LRUPos() { }
	LRUPos(const uint16_t size)
	{
		data.resize(size);
		for (uint16_t a = 0; a < size; ++a)
		{
			NodeBlock& node = data[a].second;
			node.prev = static_cast<uint16_t>(a - 1);
			node.next = static_cast<uint16_t>(a + 1);
		}
		data[size - 1].second.next = UINT16_MAX;
		used.head = UINT16_MAX, used.tail = UINT16_MAX;
		unused.head = 0, unused.tail = static_cast<uint16_t>(size - 1);
		usedCnt = 0, unuseCnt = static_cast<uint16_t>(size);
	}
	//touch(move to head) a obj, return if it is in cache
	uint16_t touch(const Key& obj)
	{
		const auto it = lookup.find(obj);
		if (it == lookup.end())
			return UINT16_MAX;
		upNode(it->second, used);
		return it->second;
	}
	//push a obj and return the pos
	//if obj is in cache, it will be moved to head
	//if obj is not in cache, it will be added to the head
	uint16_t push(const Key& obj, bool *isNewAdded = nullptr)
	{
		const auto it = lookup.find(obj);
		if (it == lookup.end())
		{//need to push obj
			uint16_t pos;
			if (unused.head != UINT16_MAX)
			{//has empty node
				pos = unused.head;
				data[pos].first = obj;
				moveNode(pos, unused, used);
				usedCnt++, unuseCnt--;
			}
			else
			{//no empty node
				pos = used.tail;
				if (onRemove != nullptr)
					onRemove(data[pos].first, pos);
				data[pos].first = obj;
				upNode(pos, used);
			}
			if (isNewAdded != nullptr)
				*isNewAdded = true;
			lookup.insert({ obj, pos });
			return pos;
		}
		else
		{//touch it
			if (isNewAdded != nullptr)
				*isNewAdded = false;
			upNode(it->second, used);
			return it->second;
		}
	}
	//pop a obj
	void pop(const Key& obj)
	{
		auto it = lookup.find(obj);
		if (it == lookup.end())
			return;
		if (onRemove != nullptr)
			onRemove(it->first, it->second);
		removeNode(it->second, used);
		lookup.erase(it);
		usedCnt--, unuseCnt++;
	}
};


class _oglProgram;

//D:derived class type, T:resource type
//GLuint getID(const T& obj) const
//**** get obj's inner id
//void innerBind(const GLuint& id, const uint8_t pos) const
//**** when an slot is allocatted for an obj whose ID is id
template<class D, class T, uint8_t offset>
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
	void outterBind(const GLuint prog, const GLuint pos, const uint8_t val) const
	{
		((const D*)this)->outterBind(prog, pos, val);
	}
protected:
	LRUPos<GLuint> cache;
	ResDister(uint8_t size) :cache(size), offset(0) { }
	ResDister(GLenum prop) :cache(uint8_t(getSize(prop) - offset)) { }
public:
	void bindAll(const GLuint prog, const std::map<GLuint, T>& objs, vector<GLint>& poss)
	{
		vector<const std::pair<const GLuint, T>*> rebinds;
		rebinds.reserve(objs.size());
		for (const auto& item : objs)
		{
			GLuint val = 0;
			if (item.second)
			{
				const auto ret = cache.touch(getID(item.second) + offset);
				if (ret == UINT16_MAX)
				{
					rebinds.push_back(&item);
					continue;
				}
				else
					val = static_cast<GLint>(ret);
			}
			if (poss[item.first] != val)
				outterBind(prog, item.first, poss[item.first] == val);
		}
		for (const auto& item : rebinds)
		{
			outterBind(prog, item->first, poss[item->first] = bind(item->second));
		}
	}
	uint8_t bind(const T& obj)
	{
		bool shouldBind = false;
		const uint8_t pos = (uint8_t)(cache.push(getID(obj), &shouldBind) + offset);
		if (shouldBind)
			innerBind(obj, pos);
		return pos;
	}

	//for max cache usage, only use it when release real resource
	void forcePop(const GLuint id)
	{
		cache.pop(id);
	}
};


//----ResourceDistrubutor
class TextureManager : public ResDister<TextureManager, oglTexture, 4>
{
	friend class ResDister<TextureManager, oglTexture, 4>;
protected:
	GLuint getID(const oglTexture& obj) const;
	void innerBind(const oglTexture& obj, const uint8_t pos) const;
	void outterBind(const GLuint pid, const GLuint pos, const uint8_t val) const;
public:
	TextureManager() :ResDister((GLenum)GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS) { }
};

class UBOManager : public ResDister<UBOManager, oglUBO, 4>
{
	friend class ResDister<UBOManager, oglUBO, 4>;
protected:
	GLuint getID(const oglUBO& obj) const;
	void innerBind(const oglUBO& obj, const uint8_t pos) const;
	void outterBind(const GLuint pid, const GLuint pos, const uint8_t val) const;
public:
	UBOManager() :ResDister((GLenum)GL_MAX_UNIFORM_BUFFER_BINDINGS) { }
};
//ResourceDistrubutor----


}

