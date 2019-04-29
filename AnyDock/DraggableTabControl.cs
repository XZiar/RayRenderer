using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Markup;
using XZiar.Util;

namespace AnyDock
{
    
    internal class SimpleTabItem : TabItem
    {
        public SimpleTabItem()
        {
            DataContextChanged += OnDataContextChanged;
        }
        public DraggableTabControl TabControlParent => ItemsControl.ItemsControlFromItemContainer(this) as DraggableTabControl;
        private void OnDataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            Header = e.NewValue;
        }
    }

    [ContentProperty(nameof(RealChildren))]
    public class DraggableTabControl : TabControl
    {
        private static readonly ResourceDictionary ResDict;
        private static readonly ControlTemplate DraggableTabControlTemplate;
        public static readonly DependencyProperty AllowDropTabProperty = DependencyProperty.Register(
            "AllowDropTab",
            typeof(bool),
            typeof(DraggableTabControl),
            new FrameworkPropertyMetadata(true, FrameworkPropertyMetadataOptions.None));
        public bool AllowDropTab
        {
            get => (bool)GetValue(AllowDropTabProperty);
            set => SetValue(AllowDropTabProperty, value);
        }
        static DraggableTabControl()
        {
            ResDict = new ResourceDictionary { Source = new Uri("AnyDock;component/DraggableTabControl.res.xaml", UriKind.RelativeOrAbsolute) };
            DraggableTabControlTemplate = (ControlTemplate)ResDict["DraggableTabControlTemplate"];
        }
        
        public DraggableTabControl()
        {
            Template = DraggableTabControlTemplate;
            ApplyTemplate();
            RealChildren.CollectionChanged += OnChildrenChanged;
        }
        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();
            TabPanel = (DraggableTabPanel)Template.FindName("TabPanel", this);
        }
        protected override DependencyObject GetContainerForItemOverride()
        {
            return new SimpleTabItem();
        }
        internal void ReorderItem(UIElement obj, UIElement dst)
        {
            var target = (Items.SourceCollection as ObservableCollection<UIElement>);
            // exchange order only
            int srcIdx = target.IndexOf(obj);
            int dstIdx = target.IndexOf(dst);
            if (srcIdx != dstIdx)
                target.Move(srcIdx, dstIdx);
        }

        internal ObservableCollectionEx<UIElement> RealChildren { get; } = new ObservableCollectionEx<UIElement>();
        private DraggableTabPanel TabPanel;
        private void OnChildrenChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (ItemsSource != sender)
                ItemsSource = sender as ObservableCollectionEx<UIElement>;
        }

    }
}
