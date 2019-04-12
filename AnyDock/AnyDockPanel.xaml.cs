using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using XZiar.Util;

namespace AnyDock
{
    /// <summary>
    /// DockPane.xaml 的交互逻辑
    /// </summary>
    [ContentProperty(nameof(Children))]
    public partial class AnyDockPanel : ContentControl
    {
        public static IValueConverter EnableHitConvertor = new BindingHelper.OneWayValueConvertor(o => (Visibility)o == Visibility.Collapsed);

        public static readonly DependencyProperty PageNameProperty = DependencyProperty.RegisterAttached(
            "PageName",
            typeof(string),
            typeof(AnyDockPanel),
            new FrameworkPropertyMetadata("", FrameworkPropertyMetadataOptions.AffectsRender));
        public static string GetPageName(UIElement element)
        {
            return element.GetValue(PageNameProperty) as string;
        }
        public static void SetPageName(UIElement element, string value)
        {
            element.SetValue(PageNameProperty, value);
        }
        private static readonly DependencyProperty ParentDockProperty = DependencyProperty.RegisterAttached(
            "ParentDock",
            typeof(AnyDockPanel),
            typeof(AnyDockPanel),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsRender));
        private static void SetParentDock(UIElement element, AnyDockPanel value)
        {
            element.SetValue(ParentDockProperty, value);
        }
        private static AnyDockPanel GetParentDock(UIElement element)
        {
            return element.GetValue(ParentDockProperty) as AnyDockPanel;
        }

        public ObservableCollection<FrameworkElement> Children
        {
            get;
            private set;
        } = new ObservableCollection<FrameworkElement>();

        private AnyDockPanel group1, group2;
        public AnyDockPanel Group1
        {
            get { return group1; }
            set { group1 = value; OnSetGroup(Group1); }
        }
        public AnyDockPanel Group2
        {
            get { return group2; }
            set { group2 = value; OnSetGroup(Group2); }
        }
        private void OnSetGroup(AnyDockPanel group)
        {
            if (group != null)
                group.ParentPanel = this;
            if (ShouldRefresh)
            {
                RefreshState();
                RefreshRealLayout();
            }
        }
        private AnyDockPanel ParentPanel = null;

        private enum DockStates { Tab, Group, Abandon };
        private DockStates State = DockStates.Tab;
        private Orientation PanelOrientation = Orientation.Horizontal;

        public AnyDockPanel()
        {
            Children.CollectionChanged += new NotifyCollectionChangedEventHandler(OnChildrenChanged);
            Loaded += OnLoaded;
            InitializeComponent();
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            ShouldRefresh = true;
            RefreshState();
            RefreshRealLayout();
        }

