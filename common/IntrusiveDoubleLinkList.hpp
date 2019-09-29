#include "CommonRely.hpp"
#include "SpinLock.hpp"
#include <atomic>
#include <type_traits>


namespace common::container
{


template<typename NodeType>
class IntrusiveDoubleLinkList;

template<typename NodeType>
struct IntrusiveDoubleLinkListNodeBase : public NonCopyable
{
    friend class IntrusiveDoubleLinkList<NodeType>;
private:
    NodeType* Prev, * Next; //spin-locker's memory_order_seq_cst promise their order
protected:
    IntrusiveDoubleLinkListNodeBase() : Prev(nullptr), Next(nullptr) { }
};

template<typename NodeType>
class IntrusiveDoubleLinkList : public NonCopyable
{
public:
    static_assert(std::is_base_of_v<IntrusiveDoubleLinkListNodeBase<NodeType>, NodeType>, "NodeType should deriver from BiDirLinkedListNodeBase");
private:
    std::atomic<NodeType*> Head{ nullptr }, Tail{ nullptr };
    WRSpinLock ModifyLock;
public:
    bool AppendNode(NodeType* node)
    {
        const auto lock = ModifyLock.ReadScope();
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

    NodeType* PopNode(NodeType* node, bool returnNext = true)
    {
        const auto lock = ModifyLock.WriteScope();
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

    NodeType* ToNext(const NodeType* node) noexcept
    {
        const auto lock = ModifyLock.WriteScope();
        return node->Next;
    }

    bool IsEmpty() const noexcept { return Tail == nullptr; }
    NodeType* Begin() const noexcept { return Head; }
    NodeType* End() const noexcept { return Tail; }

    template<typename Func>
    void ForEach(Func&& func, bool cleanup = false)
    {
        const auto lock = ModifyLock.WriteScope();
        for (NodeType* current = Head; current != nullptr;)
        {
            const auto next = current->Next;
            func(current);
            current = next;
        }
        if (cleanup)
            Head = nullptr, Tail = nullptr;
    }

    void Clear(bool needToRelease = true)
    {
        NodeType* current = nullptr;
        {
            const auto lock = ModifyLock.WriteScope();
            current = Head.exchange(nullptr);
            Tail = nullptr;
        }
        if (needToRelease)
        {
            while (current != nullptr)
            {
                const auto next = current->Next;
                delete current;
                current = next;
            }
        }
    }
};


}