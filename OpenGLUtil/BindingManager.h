#pragma once

#include "oglPch.h"
#include "oglTexture.h"
#include "oglBuffer.h"
#include "oglContext.h"

namespace oglu::detail
{


class ResManCore : public common::NonCopyable
{
private:
    struct NodeBlock
    {
        GLuint Data;
        uint16_t Prev, Next;
    };
    struct LinkBlock
    {
        uint16_t Head = UINT16_MAX, Tail = UINT16_MAX;
    };
    std::vector<NodeBlock> Nodes;
    LinkBlock Unused;
    LinkBlock Used;
    uint16_t UsedCnt, UnusedCnt;

    forceinline uint16_t GetNodePos(const NodeBlock* node) const noexcept
    {
        return static_cast<uint16_t>(node - Nodes.data());
    }

    ///<summary>append a node to the link</summary>  
    ///<param name="node">node being appended</param>
    ///<param name="link">target link</param>
    forceinline void AppendLink(NodeBlock* node, LinkBlock& link) noexcept
    {
        const auto pos = GetNodePos(node);

        if (link.Head == UINT16_MAX)
            link.Head = pos;
        else
            Nodes[link.Tail].Next = pos, node->Prev = link.Tail;

        link.Tail = pos;
    }
    ///<summary>prepend a link to another link's head</summary>  
    ///<param name="src">source link</param>
    ///<param name="dst">destination link</param>
    forceinline void PrependLink(LinkBlock& src, LinkBlock& dst) noexcept
    {
        if (dst.Head != UINT16_MAX) // has head
            Nodes[dst.Head].Prev = src.Tail, Nodes[src.Tail].Next = dst.Head;
        else // no head, empty link
            dst.Tail = src.Tail;
        dst.Head = src.Head;
        src.Head = src.Tail = UINT16_MAX;
    }


    ///<summary>detach a node at [pos] from used</summary>  
    ///<param name="pos">node index</param>
    forceinline NodeBlock* DetachUsed(const uint16_t pos) noexcept
    {
        if (pos == UINT16_MAX) 
            return nullptr;
        auto& node = Nodes[pos];

        if (pos == Used.Head) // is head
            Used.Head = node.Next; // next become head
        else // not head
            Nodes[node.Prev].Next = node.Next;

        if (pos == Used.Tail) // is tail
            Used.Tail = node.Prev; // prev become tail
        else // not tail
            Nodes[node.Next].Prev = node.Prev;

        node.Prev = node.Next = UINT16_MAX;
        return &node;
    }
    ///<summary>detach a node from used</summary>  
    ///<param name="node">node</param>
    forceinline NodeBlock* DetachUsed(NodeBlock* node) noexcept
    {
        if (node == nullptr)
            return node;
        const auto pos = GetNodePos(node);

        if (pos == Used.Head) // is head
            Used.Head = node->Next; // next become head
        else // not head
            Nodes[node->Prev].Next = node->Next;

        if (pos == Used.Tail) // is tail
            Used.Tail = node->Prev; // prev become tail
        else // not tail
            Nodes[node->Next].Prev = node->Prev;

        node->Prev = node->Next = UINT16_MAX;
        return node;
    }

    ///<summary>alloc a node from unused</summary>  
    forceinline std::pair<NodeBlock*, uint16_t> AllocUnused() noexcept
    {
        const auto idx = Unused.Head;
        if (idx == UINT16_MAX) // no avaliable
            return { nullptr, UINT16_MAX };
        
        auto& node = Nodes[idx];
        if (Unused.Tail == Unused.Head) // only one
            Unused.Tail = Unused.Head = UINT16_MAX;
        else
        {
            Nodes[node.Next].Prev = UINT16_MAX;
            Unused.Head = node.Next;
            node.Next = UINT16_MAX;
        }
        
        UnusedCnt--, UsedCnt++;
        return { &node, idx };
    }
    ///<summary>alloc a node from used</summary>  
    forceinline std::pair<NodeBlock*, uint16_t> AllocUsed() noexcept
    {
        const auto idx = Used.Tail;
        if (Used.Tail == UINT16_MAX) // no avaliable
            return { nullptr, UINT16_MAX };

        auto& node = Nodes[idx];
        if (Used.Tail == Used.Head) // only one
            Used.Tail = Used.Head = UINT16_MAX;
        else
        {
            Used.Tail = node.Prev;
            node.Prev = UINT16_MAX;
        }

        return { &node, idx };
    }
    ///<summary>release a node to unused</summary>  
    forceinline void ReleaseNode(NodeBlock* node, const uint16_t pos) noexcept
    {
        const auto idx = Unused.Head;
        if (idx == UINT16_MAX) // empty
            Unused.Tail = pos;
        else // has head
            node->Next = Unused.Head, Nodes[Unused.Head].Next = pos;
        Unused.Head = pos;

        UsedCnt--, UnusedCnt++;
    }

