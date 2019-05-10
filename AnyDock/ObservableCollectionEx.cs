using System;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Collections;

namespace AnyDock
{
    public class ObservableCollectionEx<T> : ObservableCollection<T>
    {
        protected override void ClearItems()
        {
            //var arg = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Remove);
            foreach (var ele in Items)
            {
                OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Remove, ele));
            }
            base.ClearItems();
        }
    }
}
