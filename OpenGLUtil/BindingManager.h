#pragma once

#include "oglPch.h"
#include "oglTexture.h"
#include "oglBuffer.h"
#include "oglContext.h"

namespace oglu::detail
{


class ResManCore : public common::NonCopyable
{
protected:
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
public:
    ResManCore(const uint16_t size) noexcept;
    
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
    bool BindAll(common::span<BindInfo> bindings) noexcept;
    void ReleaseNode(GLuint obj) noexcept;
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


class BindlessTexManager : public ResourceBinder<oglTexBase_>
{
public:
    ~BindlessTexManager() override {}
    void BindAll(const GLuint progId, const std::map<GLuint, oglTexBase>& bindingMap, std::vector<GLint>&) override;
    void ReleaseRes(const oglTexBase_* res) override;
};


class BindlessImgManager : public ResourceBinder<oglImgBase_>
{
public:
    ~BindlessImgManager() override {}
    void BindAll(const GLuint progId, const std::map<GLuint, oglImgBase>& bindingMap, std::vector<GLint>&) override;
    void ReleaseRes(const oglImgBase_* res) override;
};


}