    forceinline std::pair<NodeBlock*, uint16_t> Find(const GLuint obj) noexcept
    {
        for (uint16_t idx = Used.Head; idx != UINT16_MAX;)
        {
            auto& node = Nodes[idx];
            if (node.Data == obj)
                return { &node, idx };
            idx = node.Next;
        }
        return { nullptr, UINT16_MAX };
    }
public:
    ResManCore(const uint16_t size)
    {
        Nodes.resize(size);
        for (uint16_t loc = 0; loc < size; ++loc)
        {
            auto& node = Nodes[loc];
            node.Data = GLInvalidEnum;
            node.Prev = static_cast<uint16_t>(loc - 1);
            node.Next = static_cast<uint16_t>(loc + 1);
        }
        if (size > 0)
        {
            Nodes[0].Prev = Nodes[size - 1].Next = UINT16_MAX;
            Used.Head = Used.Tail = UINT16_MAX;
            Unused.Head = 0, Unused.Tail = static_cast<uint16_t>(size - 1);
            UsedCnt = 0, UnusedCnt = static_cast<uint16_t>(size);
        }
        else
        {
            Used.Head   = Used.Tail   = UINT16_MAX;
            Unused.Head = Unused.Tail = UINT16_MAX;
            UsedCnt = UnusedCnt = 0;
        }
    }

    struct BindInfo
    {
        GLuint Obj;
        uint16_t Slot;
        bool Changed;
        bool NewAlloc;
        BindInfo(GLuint obj, GLint slot) : Obj(obj), Slot(static_cast<uint16_t>(slot)), Changed(false), NewAlloc(false) { }
    };
    ///<summary>bind multiple objects</summary>
    ///<param name="bindings">current bindings</param>
    bool BindAll(common::span<BindInfo> bindings) noexcept
    {
        uint32_t rebinds = 0;

        for (auto& bind : bindings)
        {
            if (const auto [pNode, idx] = Find(bind.Obj); pNode)
            {
                bind.Changed = bind.Slot != idx;
                bind.Slot = idx;
                bind.NewAlloc = false;
            }
            else
            {
                bind.Slot = UINT16_MAX;
                bind.NewAlloc = true;
                rebinds++;
            }
        }

        if (rebinds == 0)
            return true; // do not modify the link to save time

        LinkBlock temp;
        // detach binded used to avoid confilict
        for (const auto& bind : bindings)
        {
            if (!bind.NewAlloc)
            {
                const auto pNode = DetachUsed(bind.Slot);
                AppendLink(pNode, temp);
            }
        }

        // alloc for unbinded
        for (auto& bind : bindings)
        {
            if (bind.NewAlloc)
            {
                auto [pNode, idx] = AllocUnused();
                if (!pNode)
                    std::tie(pNode, idx) = AllocUsed();
                if (pNode)
                {
                    pNode->Data = bind.Obj;
                    bind.Changed = bind.Slot != idx;
                    bind.Slot = idx;
                    AppendLink(pNode, temp);
                    rebinds--;
                }
                else
                {
                    break; // ignore later rebinds cause no node will be avaliable
                }
            }
        }

        PrependLink(temp, Used);
        return rebinds == 0;
    }

    void ReleaseNode(GLuint obj) noexcept
    {
        if (const auto [pNode, idx] = Find(obj); pNode)
        {
            pNode->Data = GLInvalidEnum;
            DetachUsed(pNode);
            ReleaseNode(pNode, idx);
        }
    }
};


template<typename T>
class CachedResManager : public ResourceBinder<T>
{
private:
    ResManCore Core;
    static GLuint GetID(const T* res) noexcept;
    void BindRess(const T* res, const uint16_t slot) noexcept;
    void BindToProg(const GLuint progId, const GLuint loc, const uint16_t slot) noexcept;
public:
    CachedResManager(const GLint size) : Core(static_cast<uint16_t>(std::min(std::max(size, 2) - 2, 512))) { }
    ~CachedResManager() override {}
    void BindAll(const GLuint progId, const std::map<GLuint, std::shared_ptr<T>>& bindingMap, std::vector<GLint>& bingdingVals) override
    {
        std::vector<ResManCore::BindInfo> bindings;
        bindings.reserve(bindingMap.size());
        for (const auto& [loc, res] : bindingMap)
        {
            if (res)
                bindings.emplace_back(GetID(res.get()), bingdingVals[loc] - 2);
        }
        Core.BindAll(bindings);

        size_t idx = 0;
        for (const auto& [loc, res] : bindingMap)
        {
            const auto& bind = bindings[idx++];
            if (bind.Slot == UINT16_MAX) continue;
            const uint16_t slot = static_cast<uint16_t>(bind.Slot + 2);
            if (bind.NewAlloc)
                BindRess(res.get(), slot);
            if (bind.Changed)
            {
                BindToProg(progId,loc, slot);
                bingdingVals[loc] = slot;
            }
        }
    }
    void ReleaseRes(const T* res) override
    {
        Core.ReleaseNode(GetID(res));
    }
};


}

