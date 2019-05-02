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
        private static readonly ControlTemplate SimpleTabItemTemplate;
        static SimpleTabItem()
        {
            SimpleTabItemTemplate = (ControlTemplate)DraggableTabControl.ResDict["SimpleTabItemTemplate"];
        }
        public SimpleTabItem()
        {
            Template = SimpleTabItemTemplate;
        }
        public DraggableTabControl TabControlParent => ItemsControl.ItemsControlFromItemContainer(this) as DraggableTabControl;

    }

    [ContentProperty(nameof(RealChildren))]
    public class DraggableTabControl : TabControl
    {
        internal static readonly ResourceDictionary ResDict;
        private static readonly ControlTemplate DraggableTabControlTemplate;
        private static readonly MoreTabsPopup MoreTabsPopup;

        public static readonly DependencyProperty ShowAllTabsProperty = DependencyProperty.Register(
            "ShowAllTabs",
            typeof(bool),
            typeof(DraggableTabControl),
            new FrameworkPropertyMetadata(false, FrameworkPropertyMetadataOptions.AffectsMeasure));
        public bool ShowAllTabs
        {
            get => (bool)GetValue(ShowAllTabsProperty);
            set => SetValue(ShowAllTabsProperty, value);
        }
        public static readonly DependencyProperty ShowCloseButtonProperty = DependencyProperty.Register(
            "ShowCloseButton",
            typeof(bool),
            typeof(DraggableTabControl),
            new FrameworkPropertyMetadata(true, FrameworkPropertyMetadataOptions.AffectsMeasure));
        public bool ShowCloseButton
        {
            get => (bool)GetValue(ShowCloseButtonProperty);
            set => SetValue(ShowCloseButtonProperty, value);
        }
        public static readonly DependencyProperty ShowIconProperty = DependencyProperty.Register(
            "ShowIcon",
            typeof(bool),
            typeof(DraggableTabControl),
            new FrameworkPropertyMetadata(true, FrameworkPropertyMetadataOptions.AffectsMeasure));
        public bool ShowIcon
        {
            get => (bool)GetValue(ShowIconProperty);
            set => SetValue(ShowIconProperty, value);
        }

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
            DraggableTabControlTemplate = (ControlTemplate)     ResDict["DraggableTabControlTemplate"];
            MoreTabsPopup = new MoreTabsPopup();
    }
        
        public DraggableTabControl()
        {
            Template = DraggableTabControlTemplate;
            ApplyTemplate();
            //CollectionViewSource.GetDefaultView(Items).CollectionChanged += OnChildrenChanged;
            RealChildren.CollectionChanged += OnRealChildrenChanged;
        }

        public override void OnApplyTemplate()
        {
            if (MoreTabDropButton != null)
                MoreTabDropButton.Click -= MoreTabDropButtonClickAsync;
            base.OnApplyTemplate();
            MoreTabDropButton = (Button)Template.FindName("btnMoreTabDrop", this);
            MoreTabDropButton.Click += MoreTabDropButtonClickAsync;
            TabPanel = (DraggableTabPanel)Template.FindName("headerPanel", this);
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

        private async void MoreTabDropButtonClickAsync(object sender, RoutedEventArgs e)
        {
            var newItem = await MoreTabsPopup.ShowTabs(sender as Button, Items.SourceCollection);
            if (newItem != null)
                SelectedItem = newItem;
        }

        internal ObservableCollectionEx<UIElement> RealChildren { get; } = new ObservableCollectionEx<UIElement>();
        private DraggableTabPanel TabPanel;
        private Button MoreTabDropButton;
        private void OnRealChildrenChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (ItemsSource != sender)
                ItemsSource = sender as ObservableCollectionEx<UIElement>;
            foreach (var x in e.DeledItems<UIElement>())
                AnyDockManager.RemoveClosedHandler(x, OnTabClosed);
            foreach (var x in e.AddedItems<UIElement>())
                AnyDockManager.AddClosedHandler(x, OnTabClosed);
        }

        private void OnTabClosed(UIElement sender, AnyDockManager.TabCloseEventArgs args)
        {
            RealChildren.Remove(args.TargetElement);
        }

    }
}
