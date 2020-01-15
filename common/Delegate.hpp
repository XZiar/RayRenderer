#pragma once
#include "CommonRely.hpp"
#include "SpinLock.hpp"
#include "IntrusiveDoubleLinkList.hpp"
#include <functional>
#include <random>

namespace common
{


class CallbackToken
{
    template<typename...> friend class Delegate;
    void* CallbackPtr;
    uint32_t ID;
    CallbackToken(void* ptr, const uint32_t id) : CallbackPtr(ptr), ID(id) { }
};

template<typename... Args>
class Delegate : public NonCopyable
{
private:
    struct CallbackNode : container::IntrusiveDoubleLinkListNodeBase<CallbackNode>
    {
        uint32_t ID;
        uint32_t UID;
        std::function<void(const Args&...)> Callback;
        template<typename T>
        CallbackNode(const uint32_t id, const uint32_t uid, T&& callback)
            : ID(id), UID(uid), Callback(std::forward<T>(callback)) { }
    };

    container::IntrusiveDoubleLinkList<CallbackNode> CallbackList;
    std::atomic_uint32_t Indexer;
    mutable RWSpinLock Lock;
    uint32_t UID;
public:
    constexpr Delegate() : Indexer(0), Lock(), UID(std::random_device()())
    { }
    ~Delegate()
    {
        CallbackList.ForEach([](CallbackNode* node) { delete node; }, true);
    }
    void operator()(const Args&... args) const
    {
        if (CallbackList.IsEmpty()) // fast pass when no callback is registered
            return;
        const auto lock = Lock.ReadScope();
        CallbackList.ForEachRead([&](CallbackNode* node) 
            {
                node->Callback(args...);
            });
    }
    template<typename T>
    CallbackToken operator+=(T&& callback)
    {
        const auto lock = Lock.WriteScope();
        const auto idx = Indexer++;
        auto node = new CallbackNode(idx, UID, std::forward<T>(callback));
        CallbackList.AppendNode(node);
        return CallbackToken(node, idx);
    }
    bool operator-=(const CallbackToken& token)
    {
        auto node = reinterpret_cast<CallbackNode*>(token.CallbackPtr);
        if (node == nullptr || node->UID != UID || token.ID != node->ID || CallbackList.IsEmpty())
            return false;
        else
        {
            const auto lock = Lock.WriteScope();
            CallbackList.PopNode(node);
            delete node;
            return true;
        }
    }
};


}
