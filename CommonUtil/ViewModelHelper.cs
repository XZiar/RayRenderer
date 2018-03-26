using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;

namespace XZiar.Util
{
    public abstract class ObservableList<T> : IEnumerable<T>, INotifyCollectionChanged, INotifyPropertyChanged
    {
        protected static NotifyCollectionChangedEventArgs RefeshEventArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset);
        public event NotifyCollectionChangedEventHandler CollectionChanged;
        public event PropertyChangedEventHandler PropertyChanged;

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
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
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
}
