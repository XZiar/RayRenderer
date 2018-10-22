#pragma once

#include "RenderCoreWrapRely.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Collections::Specialized;
using namespace System::Threading::Tasks;
using namespace msclr::interop;

namespace Dizz
{

generic<typename T>
private delegate void AddObjectEventHandler(Object^ sender, T obj, bool% shouldAdd);
generic<typename T>
private delegate void ObjectPropertyChangedEventHandler(Object^ sender, T obj, PropertyChangedEventArgs^ e);

generic<typename T> where T : INotifyPropertyChanged
public ref class ObservableProxyContainer : public INotifyCollectionChanged, public IEnumerable<T>
{
private:
    initonly List<T>^ InnerContainer;
    initonly PropertyChangedEventHandler^ ObjectPropertyChangedCallback;
    void OnObjectPropertyChanged(Object^ sender, PropertyChangedEventArgs^ e)
    {
        T object = safe_cast<T>(sender);
        ObjectPropertyChanged(this, object, e);
        if (e->PropertyName == "Name") // influence ToString
            CollectionChanged(this, gcnew NotifyCollectionChangedEventArgs(
                NotifyCollectionChangedAction::Replace, object, object, InnerContainer->IndexOf(object)));
    }
internal:
    ObservableProxyContainer() 
    { 
        InnerContainer = gcnew List<T>(0);
        ObjectPropertyChangedCallback = gcnew PropertyChangedEventHandler(this, &ObservableProxyContainer::OnObjectPropertyChanged);
    }
    event AddObjectEventHandler<T>^ BeforeAddObject;
    event ObjectPropertyChangedEventHandler<T>^ ObjectPropertyChanged;
    void InnerAdd(T object)
    {
        object->PropertyChanged += ObjectPropertyChangedCallback;
        InnerContainer->Add(object);
        CollectionChanged(this, gcnew NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction::Add, object, InnerContainer->Count - 1));
    }
    void InnerDel(T object)
    {
        object->PropertyChanged -= ObjectPropertyChangedCallback;
        const auto idx = InnerContainer->IndexOf(object);
        if (idx == -1) return;
        InnerContainer->RemoveAt(idx);
        CollectionChanged(this, gcnew NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction::Remove, object, idx));
    }
public:
    virtual event NotifyCollectionChangedEventHandler^ CollectionChanged;
    virtual IEnumerator<T>^ GetEnumerator()
    { 
        return InnerContainer->GetEnumerator();
    }
    virtual System::Collections::IEnumerator^ GetEnumerator2() = System::Collections::IEnumerable::GetEnumerator
    {
        return InnerContainer->GetEnumerator();
    }
    property T default[int32_t]
    {
        T get(int32_t i)
        {
            if (i < 0 || i >= InnerContainer->Count)
                return T();
            else
                return InnerContainer[i];
        }
    }
    property int32_t Count
    {
        int32_t get()
        {
            return InnerContainer->Count;
        }
    }
    void Add(T object)
    {
        bool shouldAdd = false;
        BeforeAddObject(this, object, shouldAdd);
        if (shouldAdd)
            InnerAdd(object);
    }
};
//
//public abstract class ObservableContainer<T> : BaseViewModel, IEnumerable<T>, INotifyCollectionChanged
//{
//    protected static NotifyCollectionChangedEventArgs RefeshEventArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset);
//    public event NotifyCollectionChangedEventHandler CollectionChanged;
//    protected void OnCollectionChanged()
//    {
//        CollectionChanged ? .Invoke(this, RefeshEventArgs);
//    }
//    protected void OnCollectionChanged(NotifyCollectionChangedEventArgs e)
//    {
//        CollectionChanged ? .Invoke(this, e);
//    }
//
//    protected abstract IEnumerator<T> InnerGetEnumerator();
//
//    public IEnumerator<T> GetEnumerator() = > InnerGetEnumerator();
//
//    IEnumerator IEnumerable.GetEnumerator() = > InnerGetEnumerator();
//}


}

