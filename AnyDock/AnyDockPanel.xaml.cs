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
            set { OnSetGroup(ref group1, value); }
        }
        public AnyDockPanel Group2
        {
            get { return group2; }
            set { OnSetGroup(ref group2, value); }
        }
        private void OnSetGroup(ref AnyDockPanel dst, AnyDockPanel group)
        {
            if (dst != null)
                grid.Children.Remove(dst);
            if (group != null)
            {
                group.ParentPanel = this;
                group.Visibility = Visibility.Collapsed;
                grid.Children.Add(group);
            }
            dst = group;
            if (ShouldRefresh)
                RefreshState();
        }
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
            if (e.Action == NotifyCollectionChangedAction.Remove || e.Action == NotifyCollectionChangedAction.Reset)
            {
                if (e.OldItems != null)
                    foreach (var x in e.OldItems.Cast<UIElement>())
                        SetParentDock(x, null);
                if (Children.Count == 0 && ShouldRefresh)
                    RefreshState();
            }
        }
        private AnyDockPanel ParentPanel = null;

        private enum DockStates { Tab, Group, Abandon };
        private DockStates State_ = DockStates.Tab;
        private DockStates State
        {
            get { return State_; }
            set
            {
                if (State_ == DockStates.Abandon)
                    throw new InvalidOperationException("Should not access an abandoned panel.");
                if (value == DockStates.Group)
                {
                    Group1.Visibility = Visibility.Visible;
                    Group2.Visibility = Visibility.Visible;
                    MainTab.Visibility = Visibility.Collapsed;
                    Splitter.Visibility = Visibility.Visible;
                }
                else
                {
                    Splitter.Visibility = Visibility.Collapsed;
                    MainTab.Visibility = Visibility.Visible;
                }
                State_ = value;
            }
        }
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
        }
        private void RefreshState()
        {
            if (State == DockStates.Abandon)
                return;
            if (Group1 != null && Group2 != null)
            {
                if (Children.Count > 0)
                    throw new InvalidOperationException("AnyDockPanel should not has children when group occupied!");
                if (PanelOrientation == Orientation.Horizontal)
                {
                    Grid.SetRow(Group1, 0); Grid.SetRowSpan(Group1, 3); Grid.SetColumn(Group1, 0); Grid.SetColumnSpan(Group1, 1);
                    Grid.SetRow(Group2, 0); Grid.SetRowSpan(Group2, 3); Grid.SetColumn(Group2, 2); Grid.SetColumnSpan(Group2, 1);
                    Grid.SetRow(Splitter, 0); Grid.SetRowSpan(Splitter, 3); Grid.SetColumn(Splitter, 1); Grid.SetColumnSpan(Splitter, 1);
                }
                else
                {
                    Grid.SetRow(Group1, 0); Grid.SetRowSpan(Group1, 1); Grid.SetColumn(Group1, 0); Grid.SetColumnSpan(Group1, 3);
                    Grid.SetRow(Group2, 2); Grid.SetRowSpan(Group2, 1); Grid.SetColumn(Group2, 0); Grid.SetColumnSpan(Group2, 3);
                    Grid.SetRow(Splitter, 1); Grid.SetRowSpan(Splitter, 1); Grid.SetColumn(Splitter, 0); Grid.SetColumnSpan(Splitter, 3);
                }
                State = DockStates.Group;
            }
            else
            {
                if (Group1 != null || Group2 != null)
                    throw new InvalidOperationException("Both group need to have element!");
                if (Children.Count == 0 && ParentPanel != null) // Children just get removed
                {
                    if (ParentPanel.State != DockStates.Group)
                        throw new InvalidOperationException("Won't hold a panel when not in GROUP mode");
                    AnyDockPanel remain;
                    if (ParentPanel.Group1 == this)
                        remain = ParentPanel.Group2;
                    else if (ParentPanel.Group2 == this)
                        remain = ParentPanel.Group1;
                    else
                        throw new InvalidOperationException("Parent doesn't hold the panel");
                    remain.HandoverToParent();
                    State = DockStates.Abandon;
                }
                else
                {
                    State = DockStates.Tab;
                }
            }
        }
        private void MoveChildrenTo(AnyDockPanel dst)
        {
            if (dst.Children.Count != 0)
                throw new InvalidOperationException("Should not has children when being moved.");
            var sel = MainTab.SelectedIndex;
            var kids = Children.ToArray();
            ShouldRefresh = false;
            Children.Clear();
            ShouldRefresh = true;
            foreach (var ele in kids)
                dst.Children.Add(ele);
            dst.MainTab.SelectedIndex = sel;
        }
        private void HandoverToParent()
        {
            if (ParentPanel == null)
                return;
            if (ParentPanel.State != DockStates.Group)
                throw new InvalidOperationException("Won't hold a panel when not in GROUP mode");
            if (State == DockStates.Tab)
            {
                ParentPanel.ShouldRefresh = false;
                ParentPanel.Group1 = ParentPanel.Group2 = null;
                ParentPanel.ShouldRefresh = true;
                MoveChildrenTo(ParentPanel);
            }
            else
            {
                ParentPanel.ShouldRefresh = ShouldRefresh = false;
                AnyDockPanel gp1 = Group1, gp2 = Group2;
                Group1 = Group2 = null;
                ParentPanel.PanelOrientation = PanelOrientation;
                ParentPanel.Group1 = gp1; ParentPanel.Group2 = gp2;
                ParentPanel.ShouldRefresh = ShouldRefresh = true;
            }
            ParentPanel.RefreshState();
            State = DockStates.Abandon;
        }

        private bool ShouldRefresh = false;
        

        public void SetGroups(AnyDockPanel panel1, AnyDockPanel panel2, Orientation orientation = Orientation.Horizontal)
        {
            ShouldRefresh = false;
            Group1 = panel1; Group2 = panel2;
            PanelOrientation = orientation;
            ShouldRefresh = true;
            RefreshState();
        }

        private class DragInfo { public TabItem Source = null; public Point StarPoint; }
        private static readonly DragInfo PendingDrag = new DragInfo();
        private class DragData
        {
            internal readonly ModifierKeys Keys;
            internal readonly FrameworkElement Element;
            internal readonly AnyDockPanel Panel;
            internal DragData(object source)
            {
                Keys = Keyboard.Modifiers;
                if (source is TabItem item && GetParentDock(item) == null)
                    Element = (FrameworkElement)item.Content;
                else
                    Element = (FrameworkElement)source;
                Panel = GetParentDock(Element);
            }
        }
        private void TabItemMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            PendingDrag.Source = (TabItem)sender;
            PendingDrag.StarPoint = e.GetPosition(null);
        }
        private void TabItemMouseLeave(object sender, MouseEventArgs e)
        {
            if (e.LeftButton != MouseButtonState.Pressed)
                return;
            var item = (TabItem)sender;
            if (PendingDrag.Source != item)
                return;
            BeginTabItemDrag(item);
            e.Handled = true;
        }
        private void TabItemMouseMove(object sender, MouseEventArgs e)
        {
            if (e.LeftButton != MouseButtonState.Pressed)
                return;
            var item = (TabItem)sender;
            if (PendingDrag.Source != item)
                return;
            var diff = e.GetPosition(null) - PendingDrag.StarPoint;
            if (Math.Abs(diff.X) <= SystemParameters.MinimumHorizontalDragDistance ||
                Math.Abs(diff.Y) <= SystemParameters.MinimumVerticalDragDistance)
                return;
            BeginTabItemDrag(item);
            e.Handled = true;
        }
        private static void BeginTabItemDrag(TabItem item)
        {
            PendingDrag.Source = null;
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
            var dst = new DragData(sender);
            if (src.Panel == null || dst.Panel == null)
                throw new InvalidOperationException("Drag objects should belong to AnyDock");
            if (src.Panel == dst.Panel)
            {
                if (src.Element == dst.Element)
                    return;
                // exchange order only
                int srcIdx = src.Panel.Children.IndexOf(src.Element);
                int dstIdx = dst.Panel.Children.IndexOf(dst.Element);
                src.Panel.Children.Move(srcIdx, dstIdx);
            }
            else
            {
                if (src.Element == dst.Element)
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
            DragOverLay.Visibility = Visibility.Collapsed;
            var hitted = VisualTreeHelper.HitTest(DragOverLay, e.GetPosition(DragOverLay));
            if (hitted == null || !((hitted.VisualHit as Polygon)?.Tag is string hitPart))
                return;
            var src = (DragData)e.Data.GetData(typeof(DragData));
            if (hitPart == "Middle")
            {
                if (Children.Count == 0)
                {
                    src.Panel.Children.Remove(src.Element);
                    Children.Add(src.Element);
                }
                else
                    TabItemDrop(MainTab.SelectedItem, e);
            }
            else
            {
                if (Group1 != null || Group2 != null)
                    throw new InvalidOperationException("Panel should not has groups when accept drop");
                AnyDockPanel panel1 = new AnyDockPanel(), panel2 = new AnyDockPanel();
                src.Panel.Children.Remove(src.Element);
                var dstPanel = State == DockStates.Abandon ? ParentPanel : this; // in case collapsed
                switch (hitPart)
                {
                case "Up":
                case "Left":
                    panel1.Children.Add(src.Element);
                    dstPanel.MoveChildrenTo(panel2);
                    break;
                case "Down":
                case "Right":
                    dstPanel.MoveChildrenTo(panel1);
                    panel2.Children.Add(src.Element);
                    break;
                default:
                    throw new InvalidOperationException($"Unknown Tag [{hitPart}]");
                }
                dstPanel.SetGroups(panel1, panel2, (hitPart == "Left" || hitPart == "Right") ? Orientation.Horizontal : Orientation.Vertical);
            }
            e.Handled = true;
        }


        public string SelfCheck(int level)
        {
            var prefix = new string('\t', level);
            string ret = "";
            ret += $"{prefix}AnyDock[{State}][{Children.Count} kids]\n";
            if (State == DockStates.Tab)
            {
                foreach (var kid in Children)
                    ret += $"{prefix}- {(kid as Label).Content} \t [{(MainTab.SelectedContent == kid ? 'X': ' ')}]\n";
            }
            else
            {
                var label1 = PanelOrientation == Orientation.Horizontal ? "Left " : "Up   ";
                var label2 = PanelOrientation == Orientation.Horizontal ? "Right" : "Down ";
                ret += $"{prefix}- [{label1}]\n";
                ret += Group1.SelfCheck(level + 1);
                ret += $"{prefix}- [{label2}]\n";
                ret += Group2.SelfCheck(level + 1);
            }
            return ret;
        }

    }
}
