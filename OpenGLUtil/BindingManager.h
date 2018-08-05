#pragma once

#include "oglRely.h"
#include "oglTexture.h"
#include "oglBuffer.h"

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
    uint8_t link;
};

template<class Key>
class LRUPos : public NonCopyable
{
private:
    vector<NodeBlock<Key>> data;
    std::map<Key, uint16_t> lookup;
    union
    {
        struct { LinkBlock unused, used, fixed; };
        LinkBlock links[3];
    };
    
    uint16_t usedCnt, unuseCnt;
    ///<summary>move a node at [pos] from itslink to [link]'s head</summary>  
    ///<param name="pos">node index in data</param>
    ///<param name="link">desire link, used for head/tail check</param>
    void moveNode(const uint16_t pos, LinkBlock& destLink)
    {
        auto& node = data[pos];
        auto& srcLink = links[node.link];

        if (pos == srcLink.head)//is head
            srcLink.head = node.next;//next become head
        else//not head
            data[node.prev].next = node.next;

        if (pos == srcLink.tail)//is tail
            srcLink.tail = node.prev;//prev become tail
        else//not tail
            data[node.next].prev = node.prev;

        node.prev = UINT16_MAX;
        node.next = destLink.head;
        if (destLink.head != UINT16_MAX)//has element before
            data[destLink.head].prev = pos;
        else//only element, change tail
            destLink.tail = pos;
        destLink.head = pos;
        node.link = static_cast<uint8_t>(&destLink - links);
    }
    ///<summary>move a node at [pos] to its link's head</summary>  
    ///<param name="pos">node index in data</param>
    void upNode(uint16_t pos)
    {
        auto& node = data[pos];
        auto& link = links[node.link];
        if (pos == link.head)//is head
            return;
        //not head
        data[node.prev].next = node.next;
        if (pos == link.tail)//is tail
            link.tail = node.prev;//prev become tail
        else//not tail
            data[node.next].prev = node.prev;
        node.prev = UINT16_MAX;
        node.next = link.head;
        data[link.head].prev = pos;
        link.head = pos;
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
            node.link = static_cast<uint8_t>(&unused - links);
        }
        data[size - 1].next = UINT16_MAX;
        used.head = used.tail = UINT16_MAX;
        unused.head = 0, unused.tail = static_cast<uint16_t>(size - 1);
        fixed.head = fixed.tail = UINT16_MAX;
        usedCnt = 0, unuseCnt = static_cast<uint16_t>(size);
    }
    //touch(move to head) a obj, return if it is in cache
    uint16_t touch(const Key& obj)
    {
        const auto it = lookup.find(obj);
        if (it == lookup.end())
            return UINT16_MAX;
        upNode(it->second);
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
                moveNode(pos, used);
                usedCnt++, unuseCnt--;
            }
            else
            {//no empty node
                pos = used.tail;
                if (onRemove != nullptr)
                    onRemove(data[pos].obj, pos);
                data[pos].obj = obj;
                upNode(pos);
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
            upNode(it->second);
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
        moveNode(it->second, unused);
        lookup.erase(it);
        usedCnt--, unuseCnt++;
    }
    void pin(const uint16_t count)
    {
        if (fixed.head != UINT16_MAX)
            return;
        fixed.head = used.head;
        uint16_t tail = fixed.head;
        data[tail].link = static_cast<uint8_t>(&fixed - links);
        for (uint16_t i = 1; i < count && tail != UINT16_MAX; ++i)
        {
            auto& node = data[tail];
            tail = node.next;
            node.link = static_cast<uint8_t>(&fixed - links);
        }
        fixed.tail = tail;
        used.head = data[tail].next;
        data[tail].next = UINT16_MAX;
        if(used.head != UINT16_MAX)
            data[used.head].prev = UINT16_MAX;
    }
    void unpin()
    {
        if (fixed.head == UINT16_MAX)
            return;
        for (uint16_t cur = fixed.head; cur != UINT16_MAX;)
        {
            auto& node = data[cur];
            node.link = static_cast<uint8_t>(&used - links);
            cur = node.next;
        }
        if (used.head != UINT16_MAX)
            data[used.head].prev = fixed.tail, data[fixed.tail].next = used.head;
        used.head = fixed.head;
        fixed.head = fixed.tail = UINT16_MAX;
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
    ///<summary>return ID for the target</summary>  
    ///<param name="obj">target</param>
    ///<returns>[objID]</returns>
    GLuint getID(const T& obj) const
    {
        return ((const D*)this)->getID(obj);
    }
    ///<summary>when an slot is allocatted for an obj</summary>  
    ///<param name="obj">obj</param>
    ///<param name="slot">allocatted [slot]</param>
    void innerBind(const T& obj, const uint16_t slot) const
    {
        ((const D*)this)->innerBind(obj, slot);
    }
    ///<summary>when the resource need to bind to a new mid-loc</summary>  
    ///<param name="loc">[loc]</param>
    ///<param name="slot">[slot]</param>
    void outterBind(const GLuint prog, const GLuint loc, const uint16_t slot) const
    {
        ((const D*)this)->outterBind(prog, loc, slot);
    }
protected:
    LRUPos<GLuint> cache;
    const uint32_t GLLimit;
    ResDister(uint8_t size) :cache(size) { }
    ResDister(GLenum prop) :GLLimit(GetLimit(prop)), cache(GetSize(prop)) { }
public:
    ///<summary>bind multiple objects</summary>
    ///<param name="prog">progID</param>
    ///<param name="objs">binding map requested from [loc] to [objID]</param>
    ///<param name="curBindings">current binding vectoe of [slot] indexed by [loc]</param>
    void bindAll(const GLuint prog, const std::map<GLuint, T>& objs, vector<GLint>& curBindings, const bool shouldPin = false)
    {
        const std::pair<const GLuint, T> *rebinds[256]; uint32_t rebindCnt = 0;
        uint16_t bindCount = 0;
        for (const std::pair<const GLuint, T>& item : objs)
        {
            if (!item.second) //if not valid, just skip, undefined behaviour
                continue;
            bindCount++;
            const auto ret = cache.touch(getID(item.second));
            if (ret == UINT16_MAX) //not in cache, need rebind
            {
                rebinds[rebindCnt++] = &item;
                continue;
            }
            //in cache, has binded
            const uint16_t slot = static_cast<uint16_t>(ret + Offset);
            if (auto& bind = curBindings[item.first]; bind != slot) //resource's binded mid-loc has changed
            {
                bind = slot;
                outterBind(prog, item.first, slot);
            }
        }
        for (uint32_t i = 0; i < rebindCnt; ++i)
        {
            const auto& item = *rebinds[i];
            const auto ret = cache.push(getID(item.second), nullptr);
            const uint16_t slot = static_cast<uint16_t>(ret + Offset);
            innerBind(item.second, slot);
            curBindings[item.first] = slot;
            outterBind(prog, item.first, slot);
        }
        if (shouldPin)
            cache.pin(bindCount);
    }
    ///<summary>bind a single object</summary>  
    ///<param name="obj">obj</param>
    ///<returns>slot's mid-loc</returns>
    uint16_t bind(const T& obj, const bool shouldPin = false)
    {
        bool shouldBind = false;
        const auto ret = cache.push(getID(obj), &shouldBind);
        const uint16_t pos = static_cast<uint16_t>(ret + Offset);
        if (shouldBind)
            innerBind(obj, pos);
        if (shouldPin)
            cache.pin(1);
        return pos;
    }
    void unpin()
    {
        cache.unpin();
    }
    //for max cache usage, only use it when release real resource
    void forcePop(const GLuint id)
    {
        cache.pop(id);
    }
};


//----ResourceDistrubutor
class TextureManager : public ResDister<TextureManager, oglTexBase, 4>
{
    friend class ResDister<TextureManager, oglTexBase, 4>;
protected:
    GLuint getID(const oglTexBase& obj) const;
    void innerBind(const oglTexBase& obj, const uint16_t slot) const;
    void outterBind(const GLuint prog, const GLuint loc, const uint16_t slot) const;
public:
    TextureManager() :ResDister((GLenum)GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS) { }
};

class UBOManager : public ResDister<UBOManager, oglUBO, 4>
{
    friend class ResDister<UBOManager, oglUBO, 4>;
protected:
    GLuint getID(const oglUBO& obj) const;
    void innerBind(const oglUBO& obj, const uint16_t slot) const;
    void outterBind(const GLuint prog, const GLuint loc, const uint16_t slot) const;
public:
    UBOManager() :ResDister((GLenum)GL_MAX_UNIFORM_BUFFER_BINDINGS) { }
};
//ResourceDistrubutor----


}

