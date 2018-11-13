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
private delegate void DelObjectEventHandler(Object^ sender, T obj, bool% shouldDel);
generic<typename T>
public delegate void ObjectPropertyChangedEventHandler(Object^ sender, T obj, PropertyChangedEventArgs^ e);

generic<typename T> where T : INotifyPropertyChanged
public ref class ObservableProxyReadonlyContainer : public INotifyCollectionChanged, public IReadOnlyList<T>
{
protected:
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
    ObservableProxyReadonlyContainer()
    {
        InnerContainer = gcnew List<T>(0);
        ObjectPropertyChangedCallback = gcnew PropertyChangedEventHandler(this, &ObservableProxyReadonlyContainer::OnObjectPropertyChanged);
    }
    void InnerAdd(T item)
    {
        item->PropertyChanged += ObjectPropertyChangedCallback;
        InnerContainer->Add(item);
        CollectionChanged(this, gcnew NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction::Add, item, InnerContainer->Count - 1));
    }
    void InnerInsert(int index, T item)
    {
        item->PropertyChanged += ObjectPropertyChangedCallback;
        InnerContainer->Insert(index, item);
        CollectionChanged(this, gcnew NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction::Add, item, index));
    }
    bool InnerDel(T object)
    {
        object->PropertyChanged -= ObjectPropertyChangedCallback;
        const auto idx = InnerContainer->IndexOf(object);
        if (idx == -1)
            return false;
        InnerContainer->RemoveAt(idx);
        CollectionChanged(this, gcnew NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction::Remove, object, idx));
        return true;
    }
    void InnerClear()
    {
        for each (T item in InnerContainer)
        {
            item->PropertyChanged -= ObjectPropertyChangedCallback;
        }
        CollectionChanged(this, gcnew NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction::Remove, gcnew List<T>(InnerContainer)));
        InnerContainer->Clear();
    }
public:
    event ObjectPropertyChangedEventHandler<T>^ ObjectPropertyChanged;
    virtual event NotifyCollectionChangedEventHandler^ CollectionChanged;
    virtual IEnumerator<T>^ GetEnumerator()
    {
        return InnerContainer->GetEnumerator();
    }
    virtual System::Collections::IEnumerator^ GetEnumerator2() = System::Collections::IEnumerable::GetEnumerator
    {
        return InnerContainer->GetEnumerator();
    }
    virtual property T default[int32_t]
    {
        T get(int32_t i)
        {
            if (i < 0 || i >= InnerContainer->Count)
                return T();
            else
                return InnerContainer[i];
        }
    }

    virtual property int Count { int get() { return InnerContainer->Count; } }
    virtual property bool IsReadOnly { bool get() { return true; } }

    virtual int32_t IndexOf(T object)
    {
        return InnerContainer->IndexOf(object);
    }
    virtual bool Contains(T item)
    {
        return InnerContainer->Contains(item);
    }
    virtual void CopyTo(array<T, 1> ^array, int arrayIndex)
    {
        InnerContainer->CopyTo(array, arrayIndex);
    }
};

generic<typename T> where T : INotifyPropertyChanged
public ref class ObservableProxyContainer : public ObservableProxyReadonlyContainer<T>, public IList<T>
{
protected:
internal:
    event AddObjectEventHandler<T>^ BeforeAddObject;
    event DelObjectEventHandler<T>^ BeforeDelObject;
    ObservableProxyContainer() 
    { }
public:
    virtual property T default[int32_t]
    {
        void set(int32_t i, T item)
        {
            throw gcnew System::NotImplementedException();
        }
    }

    virtual property bool IsReadOnly { bool get() override { return false; } }
    
    virtual void Add(T item)
    {
        bool shouldAdd = false;
        BeforeAddObject(this, item, shouldAdd);
        if (shouldAdd)
            InnerAdd(item);
    }
    virtual void Clear()
    {
        ObservableProxyReadonlyContainer<T>::InnerClear();
    }
    virtual bool Remove(T item)
    {
        bool shouldDel = false;
        BeforeDelObject(this, item, shouldDel);
        if (shouldDel)
            return InnerDel(item);
        return false;
    }
    virtual void Insert(int index, T item)
    {
        bool shouldAdd = false;
        BeforeAddObject(this, item, shouldAdd);
        if (shouldAdd)
            InnerInsert(index, item);
    }
    virtual void RemoveAt(int index)
    {
        bool shouldDel = false;
        auto item = InnerContainer[index];
        BeforeDelObject(this, item, shouldDel);
        if (shouldDel)
            InnerDel(item);
    }
};


}

