#include "RenderCoreWrapRely.h"
#include "Async.h"
using namespace System::Runtime::CompilerServices;

namespace Common
{


AsyncWaiter::AsyncItem::AsyncItem(std::shared_ptr<common::detail::PromiseResultCore>&& pmsCore, SynchronizationContext^ syncContext, Action^ afterComplete)
{
    WRAPPER_NATIVE_PTR(PmsCore, ptr);
    new (ptr) std::shared_ptr<common::detail::PromiseResultCore>(pmsCore);
    SyncContext = syncContext;
    AfterComplete = afterComplete;
}

AsyncWaiter::AsyncItem::!AsyncItem()
{
    WRAPPER_NATIVE_PTR(PmsCore, ptr);
    ptr->~shared_ptr();
}

bool AsyncWaiter::AsyncItem::IsComplete()
{
    WRAPPER_NATIVE_PTR(PmsCore, ptr);
    return static_cast<uint8_t>((*ptr)->GetState()) >= CompleteState;
}

static void PerformAction(Object^ action)
{
    static_cast<Action^>(action)->Invoke();
}

static AsyncWaiter::AsyncWaiter()
{
    TaskList = gcnew LinkedList<AsyncItem^>();
    AsyncCallback = gcnew SendOrPostCallback(&PerformAction);
    TaskThread = gcnew Thread(gcnew ThreadStart(&AsyncWaiter::PerformTask));
    TaskThread->Start();
}

void AsyncWaiter::PerformTask()
{
    while (true)
    {
        bool hasChecked = false, hasComplete = false;
        Monitor::Enter(TaskList);
        auto node = TaskList->First;
        while (node != nullptr)
        {
            Monitor::Exit(TaskList);
            hasChecked = true; 
            LinkedListNode<AsyncItem^>^ del = nullptr;
            if (node->Value->IsComplete())
            {
                hasComplete = true;
                if (SynchronizationContext::Current == node->Value->SyncContext)
                    node->Value->AfterComplete->Invoke();
                else
                    node->Value->SyncContext->Post(AsyncCallback, node->Value->AfterComplete);
                del = node;
            }
            Monitor::Enter(TaskList);
            node = node->Next;
            if (del != nullptr)
                TaskList->Remove(del);
        }
        if (!hasChecked)
            Monitor::Wait(TaskList);
        else
        {
            Monitor::Exit(TaskList);
            if (!hasComplete)
                Thread::Sleep(10);
        }
    }
}

void AsyncWaiter::Put(AsyncItem^ item)
{
    Monitor::Enter(TaskList);
    try
    {
        auto node = TaskList->AddLast(item);
        if (node->Previous == nullptr) // empty
            Monitor::Pulse(TaskList);
    }
    finally
    {
        Monitor::Exit(TaskList);
    }
}




}
