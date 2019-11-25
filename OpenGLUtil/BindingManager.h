#pragma once

#include "oglPch.h"
#include "oglTexture.h"
#include "oglBuffer.h"
#include "oglContext.h"

namespace oglu::detail
{


struct LinkBlock
{
    uint16_t Head = UINT16_MAX, Tail = UINT16_MAX;
};

template<class Key>
struct NodeBlock
{
    Key Data;
    uint16_t Prev, Next;
    uint8_t LinkId;
};

template<class Key>
class LRUPos : public common::NonCopyable
{
private:
    std::vector<NodeBlock<Key>> Nodes;
    std::map<Key, uint16_t> lookup;
    LinkBlock Links[3];
    constexpr LinkBlock& Unused() { return Links[0]; }
    constexpr LinkBlock& Used()   { return Links[1]; }
    constexpr LinkBlock& Fixed()  { return Links[2]; }
    constexpr uint8_t GetLinkId(const LinkBlock& link) const { return static_cast<uint8_t>(&link - Links); }
    
    uint16_t usedCnt, unuseCnt;
    ///<summary>move a node at [pos] from itslink to [link]'s head</summary>  
    ///<param name="pos">node index in data</param>
    ///<param name="destLink">desire link, used for head/tail check</param>
    void moveNode(const uint16_t pos, LinkBlock& destLink)
    {
        auto& node = Nodes[pos];
        auto& srcLink = Links[node.LinkId];

        if (pos == srcLink.Head)//is head
            srcLink.Head = node.Next;//next become head
        else//not head
            Nodes[node.Prev].Next = node.Next;

        if (pos == srcLink.Tail)//is tail
            srcLink.Tail = node.Prev;//prev become tail
        else//not tail
            Nodes[node.Next].Prev = node.Prev;

        node.Prev = UINT16_MAX;
        node.Next = destLink.Head;
        if (destLink.Head != UINT16_MAX)//has element before
            Nodes[destLink.Head].Prev = pos;
        else//only element, change tail
            destLink.Tail = pos;
        destLink.Head = pos;
        node.LinkId = GetLinkId(destLink);
    }
    ///<summary>move a node at [pos] to its link's head</summary>  
    ///<param name="pos">node index in data</param>
    void upNode(uint16_t pos)
    {
        auto& node = Nodes[pos];
        auto& link = Links[node.LinkId];
        if (pos == link.Head)//is head
            return;
        //not head
        Nodes[node.Prev].Next = node.Next;
        if (pos == link.Tail)//is tail
            link.Tail = node.Prev;//prev become tail
        else//not tail
            Nodes[node.Next].Prev = node.Prev;
        node.Prev = UINT16_MAX;
        node.Next = link.Head;
        Nodes[link.Head].Prev = pos;
        link.Head = pos;
    }
public:
    using CacheCallBack = std::function<void(const Key& obj, const uint16_t pos)>;
    CacheCallBack onRemove;
    //LRUPos() { }
    LRUPos(const uint16_t size)
    {
        Nodes.resize(size);
        for (uint16_t a = 0; a < size; ++a)
        {
            auto& node = Nodes[a];
            node.Prev = static_cast<uint16_t>(a - 1);
            node.Next = static_cast<uint16_t>(a + 1);
            node.LinkId = GetLinkId(Unused());
        }
        Nodes[size - 1].Next = UINT16_MAX;
        Used().Head = Used().Tail = UINT16_MAX;
        Unused().Head = 0, Unused().Tail = static_cast<uint16_t>(size - 1);
        Fixed().Head = Fixed().Tail = UINT16_MAX;
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
            if (Unused().Head != UINT16_MAX)
            {//has empty node
                pos = Unused().Head;
                Nodes[pos].Data = obj;
                moveNode(pos, Used());
                usedCnt++, unuseCnt--;
            }
            else
            {//no empty node
                pos = Used().Tail;
                if (onRemove != nullptr)
                    onRemove(Nodes[pos].Data, pos);
                Nodes[pos].Data = obj;
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
        moveNode(it->second, Unused());
        lookup.erase(it);
        usedCnt--, unuseCnt++;
    }
    void pin(const uint16_t count)
    {
        if (Fixed().Head != UINT16_MAX)
            return;
        Fixed().Head = Used().Head;
        uint16_t tail = Fixed().Head;
        for (uint16_t i = 1; i < count && tail != UINT16_MAX; ++i)
        {
            auto& node = Nodes[tail];
            tail = node.Next;
            node.LinkId = GetLinkId(Fixed()); 
        }
        Nodes[tail].LinkId = GetLinkId(Fixed());
        Fixed().Tail = tail;
        Used().Head = Nodes[tail].Next;
        Nodes[tail].Next = UINT16_MAX;
        if(Used().Head != UINT16_MAX)
            Nodes[Used().Head].Prev = UINT16_MAX;
    }
    void unpin()
    {
        if (Fixed().Head == UINT16_MAX)
            return;
        for (uint16_t cur = Fixed().Head; cur != UINT16_MAX;)
        {
            auto& node = Nodes[cur];
            node.LinkId = GetLinkId(Used());
            cur = node.Next;
        }
        if (Used().Head != UINT16_MAX)
            Nodes[Used().Head].Prev = Fixed().Tail, Nodes[Fixed().Tail].Next = Used().Head;
        Used().Head = Fixed().Head;
        if (Used().Tail == UINT16_MAX)
            Used().Tail = Fixed().Tail;
        Fixed().Head = Fixed().Tail = UINT16_MAX;
    }
};


class oglProgram_;

//D:derived class type, T:resource type
template<class D, class T, uint8_t Offset>
class ResDister : public common::NonCopyable
{
private:
    static GLint GetLimit(const GLenum prop)
    {
        GLint maxs;
        CtxFunc->ogluGetIntegerv(prop, &maxs);
        return maxs;
    }
    static uint8_t GetSize(const GLenum prop)
    {
        GLint maxs;
        CtxFunc->ogluGetIntegerv(prop, &maxs);
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
    ResDister(GLenum prop) :cache(GetSize(prop)), GLLimit(GetLimit(prop)) { }
public:
    ///<summary>bind multiple objects</summary>
    ///<param name="prog">progID</param>
    ///<param name="objs">binding map requested from [loc] to [objID]</param>
    ///<param name="curBindings">current binding vectoe of [slot] indexed by [loc]</param>
    void bindAll(const GLuint prog, const std::map<GLuint, T>& objs, std::vector<GLint>& curBindings, const bool shouldPin = false)
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

class TexImgManager : public ResDister<TexImgManager, oglImgBase, 4>
{
    friend class ResDister<TexImgManager, oglImgBase, 4>;
protected:
    GLuint getID(const oglImgBase& obj) const;
    void innerBind(const oglImgBase& obj, const uint16_t slot) const;
    void outterBind(const GLuint prog, const GLuint loc, const uint16_t slot) const;
public:
    TexImgManager() :ResDister((GLenum)GL_MAX_IMAGE_UNITS) { }
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