        private bool ShouldRefresh = false;
        private void OnChildrenChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.NewItems != null)
            {
                foreach (var x in e.NewItems.Cast<object>())
                {
                    if (!(x is UIElement ele))
                        throw new InvalidOperationException("Only UIElement can be added");
                    if (ele is AnyDockPanel)
                        throw new InvalidOperationException("DockPanel should not be children!");
                    SetParentDock(ele, this);
                }
            }
            if (ShouldRefresh)
            {
                RefreshState();
                RefreshRealLayout();
            }
        }
        private void MoveChildrenFrom(AnyDockPanel panel)
        {
            ShouldRefresh = false;
            Children.Clear();
            foreach (var ele in panel.Children)
                Children.Add(ele);
            ShouldRefresh = true;
            panel.ShouldRefresh = false;
            panel.Children.Clear();
            panel.ShouldRefresh = true;
        }

        public void SetGroups(AnyDockPanel panel1, AnyDockPanel panel2, Orientation orientation = Orientation.Horizontal)
        {
            ShouldRefresh = false;
            if (group1 != null)
                group1.State = DockStates.Abandon;
            if (group2 != null)
                group2.State = DockStates.Abandon;
            Group1 = panel1;
            Group2 = panel2;
            ShouldRefresh = true;
            PanelOrientation = orientation;
            RefreshState();
            RefreshRealLayout();
        }

        private void RefreshState()
        {
            if (Group1 != null && Group2 != null)
            {
                State = DockStates.Group;
                if (Children.Count > 0)
                    throw new InvalidOperationException("AnyDockPanel should not has children when group occupied!");
            }
            else if (State != DockStates.Abandon)
            {
                if (Group1 != null || Group2 != null)
                    throw new InvalidOperationException("Both group need to have element!");
                if (Children.Count == 0 && ParentPanel != null) // Children just get removed
                {
                    ParentPanel.RequestRemove(this);
                    State = DockStates.Abandon;
                }
                else
                {
                    State = DockStates.Tab;
                }
            }
        }
        private void RefreshRealLayout()
        {
            grid.Children.Remove(Group1);
            grid.Children.Remove(Group2);
            switch (State)
            {
            case DockStates.Group:
                MainTab.Visibility = Visibility.Collapsed;
                Splitter.Visibility = Visibility.Visible;
                if (PanelOrientation == Orientation.Horizontal)
                {
                    Grid.SetRow(Group1, 0); Grid.SetRowSpan(Group1, 3); Grid.SetColumn(Group1, 0); Grid.SetColumnSpan(Group1, 1);
                    Grid.SetRow(Group2, 0); Grid.SetRowSpan(Group2, 3); Grid.SetColumn(Group2, 2); Grid.SetColumnSpan(Group2, 1);
                    Grid.SetRow(Splitter, 0); Grid.SetRowSpan(Splitter, 3); Grid.SetColumn(Splitter, 1); Grid.SetColumnSpan(Splitter, 1);
                    Splitter.HorizontalAlignment = HorizontalAlignment.Stretch;
                }
                else
                {
                    Grid.SetRow(Group1, 0); Grid.SetRowSpan(Group1, 1); Grid.SetColumn(Group1, 0); Grid.SetColumnSpan(Group1, 3);
                    Grid.SetRow(Group2, 2); Grid.SetRowSpan(Group2, 1); Grid.SetColumn(Group2, 0); Grid.SetColumnSpan(Group2, 3);
                    Grid.SetRow(Splitter, 1); Grid.SetRowSpan(Splitter, 1); Grid.SetColumn(Splitter, 0); Grid.SetColumnSpan(Splitter, 3);
                    Splitter.VerticalAlignment = VerticalAlignment.Stretch;
                }
                grid.Children.Add(Group1);
                grid.Children.Add(Group2);
                break;
            case DockStates.Tab:
                Splitter.Visibility = Visibility.Collapsed;
                MainTab.Visibility = Visibility.Visible;
                break;
            }

        }

        private class DragInfo { public Point StarPoint; public bool IsPending = false; }
        private static readonly ConditionalWeakTable<TabItem, DragInfo> DragStartPoints = new ConditionalWeakTable<TabItem, DragInfo>();
        private class DragData
        {
            internal readonly ModifierKeys Keys;
            internal readonly TabItem Item;
            internal readonly FrameworkElement Element;
            internal readonly AnyDockPanel Panel;
            internal DragData(TabItem item)
            {
                Keys = Keyboard.Modifiers;
                Item = item;
                Element = item.Content as FrameworkElement;
                Panel = GetParentDock(Element);
            }
        }
        private void TabItemMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            var info = DragStartPoints.GetOrCreateValue((TabItem)sender);
            info.StarPoint = e.GetPosition(null); info.IsPending = true;
        }
        private void TabItemMouseLeave(object sender, MouseEventArgs e)
        {
            if (e.LeftButton != MouseButtonState.Pressed)
                return;
            var item = (TabItem)sender;
            if (!DragStartPoints.TryGetValue(item, out DragInfo info) || !info.IsPending)
                return;
            BeginTabItemDrag(item);
            e.Handled = true;
        }
        private void TabItemMouseMove(object sender, MouseEventArgs e)
        {
            if (e.LeftButton != MouseButtonState.Pressed)
                return;
            var item = (TabItem)sender;
            if (!DragStartPoints.TryGetValue(item, out DragInfo info) || !info.IsPending)
                return;
            var diff = e.GetPosition(null) - info.StarPoint;
            if (Math.Abs(diff.X) <= SystemParameters.MinimumHorizontalDragDistance ||
                Math.Abs(diff.Y) <= SystemParameters.MinimumVerticalDragDistance)
                return;
            BeginTabItemDrag(item);
            e.Handled = true;
        }
        private static void BeginTabItemDrag(TabItem item)
        {
            if (!DragStartPoints.TryGetValue(item, out DragInfo info))
                throw new InvalidOperationException("DragInfo should be already created.");
            info.IsPending = false;
            DragDrop.DoDragDrop(item, new DragData(item), DragDropEffects.Move);
        }

        private void TabItemDragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(typeof(DragData)) && sender == e.Source)
            {
                e.Effects = DragDropEffects.Move;
                e.Handled = true;
            }
            else
                e.Effects = DragDropEffects.None;
        }
        private void TabItemDrop(object sender, DragEventArgs e)
        {
            if (!e.Data.GetDataPresent(typeof(DragData)))
                return;
            var src = (DragData)e.Data.GetData(typeof(DragData));
            var dst = new DragData((TabItem)sender);
            if (src.Panel == null || dst.Panel == null)
                throw new InvalidOperationException("Drag objects should belong to AnyDock");
            if (src.Panel == dst.Panel)
            {
                if (src.Item == dst.Item)
                    return;
                // exchange order only
                int srcIdx = src.Panel.Children.IndexOf(src.Element);
                int dstIdx = dst.Panel.Children.IndexOf(dst.Element);
                src.Panel.Children.Move(srcIdx, dstIdx);
            }
            else
            {
                if (src.Item == dst.Item)
                    throw new InvalidOperationException("Should not be the same TabItem");
                // move item
                src.Panel.Children.Remove(src.Element);
                var dstPanel = dst.Panel.State == DockStates.Abandon ? dst.Panel.ParentPanel : dst.Panel; // in case collapsed
                int dstIdx = dstPanel.Children.IndexOf(dst.Element);
                dstPanel.Children.Insert(dstIdx, src.Element);
                dstPanel.MainTab.SelectedIndex = dstIdx;
            }
            e.Handled = true;
        }

        private void RequestRemove(AnyDockPanel panel)
        {
            if (State != DockStates.Group)
                throw new InvalidOperationException("Won't hold a panel when not in GROUP mode");
            AnyDockPanel remain;
            if (Group1 == panel)
                remain = Group2;
            else if (Group2 == panel)
                remain = Group1;
            else
                throw new InvalidOperationException("Doesn't hold the panel");
            if (remain.State == DockStates.Group)
                remain.grid.Children.Clear();
            else
                MoveChildrenFrom(remain);
            ShouldRefresh = false;
            Group1 = remain.Group1;
            Group2 = remain.Group2;
            PanelOrientation = remain.PanelOrientation;
            ShouldRefresh = true;
            remain.State = DockStates.Abandon;
            RefreshState();
            RefreshRealLayout();
        }

        private void TabCoreDragEnter(object sender, DragEventArgs e)
        {
            if (!e.Data.GetDataPresent(typeof(DragData)))
            {
                e.Effects = DragDropEffects.None;
                return;
            }
            Console.WriteLine($"Enter {sender.GetType()}");
            e.Effects = DragDropEffects.Move;
            DragOverLay.Height = (sender as ContentControl).ActualHeight;
            DragOverLay.Visibility = Visibility.Visible;
            e.Handled = true;
        }

        private void TabCoreDragLeave(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(typeof(DragData)))
            {
                Console.WriteLine($"Leave {sender.GetType()}");
                DragOverLay.Visibility = Visibility.Collapsed;
                e.Handled = true;
            }
        }

        private void TabCoreDrop(object sender, DragEventArgs e)
        {
            if (!e.Data.GetDataPresent(typeof(DragData)))
                return;
            Console.WriteLine($"Drop  {sender.GetType()}");
            var e2 = VisualTreeHelper.HitTest(DragOverLay, e.GetPosition(DragOverLay));
            DragOverLay.Visibility = Visibility.Collapsed;
            e.Handled = true;
        }

    }
}
