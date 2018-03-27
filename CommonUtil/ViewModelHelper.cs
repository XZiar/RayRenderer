using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Threading;

namespace XZiar.Util
{
    public class BaseViewModel : INotifyPropertyChanged
    {
        protected static SynchronizationContext SyncContext = SynchronizationContext.Current;
        public static void Init() { }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            var handler = PropertyChanged;
            if (handler == null)
                return;
            var arg = new PropertyChangedEventArgs(propertyName);
            if (SynchronizationContext.Current == SyncContext)
                handler(this, arg);
            else
                SyncContext.Post(x => handler(this, arg), null);
        }
    }

    public abstract class ObservableList2<T> : BaseViewModel, IEnumerable<T>, INotifyCollectionChanged
    {
        protected static NotifyCollectionChangedEventArgs RefeshEventArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset);
        public event NotifyCollectionChangedEventHandler CollectionChanged;

        protected abstract int GetSize();
        protected abstract T GetElement(int idx);
        protected void OnCollectionChanged()
        {
            CollectionChanged(this, RefeshEventArgs);
        }
        protected void OnCollectionChanged(NotifyCollectionChangedEventArgs e)
        {
            CollectionChanged(this, e);
        }

        public IEnumerator<T> GetEnumerator()
        {
            for (int i = 0, size = GetSize(); i < size; ++i)
                yield return GetElement(i);
        }
        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
    }

    public class ObservableList<T> : BaseViewModel, IEnumerable<T>, INotifyCollectionChanged
    {
        protected static NotifyCollectionChangedEventArgs RefeshEventArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset);
        public event NotifyCollectionChangedEventHandler CollectionChanged;
        protected readonly IList<T> Src;

        protected ObservableList(IList<T> list) { Src = list; }

        protected void OnCollectionChanged()
        {
            CollectionChanged(this, RefeshEventArgs);
        }
        protected void OnCollectionChanged(NotifyCollectionChangedEventArgs e)
        {
            CollectionChanged(this, e);
        }
        protected void OnItemContentChanged(T item)
        {
            var arg = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Replace, item, item, Src.IndexOf(item));
            CollectionChanged(this, arg);
        }

        public IEnumerator<T> GetEnumerator()
        {
            return Src.GetEnumerator();
        }
        IEnumerator IEnumerable.GetEnumerator()
        {
            return Src.GetEnumerator();
        }
    }

    public class ObservableList3<T> : BaseViewModel, IEnumerable<T>, INotifyCollectionChanged
    {
        protected static NotifyCollectionChangedEventArgs RefeshEventArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset);
        public event NotifyCollectionChangedEventHandler CollectionChanged;
        protected readonly ObservableCollection<T> Src;

        protected ObservableList3(ObservableCollection<T> list) { Src = list; Src.CollectionChanged += (o, e) => OnCollectionChanged(e); }

        protected void OnCollectionChanged()
        {
            CollectionChanged(this, RefeshEventArgs);
        }
        protected void OnCollectionChanged(NotifyCollectionChangedEventArgs e)
        {
            CollectionChanged(this, e);
        }
        protected void OnItemContentChanged(T item)
        {
            var arg = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Replace, item, item, Src.IndexOf(item));
            CollectionChanged(this, arg);
        }

        public IEnumerator<T> GetEnumerator()
        {
            return Src.GetEnumerator();
        }
        IEnumerator IEnumerable.GetEnumerator()
        {
            return Src.GetEnumerator();
        }
    }
}
