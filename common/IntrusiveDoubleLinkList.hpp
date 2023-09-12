#pragma once
#include "CommonRely.hpp"
#include "SpinLock.hpp"
#include <atomic>
#include <type_traits>


namespace common::container
{


template<typename NodeType>
class IntrusiveDoubleLinkListBase;

template<typename NodeType>
struct IntrusiveDoubleLinkListNodeBase
{
    friend class IntrusiveDoubleLinkListBase<NodeType>;
private:
    NodeType *Prev, *Next; //spin-locker's memory_order_seq_cst promise their order
protected:
    IntrusiveDoubleLinkListNodeBase() : Prev(nullptr), Next(nullptr) { }
    COMMON_NO_COPY(IntrusiveDoubleLinkListNodeBase)
    COMMON_NO_MOVE(IntrusiveDoubleLinkListNodeBase)
};

template<typename NodeType>
class IntrusiveDoubleLinkListBase
{
public:
    static_assert(std::is_base_of_v<IntrusiveDoubleLinkListNodeBase<NodeType>, NodeType>, "NodeType should deriver from IntrusiveDoubleLinkListNodeBase");
private:
    std::atomic<NodeType*> Head{ nullptr }, Tail{ nullptr };
protected:
    bool AppendNode_(NodeType* node) noexcept
    {
        NodeType* tail = Tail;
        while(true)
        {
            if (!Tail.compare_exchange_strong(tail, node)) // preempted
                continue;
            if (tail == nullptr) // was empty
            {
                Head = node; // only append, ensure no modify on Head
                return true; 
            }
            else
            {
                tail->Next = node;
                node->Prev = tail;
                return false;
            }
        }
    }

    NodeType* PopNode_(NodeType* node, bool returnNext = true) noexcept
    {
        auto prev = node->Prev, next = node->Next;
        if (prev)
            prev->Next = next;
        else
            Head = next;
        if (next)
            next->Prev = prev;
        else
            Tail = prev;
        return returnNext ? next : prev;
    }

    NodeType* ToNext_(const NodeType* node) noexcept
    {
        return node->Next;
    }

    template<typename Func>
    void ForEachRead_(Func&& func) const
    {
        for (NodeType* current = Head; current != nullptr;)
        {
            func(current);
            current = current->Next;
        }
    }

    template<typename Func>
    void ForEach_(Func&& func, bool cleanup = false)
    {
        for (NodeType* current = Head; current != nullptr;)
        {
            const auto next = current->Next;
            func(current);
            current = next;
        }
        if (cleanup)
            Head = nullptr, Tail = nullptr;
    }

    NodeType* Clear_() noexcept
    {
        NodeType* current = nullptr;
        current = Head.exchange(nullptr);
        Tail = nullptr;
        return current;
    }

    static void ReleaseAll_(NodeType* current) noexcept
    {
        while (current != nullptr)
        {
            const auto next = current->Next;
            delete current;
            current = next;
        }
    }
public:
    bool IsEmpty() const noexcept { return Tail == nullptr; }
    NodeType* Begin() const noexcept { return Head; }
    NodeType* End() const noexcept { return Tail; }
};

template<typename NodeType, typename Locker = WRSpinLock>
class IntrusiveDoubleLinkList : public IntrusiveDoubleLinkListBase<NodeType>
{
private:
    mutable Locker ModifyLock;
public:
    bool AppendNode(NodeType* node) noexcept
    {
        const auto lock = ModifyLock.ReadScope();
        return this->AppendNode_(node);
    }

    NodeType* PopNode(NodeType* node, bool returnNext = true) noexcept
    {
        const auto lock = ModifyLock.WriteScope();
        return this->PopNode_(node, returnNext);
    }

    NodeType* ToNext(const NodeType* node) noexcept
    {
        const auto lock = ModifyLock.WriteScope();
        return this->ToNext_(node);
    }

    template<typename Func>
    void ForEachRead(Func&& func) const
    {
        const auto lock = ModifyLock.ReadScope();
        this->ForEachRead_(std::forward<Func>(func));
    }

    template<typename Func>
    void ForEach(Func&& func, bool cleanup = false)
    {
        const auto lock = ModifyLock.WriteScope();
        this->ForEach_(std::forward<Func>(func), cleanup);
    }

    void Clear(bool needToRelease = true) noexcept
    {
        NodeType* current = nullptr;
        {
            const auto lock = ModifyLock.WriteScope();
            current = this->Clear_();
        }
        if (needToRelease)
        {
            this->ReleaseAll_(current);
        }
    }
};


}