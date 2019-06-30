using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace XZiar.Util.Collections
{
    public static class NotifyCollectionChangedEventArgsEx
    {
        private delegate void InitRemoveDelegate(NotifyCollectionChangedEventArgs arg, NotifyCollectionChangedAction action, IList oldItems, int oldStartingIndex);
        private static readonly InitRemoveDelegate ResetInitializer;
        static NotifyCollectionChangedEventArgsEx()
        {
            ResetInitializer = (InitRemoveDelegate)Delegate.CreateDelegate(typeof(InitRemoveDelegate), 
                typeof(NotifyCollectionChangedEventArgs).GetMethod("InitializeRemove", BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public));
        }
        public static NotifyCollectionChangedEventArgs NewResetEvent(IList list)
        {
            var arg = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset);
            ResetInitializer(arg, NotifyCollectionChangedAction.Reset, list, 0);
            return arg;
        }
    }

    public class ObservableSet<T> : ISet<T>, INotifyCollectionChanged
    {
        private readonly ISet<T> Source;
        private readonly List<T> IndexList = new List<T>();

        public event NotifyCollectionChangedEventHandler CollectionChanged;

        public ObservableSet() : this(new HashSet<T>()) { }

        public ObservableSet(ISet<T> source)
        {
            Source = source;
        }

        #region ISet<T> immutable mapping

        public int Count => Source.Count;

        public bool IsReadOnly => Source.IsReadOnly;

        public bool Contains(T item) => Source.Contains(item);

        public void CopyTo(T[] array, int arrayIndex) => Source.CopyTo(array, arrayIndex);

        public IEnumerator<T> GetEnumerator() => Source.GetEnumerator();

        IEnumerator IEnumerable.GetEnumerator() => Source.GetEnumerator();

        public bool IsProperSubsetOf(IEnumerable<T> other) => Source.IsProperSubsetOf(other);

        public bool IsProperSupersetOf(IEnumerable<T> other) => Source.IsProperSupersetOf(other);

        public bool IsSubsetOf(IEnumerable<T> other) => Source.IsSubsetOf(other);

        public bool IsSupersetOf(IEnumerable<T> other) => Source.IsSupersetOf(other);

        public bool Overlaps(IEnumerable<T> other) => Source.Overlaps(other);

        public bool SetEquals(IEnumerable<T> other) => Source.SetEquals(other);

        void ICollection<T>.Add(T item) => Add(item);

        #endregion ISet<T> immutable mapping

        #region INotifyCollectionChanged support
        protected virtual void OnCollectionChanged(NotifyCollectionChangedEventArgs e)
        {
            lock (CollectionChanged)
            {
                CollectionChanged?.Invoke(this, e);
            }
        }
        private void ClearAndNotify()
        {
            OnCollectionChanged(NotifyCollectionChangedEventArgsEx.NewResetEvent(IndexList));
            IndexList.Clear();
        }
        private void AddAllAndNotify()
        {
            IndexList.AddRange(Source);
            OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Add, IndexList));
        }
        #endregion INotifyCollectionChanged support

        #region ISet<T> mutable mapping

        public bool Add(T item)
        {
            lock (Source)
            {
                if (Source.Add(item))
                {
                    IndexList.Add(item);
                    OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Add, item, Source.Count - 1));
                    return true;
                }
            }
            return false;
        }

        public void Clear()
        {
            lock (Source)
            {
                Source.Clear();
                ClearAndNotify();
            }
        }

        public void ExceptWith(IEnumerable<T> other)
        {
            lock (Source)
            {
                var oldcnt = Source.Count;
                Source.ExceptWith(other);
                if (Source.Count != oldcnt)
                {
                    ClearAndNotify();
                    AddAllAndNotify();
                }
            }
        }

        public void IntersectWith(IEnumerable<T> other)
        {
            lock (Source)
            {
                var oldcnt = Source.Count;
                Source.IntersectWith(other);
                if (Source.Count != oldcnt)
                {
                    ClearAndNotify();
                    AddAllAndNotify();
                }
            }
        }

        public bool Remove(T item)
        {
            if (Source.Remove(item))
            {
                var idx = IndexList.IndexOf(item);
                IndexList.RemoveAt(idx);
                OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Remove, item, idx));
                return true;
            }
            return false;
        }

        public void SymmetricExceptWith(IEnumerable<T> other)
        {
            lock (Source)
            {
                Source.SymmetricExceptWith(other);
                ClearAndNotify();
                AddAllAndNotify();
            }
        }

        public void UnionWith(IEnumerable<T> other)
        {
            lock (Source)
            {
                var oldcnt = Source.Count;
                Source.UnionWith(other);
                if (Source.Count != oldcnt)
                {
                    ClearAndNotify();
                    AddAllAndNotify();
                }
            }
        }

        #endregion ISet<T> mutable mapping
    }
}
