#pragma once
#include "SpinLock.h"
#include "common/CommonRely.hpp"
#include "common/IntrusiveDoubleLinkList.hpp"
#include <functional>
#include <random>

namespace common
{


class CallbackToken
{
    template<typename...> friend class Delegate;
    void* CallbackPtr;
    uint32_t ID;
    constexpr CallbackToken(void* ptr, const uint32_t id) noexcept : CallbackPtr(ptr), ID(id) { }
public:
    constexpr CallbackToken() noexcept : CallbackPtr(nullptr), ID(0) { }
};

template<typename... Args>
class Delegate
{
private:
    struct CallbackNode : container::IntrusiveDoubleLinkListNodeBase<CallbackNode>
    {
        uint32_t ID;
        uint32_t UID;
        CallbackNode(const uint32_t id, const uint32_t uid)
            : ID(id), UID(uid) { }
        virtual ~CallbackNode() { }
        virtual void Call(const Args&...) = 0;
    };
    struct CallbackNodeFull : public CallbackNode
    {
        std::function<void(const Args&...)> Callback;
        template<typename T>
        CallbackNodeFull(const uint32_t id, const uint32_t uid, T&& callback)
            : CallbackNode(id, uid), Callback(std::forward<T>(callback)) { }
        ~CallbackNodeFull() override { }
        void Call(const Args&... args) override
        {
            Callback(args...);
        }
    };
    struct CallbackNodeNoArg : public CallbackNode
    {
        std::function<void()> Callback;
        template<typename T>
        CallbackNodeNoArg(const uint32_t id, const uint32_t uid, T&& callback)
            : CallbackNode(id, uid), Callback(std::forward<T>(callback)) { }
        ~CallbackNodeNoArg() override { }
        void Call(const Args&...) override
        {
            Callback();
        }
    };

    container::IntrusiveDoubleLinkList<CallbackNode, spinlock::WRSpinLock> CallbackList;
    std::atomic_uint32_t Indexer;
    mutable spinlock::RWSpinLock Lock;
    uint32_t UID;
    template<typename T>
    CallbackNode* CreateNode(const uint32_t id, T&& callback) const
    {
        if constexpr (std::is_invocable_v<T, Args...>)
            return new CallbackNodeFull(id, UID, std::forward<T>(callback));
        else if constexpr (std::is_invocable_v<T>)
            return new CallbackNodeNoArg(id, UID, std::forward<T>(callback));
        else
            static_assert(!AlwaysTrue<T>, "unsupported callback type");
    }
public:
    constexpr Delegate() : Indexer(0), Lock(), UID(std::random_device()())
    { }
    COMMON_NO_COPY(Delegate)
    COMMON_NO_MOVE(Delegate)
    ~Delegate()
    {
        Clear();
    }
    void Clear()
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
                node->Call(args...);
            });
    }
    template<typename T>
    CallbackToken operator+=(T&& callback)
    {
        const auto lock = Lock.WriteScope();
        const auto idx = Indexer++;
        auto node = CreateNode(idx, std::forward<T>(callback));
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
