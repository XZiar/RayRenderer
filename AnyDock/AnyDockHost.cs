using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Markup;

namespace AnyDock
{
    [ContentProperty(nameof(Dummy))]
    class AnyDockHost : Panel
    {
        public ObservableCollection<UIElement> Dummy { get; } = new ObservableCollection<UIElement>();
        public AnyDockHost()
        {
            
        }
    }
}
