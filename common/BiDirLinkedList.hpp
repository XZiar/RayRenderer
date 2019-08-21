#include "CommonRely.hpp"
#include "SpinLock.hpp"
#include <atomic>
#include <type_traits>


namespace common::container
{

template<typename NodeType>
class BiDirLinkedList : public NonCopyable
{
public:
    struct NodeBase : public NonCopyable
    {
        friend class BiDirLinkedList<NodeType>;
    private:
        NodeType* Prev = nullptr, *Next = nullptr; //spin-locker's memory_order_seq_cst promise their order
    };
    //static_assert(std::is_base_of_v<NodeBase, NodeType>, "NodeType should deriver from NodeBase");
private:
    std::atomic<NodeType*> Head{ nullptr }, Tail{ nullptr };
    std::atomic_flag ModifyFlag = ATOMIC_FLAG_INIT; //spinlock for modify TaskNode
public:
    bool AddNode(NodeType* node, NodeType* prevNode)
    {
        bool becomeNonEmpty = false;
        node->Next = nullptr;
        common::SpinLocker::Lock(ModifyFlag);//ensure add is done
        node->Prev = prevNode;
        if (prevNode == Tail)
            Tail = node;
        if (Head)
            node->Prev->Next = node;
        else //list became not empty
        {
            Head = Tail = node;
            becomeNonEmpty = true;
        }
        common::SpinLocker::Unlock(ModifyFlag);
        return becomeNonEmpty;
    }
    bool AppendNode(NodeType* node) 
    { 
        return AddNode(node, Tail);
    }

    NodeType* DelNode(NodeType* node)
    {
        common::SpinLocker locker(ModifyFlag);
        auto prev = node->Prev, next = node->Next;
        if (prev)
            prev->Next = next;
        else
            Head = next;
        if (next)
            next->Prev = prev;
        else
            Tail = prev;
        delete node;
        return next;
    }

    NodeType* ToNext(const NodeType* node)
    {
        common::SpinLocker locker(ModifyFlag); //ensure 'next' not changed when accessing it
        return node->Next;
    }

    bool IsEmpty() const noexcept { return Head == nullptr; }
    NodeType* Begin() const noexcept { return Head; }
    NodeType* End() const noexcept { return Tail; }

    template<typename Func>
    void ForEach(Func&& func)
    {
        common::SpinLocker locker(ModifyFlag); //ensure no pending adding
        for (NodeType* current = Head; current != nullptr;)
        {
            const auto next = current->Next;
            func(current);
            current = next;
        }
    }
};


}