using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
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
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
            //var handler = PropertyChanged;
            //if (handler == null)
            //    return;
            //var arg = new PropertyChangedEventArgs(propertyName);
            //if (SynchronizationContext.Current == SyncContext)
            //    handler(this, arg);
            //else
            //    SyncContext.Post(x => handler(this, arg), null);
        }
    }

    public abstract class ObservableContainer<T> : BaseViewModel, IEnumerable<T>, INotifyCollectionChanged
    {
        protected static NotifyCollectionChangedEventArgs RefeshEventArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset);
        public event NotifyCollectionChangedEventHandler CollectionChanged;
        protected void OnCollectionChanged()
        {
            CollectionChanged?.Invoke(this, RefeshEventArgs);
        }
        protected void OnCollectionChanged(NotifyCollectionChangedEventArgs e)
        {
            CollectionChanged?.Invoke(this, e);
        }

        protected abstract IEnumerator<T> InnerGetEnumerator();

        public IEnumerator<T> GetEnumerator() => InnerGetEnumerator();

        IEnumerator IEnumerable.GetEnumerator() => InnerGetEnumerator();
    }

    public class ObservableList<T> : ObservableContainer<T>
    {
        protected readonly IList<T> Src;

        protected ObservableList(IList<T> list) { Src = list; }

        protected void OnItemContentChanged(T item)
        {
            var arg = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Replace, item, item, Src.IndexOf(item));
            OnCollectionChanged(arg);
        }

        protected override IEnumerator<T> InnerGetEnumerator() => Src.GetEnumerator();
    }

    public abstract class ObservableList2<T> : ObservableContainer<T>
    {
        protected abstract int GetSize();
        protected abstract T GetElement(int idx);

        protected override IEnumerator<T> InnerGetEnumerator()
        {
            for (int i = 0, size = GetSize(); i < size; ++i)
                yield return GetElement(i);
        }
    }

    public class ObservableList3<T> : ObservableContainer<T>
    {
        protected readonly ObservableCollection<T> Src;

        protected ObservableList3(ObservableCollection<T> list) { Src = list; Src.CollectionChanged += (o, e) => OnCollectionChanged(e); }

        protected void OnItemContentChanged(T item)
        {
            var arg = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Replace, item, item, Src.IndexOf(item));
            OnCollectionChanged(arg);
        }

        protected override IEnumerator<T> InnerGetEnumerator() => Src.GetEnumerator();
    }

    public class ObservableSelection<T> : ObservableContainer<T>
    {
        public delegate void NotifySelectionChangedEventHandler(object sender);
        public event NotifySelectionChangedEventHandler SelectionChanged;
        public Dictionary<T, bool> Src
        {
            get;
            private set;
        } = new Dictionary<T, bool>();

        public ObservableSelection() { }

        protected override IEnumerator<T> InnerGetEnumerator()=> Src.Keys.GetEnumerator();

        public bool Add(T item, bool isSelected)
        {
            if (Src.ContainsKey(item))
                return false;
            Src.Add(item, isSelected);
            OnCollectionChanged();
            SelectionChanged?.Invoke(this);
            return true;
        }
        public bool Remove(T item)
        {
            var value = Src.Remove(item);
            OnCollectionChanged();
            if (value)
                SelectionChanged?.Invoke(this);
            return true;
        }
        public void RefreshSelection(IEnumerable<T> selection)
        {
            var selected = new HashSet<T>(selection);
            Src = Src.ToDictionary(x => x.Key, x => selected.Contains(x.Key));
            SelectionChanged?.Invoke(this);
        }
        public bool Contains(T item)
        {
            return Src.ContainsKey(item);
        }
        public bool Contains(T item, bool isSelected)
        {
            if (Src.TryGetValue(item, out var value))
                return value == isSelected;
            return false;
        }
    }

    public class ObservableHashSet<T> : ObservableCollection<T>
    {
        protected override void InsertItem(int index, T item)
        {
            if (!Contains(item))
                base.InsertItem(index, item);
        }

        protected override void SetItem(int index, T item)
        {
            int i = IndexOf(item);
            if (i < 0 || i == index)
                base.SetItem(index, item);
        }
    }

}
