#pragma once

#include "oglRely.h"
#include "oglTexture.h"
#include "oglBuffer.h"
#include "oglProgram.h"

namespace oglu::detail
{


struct LinkBlock
{
    uint16_t head, tail;
};

template<class Key>
struct NodeBlock
{
    Key obj;
    uint16_t prev, next;
};

template<class Key>
class LRUPos : public NonCopyable
{
private:
    vector<NodeBlock<Key>> data;
    std::map<Key, uint16_t> lookup;
    LinkBlock used, unused;
    uint16_t usedCnt, unuseCnt;
    void removeNode(uint16_t pos, LinkBlock& link)
    {
        auto& node = data[pos];

        if (pos == link.head)//is head
            link.head = node.next;//next become head
        else//not head
            data[node.prev].next = node.next;

        if (pos == link.tail)//is tail
            link.tail = node.prev;//prev become tail
        else//not tail
            data[node.next].prev = node.prev;
    }
    void addNode(uint16_t pos, LinkBlock& link)//become head
    {
        auto& node = data[pos];
        node.prev = UINT16_MAX;
        node.next = link.head;
        link.head = pos;
        if (node.next != UINT16_MAX)//has element behind
            data[node.next].prev = pos;
        else//only element, change tail
            link.tail = pos;
    }
    void upNode(uint16_t pos, LinkBlock& link)
    {
        if (pos == link.head)//is head
            return;
        removeNode(pos, link);
        addNode(pos, link);
    }
    void moveNode(uint16_t pos, LinkBlock& from, LinkBlock& to)
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
            auto& node = data[a];
            node.prev = static_cast<uint16_t>(a - 1);
            node.next = static_cast<uint16_t>(a + 1);
        }
        data[size - 1].next = UINT16_MAX;
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
                data[pos].obj = obj;
                moveNode(pos, unused, used);
                usedCnt++, unuseCnt--;
            }
            else
            {//no empty node
                pos = used.tail;
                if (onRemove != nullptr)
                    onRemove(data[pos].obj, pos);
                data[pos].obj = obj;
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
template<class D, class T, uint8_t Offset>
class ResDister : public NonCopyable
{
private:
    static GLint GetLimit(const GLenum prop)
    {
        GLint maxs;
        glGetIntegerv(prop, &maxs);
        return maxs;
    }
    static uint8_t GetSize(const GLenum prop)
    {
        GLint maxs;
        glGetIntegerv(prop, &maxs);
        maxs -= Offset;
        return static_cast<uint8_t>(maxs > 255 ? 255 : maxs);
    }
    //get obj's inner id
    GLuint getID(const T& obj) const
    {
        return ((const D*)this)->getID(obj);
    }
    //when an slot is allocatted for an obj whose ID is id
    void innerBind(const T& obj, const uint16_t pos) const
    {
        ((const D*)this)->innerBind(obj, pos);
    }
    void outterBind(const GLuint prog, const GLuint pos, const uint16_t val) const
    {
        ((const D*)this)->outterBind(prog, pos, val);
    }
protected:
    LRUPos<GLuint> cache;
    const uint32_t GLLimit;
    ResDister(uint8_t size) :cache(size) { }
    ResDister(GLenum prop) :GLLimit(GetLimit(prop)), cache(GetSize(prop)) { }
public:
    void bindAll(const GLuint prog, const std::map<GLuint, T>& objs, vector<GLint>& poss)
    {
        const std::pair<const GLuint, T> *rebinds[256]; uint32_t rebindCnt = 0;
        for (const auto& item : objs)
        {
            GLuint pos = 0;
            if (item.second)
            {
                const auto ret = cache.touch(getID(item.second));
                if (ret == UINT16_MAX) //not in cache, need rebind
                {
                    rebinds[rebindCnt++] = &item;
                    continue;
                }
                else //in cahce, has touched
                    pos = static_cast<GLint>(ret) + Offset;
            }
            if (poss[item.first] != pos) //resource's pos has changed
                outterBind(prog, item.first, poss[item.first] = pos);
        }
        for (uint32_t i = 0; i < rebindCnt; ++i)
        {
            const auto& item = *rebinds[i];
            outterBind(prog, item.first, poss[item.first] = bind(item.second));
        }
    }
    uint16_t bind(const T& obj)
    {
        bool shouldBind = false;
        const uint16_t pos = (uint16_t)(cache.push(getID(obj), &shouldBind) + Offset);
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
    void innerBind(const oglTexture& obj, const uint16_t pos) const;
    void outterBind(const GLuint pid, const GLuint pos, const uint16_t val) const;
public:
    TextureManager() :ResDister((GLenum)GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS) { }
};

class UBOManager : public ResDister<UBOManager, oglUBO, 4>
{
    friend class ResDister<UBOManager, oglUBO, 4>;
protected:
    GLuint getID(const oglUBO& obj) const;
    void innerBind(const oglUBO& obj, const uint16_t pos) const;
    void outterBind(const GLuint pid, const GLuint pos, const uint16_t val) const;
public:
    UBOManager() :ResDister((GLenum)GL_MAX_UNIFORM_BUFFER_BINDINGS) { }
};
//ResourceDistrubutor----


}

