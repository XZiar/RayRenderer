#include "RenderCoreWrapRely.h"
#include "Async.h"
using namespace System::Runtime::CompilerServices;

namespace Common
{


AsyncWaiter::AsyncTaskBase::!AsyncTaskBase()
{
    PmsCore.Destruct();
}

bool AsyncWaiter::AsyncTaskBase::IsComplete()
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
    TaskList = gcnew LinkedList<AsyncTaskBase^>();
    AsyncCallback = gcnew SendOrPostCallback(&PerformAction);
    TaskThread = gcnew Thread(gcnew ThreadStart(&AsyncWaiter::PerformTask));
    TaskThread->IsBackground = true;
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
            LinkedListNode<AsyncTaskBase^>^ del = nullptr;
            if (node->Value->IsComplete())
            {
                hasComplete = true;
                node->Value->SetResult();
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
                Thread::Sleep(40);
        }
    }
}

void AsyncWaiter::Put(AsyncTaskBase^ item)
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
